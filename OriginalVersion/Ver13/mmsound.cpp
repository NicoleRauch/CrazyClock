#define INCL_DOSPROCESS 	// DosBeep

#define INCL_DOSMODULEMGR
#include <os2.h>

#include "mmsound.h"
#include <string.h>	// memset

// #include "..\_Standard\wcapi.h"

// Globals (for the pmsndx part)
BOOL MMPMModuleLoaded = FALSE;
BOOL MMIOModuleLoaded = FALSE;
CHAR szFailBuff[CCHMAXPATH];
HMODULE hDLLModuleMci;
HMODULE hDLLModuleMmio;
VOID mmio_error( ULONG rc, char **s );

#ifndef __REAL_DLL__

  typedef ULONG (APIENTRY *PFNMCISENDCOMMAND) (USHORT,USHORT,ULONG,PVOID,USHORT);
  typedef HMMIO (*PFNMMIOOPEN) (PSZ, PMMIOINFO, ULONG );
  typedef ULONG (*PFNMMIOGETHEADER) (HMMIO, PVOID, LONG, PLONG, ULONG, ULONG );
  typedef LONG (*PFNMMIOREAD) (HMMIO, PCHAR, LONG );
  typedef USHORT (*PFNMMIOCLOSE) (HMMIO, USHORT );

  #define pmciSendCommand (*pfnMciSendCommand)
  #define pmmioOpen (*pfnMmioOpen)
  #define pmmioGetHeader (*pfnMmioGetHeader)
  #define pmmioRead (*pfnMmioRead)
  #define pmmioClose (*pfnMmioClose)

#endif
#ifdef __REAL_DLL__

  #define pmciSendCommand mciSendCommand
  #define pmmioOpen mmioOpen
  #define pmmioGetHeader mmioGetHeader
  #define pmmioRead mmioRead
  #define pmmioClose mmioClose

#endif


PFNMCISENDCOMMAND pfnMciSendCommand;
PFNMMIOOPEN			pfnMmioOpen;
PFNMMIOGETHEADER	pfnMmioGetHeader;
PFNMMIOREAD			pfnMmioRead;
PFNMMIOCLOSE		pfnMmioClose;


// end of Globals

//
// mmaudioheader
//

class mmaudioheader {
    MMAUDIOHEADER mmah;
public:
    mmaudioheader();
    MMAUDIOHEADER* get_addr() { return &mmah; }
    LONG           get_size() { return sizeof(mmah); }
};


mmaudioheader::mmaudioheader()
{
    char* p = (char*) &mmah;
    for (int i=0; i<sizeof(mmah); ) p[i++] = 0;
}

mem_wav& mem_wav::operator()( char new_start )
{
	if( new_start && new_start <= pl_depth ){
		pl_start = (CHAR)(pl_depth - new_start);  
	}
	return *this;
}
		
mem_wav::mem_wav(char* name, char *error, char repeat)	// repeat = 1 (predefined)
{
	*error = ERROR_UNSPECIFIED;
	pl_depth = repeat;
	pl_start = 0;	// must be reset after each use

// wcprintf("Open file:");
	MMIOINFO dummy;
	memset( &dummy, 0, sizeof(MMIOINFO ) );
	dummy.fccIOProc = FOURCC_WAVE;
   hmmio = pmmioOpen(name, &dummy, MMIO_READ | MMIO_DENYNONE);

   if( dummy.ulErrorRet )
//		wcprintf("Error %ld", dummy.ulErrorRet );	
   if( !hmmio ){
//    	wcprintf( "MULLHANDLE zurckgegeben");
		*error = ERROR_NULLHANDLE;
		return;
	}


    // get header

    mmaudioheader mmah;

    LONG BytesRead = 0;

//wcprintf("Get header");
    
    ULONG rc = pmmioGetHeader(hmmio, (PVOID) mmah.get_addr(), mmah.get_size(), 
                        &BytesRead, 0, 0);

	if( rc != MMIO_SUCCESS ){
		char *s;
		mmio_error( rc, &s );
//		wcprintf("mmio error %ld %s", rc, s );
		*error = ERROR_GET_HEADER;
		return;
	}
                        
    // get some infos about the waveaudio file
//wcprintf("Get info");
    
    Channels      = mmah.get_addr()->mmXWAVHeader.WAVEHeader.usChannels;
    SamplesPerSec = mmah.get_addr()->mmXWAVHeader.WAVEHeader.ulSamplesPerSec;
    BitsPerSample = mmah.get_addr()->mmXWAVHeader.WAVEHeader.usBitsPerSample;

    bsize = mmah.get_addr()->mmXWAVHeader.XWAVHeaderInfo.ulAudioLengthInBytes;
    bptr  = new CHAR[bsize];


    // read file
//wcprintf("Read file");
    rc = pmmioRead(hmmio, bptr, bsize);
 
    // close file
// wcprintf("Close file");
    rc = pmmioClose(hmmio, 0);

		/* setup playlist */

	 pl = new PLE[repeat + 1];
	 
	for( char i = 0; i < repeat; i++ ){
		pl[i].operation = DATA_OPERATION; 
		pl[i].operand1  = (ULONG)bptr;
		pl[i].operand2  = bsize;
		pl[i].operand3  = 0;
	}

    pl[repeat].operation = EXIT_OPERATION;
    pl[repeat].operand1  = 0;
    pl[repeat].operand2  = 0;
    pl[repeat].operand3  = 0;

    *error = ERROR_NO_ERROR;
}


mem_wav::~mem_wav()
{
    delete[] bptr;
    delete[] pl;
    delete this;
}

VOID mem_wav::play()
{
    MCI_OPEN_PARMS mop;
    ULONG rc;
    MCI_WAVE_SET_PARMS wsp;
    MCI_PLAY_PARMS mpp;
    MCI_GENERIC_PARMS mgp;
    
    /* open device  */

    mop.hwndCallback   = 0;
    mop.usDeviceID     = 0;
    mop.pszDeviceType  = MCI_DEVTYPE_WAVEFORM_AUDIO_NAME;
    mop.pszElementName = (PSZ)&pl[pl_start];

    rc = pmciSendCommand(0,
                        MCI_OPEN,                        /* open message */
                        MCI_WAIT | MCI_OPEN_SHAREABLE |  /* message flags */
                        MCI_OPEN_PLAYLIST,
                        &mop,                            /* parameters */
                        0);


    /* set device parameters */
    wsp.hwndCallback    = 0;
    wsp.ulSamplesPerSec = SamplesPerSec;
    wsp.usBitsPerSample = BitsPerSample;
    wsp.usChannels = Channels;

   rc = pmciSendCommand(mop.usDeviceID, MCI_SET,
			MCI_WAIT | MCI_WAVE_SET_SAMPLESPERSEC,
         &wsp, 0);

    rc = pmciSendCommand(mop.usDeviceID, MCI_SET,
			MCI_WAIT | MCI_WAVE_SET_BITSPERSAMPLE,
         &wsp, 0);

    rc = pmciSendCommand(mop.usDeviceID, MCI_SET,
			MCI_WAIT | MCI_WAVE_SET_CHANNELS,
         &wsp, 0);

    /* play the playlist */

    mpp.hwndCallback = 0;

    rc = pmciSendCommand(mop.usDeviceID,
                              MCI_PLAY,
                              MCI_WAIT,
                              &mpp,
                              0);


    /* close device */

    mgp.hwndCallback = 0;

    rc = pmciSendCommand(mop.usDeviceID,
                              MCI_CLOSE,
                              MCI_WAIT,
                              &mgp,
                              0);

	ResetPlStart();	// necessary!                              

}


/////////////////// code from pmsndx

LONG LoadMMPMSupport(VOID)
{
#ifndef __REAL_DLL__
    APIRET RetCode;
 
    if ( !MMPMModuleLoaded ){
	   RetCode = DosLoadModule(szFailBuff,sizeof(szFailBuff),"mdm",&hDLLModuleMci);
    	if (RetCode)
        return(RetCode);
      RetCode = DosQueryProcAddr(hDLLModuleMci,0L,"mciSendCommand",(PFN *)&pfnMciSendCommand);
      if (RetCode){
          DosFreeModule(hDLLModuleMci);
          return(RetCode);
      }
      MMPMModuleLoaded = TRUE;
    } // !MMPMModuleLoaded
    
    if ( !MMIOModuleLoaded ){
	   RetCode = DosLoadModule(szFailBuff,sizeof(szFailBuff),"mmio",&hDLLModuleMmio);
    	if (RetCode)
        return(RetCode);

      RetCode = DosQueryProcAddr(hDLLModuleMmio,0L,"mmioOpen",
					(PFN *)&pfnMmioOpen);
      if (RetCode){
          DosFreeModule(hDLLModuleMmio);
          return(RetCode);
      }
      RetCode = DosQueryProcAddr(hDLLModuleMmio,0L,"mmioGetHeader",
					(PFN *)&pfnMmioGetHeader);
      if (RetCode){
          DosFreeModule(hDLLModuleMmio);
          return(RetCode);
      }
      RetCode = DosQueryProcAddr(hDLLModuleMmio,0L,"mmioRead",
					(PFN *)&pfnMmioRead);
      if (RetCode){
          DosFreeModule(hDLLModuleMmio);
          return(RetCode);
      }
      RetCode = DosQueryProcAddr(hDLLModuleMmio,0L,"mmioClose",
					(PFN *)&pfnMmioClose);
      if (RetCode){
          DosFreeModule(hDLLModuleMmio);
          return(RetCode);
      }
      MMIOModuleLoaded = TRUE;
    } // !MMIOModuleLoaded
#endif
    MMPMModuleLoaded = TRUE;
    MMIOModuleLoaded = TRUE;
    return(FALSE);
}
 
LONG FreeMMPMSupport(VOID)
{
#ifndef __REAL_DLL__
    if (MMPMModuleLoaded){
        DosFreeModule(hDLLModuleMci);
        MMPMModuleLoaded = FALSE;
    }
    if (MMIOModuleLoaded){
        DosFreeModule(hDLLModuleMmio);
        MMIOModuleLoaded = FALSE;
    }
    return(FALSE);
#endif
#ifdef __REAL_DLL__
    MMPMModuleLoaded = FALSE;
    MMIOModuleLoaded = FALSE;
    return(FALSE);
#endif
}

//////////// end of code from pmsndx






VOID PlaySound( mem_wav *sound, BOOL play )
{
	if( play )			// do the soundflag checking in here
		sound->play();
}



VOID mmio_error( ULONG rc, char **s )
{
	switch (rc) {
		case MMIO_SUCCESS:
		    *s = "SUCCESS (huh?)";
		    break;
		case MMIOERR_UNBUFFERED:
		    *s = "UNBUFFERD";
		    break;
		case MMIOERR_INVALID_HANDLE:
		    *s = "INVALID HANDLE";
		    break;
		case MMIOERR_INVALID_PARAMETER:
		    *s = "INVALID PARAMETER";
		    break;
		case MMIOERR_READ_ONLY_FILE:
		    *s = "READ ONLY FILE";
		    break;
		case MMIOERR_WRITE_ONLY_FILE:
		    *s = "WRITE ONLY FILE";
		    break;
		case MMIOERR_WRITE_FAILED:
		    *s = "WRITE FAILED";
		    break;
		 case MMIOERR_READ_FAILED:
		     *s = "READ FAILED";
		     break;
		 case MMIOERR_SEEK_FAILED:
		     *s = "SEEK FAILED";
		     break;
		 case MMIOERR_NO_FLUSH_NEEDED:
		     *s = "NO FLUSH NEEDED";
		     break;
		 case MMIOERR_OUTOFMEMORY:
		     *s = "OUT OF MEMORY";
		     break;
		 case MMIOERR_CANNOTEXPAND:
		     *s = "CANNOT EXPAND";
		     break;
		 case MMIOERR_FREE_FAILED:
		     *s = "FREE FAILED";
		     break;
		 case MMIOERR_CHUNKNOTFOUND:
		     *s = "CHUNK NOT FOUND";
		     break;
		 case MMIO_ERROR:
		     *s = "ERROR";
		     break;
		case MMIO_WARNING:
		    *s = "WARNING";
		    break;
		case MMIO_CF_FAILURE:
		    *s = "CF FAILURE";
		    break;
		default:
		    *s = " (hmm...)";
		    break;
	}		
}
