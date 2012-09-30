#ifndef _MMSOUND_H_
#define _MMSOUND_H_

#define  INCL_OS2MM
#include <os2me.h>

#define ERROR_NO_ERROR     0
#define ERROR_UNSPECIFIED	1
#define ERROR_NULLHANDLE 	2
#define ERROR_GET_HEADER	3

// this one is missing in mmioos2.h
#define FOURCC_WAVE mmioFOURCC('W', 'A', 'V', 'E')

/*
// ple: a playlist entry	gleich geblieben
*/
// was ist mit class playlist?

struct PLE {
    ULONG operation;
    ULONG operand1;
    ULONG operand2;
    ULONG operand3;
};



//
// mem_wav: a waveaudio file loaded into memory
//

class mem_wav {
    HMMIO  hmmio;
    PSZ    bptr;
    ULONG  bsize;
    USHORT Channels;
    ULONG  SamplesPerSec;
    USHORT BitsPerSample;
    PLE *pl;
// CHAR *name; wenn mal OPEN_ELEMENT klappt
	unsigned char pl_depth;
	unsigned char pl_start;
	VOID ResetPlStart(){ pl_start = 0; }
    VOID play();
    PSZ    get_bptr() { return bptr; }
    ULONG  get_bsize() { return bsize; }
    USHORT get_Channels() { return Channels; }
    ULONG  get_SamplesPerSec() { return SamplesPerSec; }
    USHORT get_BitsPerSample() { return BitsPerSample; }
public:
    mem_wav(char* name, char *error, char repeat = 1);
    ~mem_wav();
    mem_wav& operator()(char);
	friend VOID PlaySound( mem_wav *sound, BOOL play );
};


VOID PlaySound( mem_wav *sound, BOOL play );
//inline VOID PlaySound( mem_wav& sound ) {	sound.play(); }

LONG LoadMMPMSupport(VOID);
LONG FreeMMPMSupport(VOID);

#endif
