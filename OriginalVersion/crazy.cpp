   //-------------------------------------------------------------------
   //
   // crazy.cpp
   //
   // Crazy Clock, for people who have already got everything
   //
   //-------------------------------------------------------------------

   // Includes 

#define INCL_DOSPROCESS
#define INCL_WIN
#define INCL_GPI
#define INCL_WINDIALOGS
#include <os2.h>
#include <stdlib.h>     // atoi(), ...
#include <string.h>	// memset()
#include <process.h>	// _beginthread(), _endthread()

#include "mytime.h"
#include "defs.h"
#include "local-profile.h"

#include "Sound.h"


   /* Globals*/

HELPINIT hiInit;
HWND    hwndMain,
   hwndWorker;

HWND hwndFrame;
HWND hwndTitleBar;
HWND hwndSysMenu;
HWND hwndMinMax;
HWND hwndMenu;
HWND hwndPopupMenu;
HWND hwndHelp;

   // old frame procedure before subclassing
PFNWP OldFrameProc;

   // neu: help hook function:
BOOL EXPENTRY help_hook( HAB hab, SHORT sMode, USHORT usTopic,
			 USHORT usSubTopic, PRECTL prclPos );

   // sound variables:
Sound *number[10]; // array of pointers
void ChimeSoundFunc();
void AlarmSoundFunc();
Sound  alarm( "alarm.wav", 1, AlarmSoundFunc ), 
   chime( "chime.wav", 1, ChimeSoundFunc );

   /* Prototypes	*/

MRESULT EXPENTRY MainWndProc (HWND, ULONG, MPARAM, MPARAM);
VOID             WorkerThread (VOID *);
MRESULT EXPENTRY WorkWndProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY AboutDlgProc(HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY ColorDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY SettingsDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY AlarmDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY AlarmMsgDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
   // function for subclassed frame:
MRESULT EXPENTRY NewFrameProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
VOID ClkShowFrameControls ( HWND hwndFrame );
VOID ClkHideFrameControls ( HWND hwndFrame );
VOID AccessSysMenu( HWND hwndFrame, BOOL Access );
VOID InitDefault();
VOID AdditionsToSysMenu();
VOID ToggleEnlarged();
VOID ToggleIconFrame();

   /* Main Program!	*/

int main (void)
{
   HAB     hab;
   HMQ     hmq;
   QMSG    qmsg;
   LONG cxScreen;
   LONG cyScreen;
   ULONG flFrameFlags = FCF_TITLEBAR | FCF_SYSMENU  |
      FCF_SIZEBORDER | FCF_MINMAX   |
      FCF_TASKLIST   | FCF_MENU     |
      FCF_ACCELTABLE;


   hab = WinInitialize (0);
   hmq = WinCreateMsgQueue (hab, 0);

   PSZ pszFullAppName = new CHAR[50];
   WinLoadString( HAB(0L), NULLHANDLE, STRG_APPNAME, 50, pszFullAppName );
      // loads app name
   PSZ pszAppName = new CHAR[strlen(pszFullAppName)+1];
   strcpy( pszAppName, pszFullAppName );

   PSZ pszVersion = new CHAR[20];
   WinLoadString( HAB(0L), NULLHANDLE, STRG_VERSION, 20, pszVersion );
      // loads version number
   PSZ pszIniName = new CHAR[20];
   WinLoadString( HAB(0L), NULLHANDLE, STRG_ININAME, 20, pszIniName );
      // loads ini file name

      // append the version number to the application name for displaying:
   strcat( pszFullAppName, " " );
   strcat( pszFullAppName, pszVersion );

      // create a profile instance
   PROFILE Profile( hab, pszAppName, pszIniName, pszVersion );


   WinRegisterClass (hab, pszFullAppName, MainWndProc, CS_SIZEREDRAW |
		     CS_MOVENOTIFY, 0);
      // frame sends a WM_MOVE message to client

   hwndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
				   &flFrameFlags, pszFullAppName, NULL,
				   0, NULLHANDLE, IDR_MAIN, &hwndMain);

      // make it get notifications when the visible region gets altered
   WinSetVisibleRegionNotify( hwndFrame, TRUE );

   hwndTitleBar = WinWindowFromID ( hwndFrame , FID_TITLEBAR ) ;
   hwndSysMenu = WinWindowFromID ( hwndFrame , FID_SYSMENU ) ;
   hwndMinMax = WinWindowFromID ( hwndFrame , FID_MINMAX ) ;
   hwndMenu = WinWindowFromID ( hwndFrame , FID_MENU ) ;

   hwndPopupMenu = WinLoadMenu( HWND_OBJECT, 0, IDR_POPUP );

      // set "Crazy Clock" as window title, even when started
      // from the command line
   WinSetWindowText( hwndFrame, pszFullAppName );

      // subclass the frame window procedure
   OldFrameProc = WinSubclassWindow( hwndFrame, NewFrameProc );
	
      // do a basic initialization of all variables
   cxScreen = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
   cyScreen = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
   InitDefault();		// initializes the variables
   WinSetWindowPos( hwndFrame, NULLHANDLE, cxScreen / 2, cyScreen / 20,
		    cxScreen / 10, cxScreen / 10,
		    SWP_SIZE | SWP_MOVE | SWP_HIDE );

      // read out of the profile what's in there 
   ReadProfile( Profile, hwndFrame, pszAppName );

   AdditionsToSysMenu();	// first DotsEnlarged must be set, then
      // the appropriate system menu entry can be added

   if( !CrazyClock.QueryfFrameIsVisible() ){	
	 // hide frame, adjust menu entry text
      if( ! (WinQueryWindowULong( hwndFrame, QWL_STYLE ) & WS_MINIMIZED) )
	    // only if prog is not minimized:
	 ClkHideFrameControls ( hwndFrame );	
   }

	
      // Help facilities
   hiInit.cb = sizeof( HELPINIT );
   hiInit.ulReturnCode = 0L;
   hiInit.pszTutorialName = NULL;
   hiInit.phtHelpTable = (PHELPTABLE)MAKEULONG(HELP_CLOCK, 0xFFFF );
   hiInit.hmodHelpTableModule = NULLHANDLE;
   hiInit.hmodAccelActionBarModule = NULLHANDLE;
   hiInit.idAccelTable = 0;
   hiInit.idActionBar = 0;
   hiInit.pszHelpWindowTitle = new CHAR[50];
   WinLoadString( HAB(0L), NULLHANDLE, STRG_HELPTITLE, 50, 
		  hiInit.pszHelpWindowTitle );
   hiInit.fShowPanelId = CMIC_HIDE_PANEL_ID;
   hiInit.pszHelpLibraryName = new CHAR[15];
   WinLoadString( HAB(0L), NULLHANDLE, STRG_HELPFILE, 15, 
		  hiInit.pszHelpLibraryName );
   hwndHelp = WinCreateHelpInstance( hab, &hiInit );
   if( hwndHelp != NULLHANDLE ){
      if( hiInit.ulReturnCode != 0 ){	// error
	 WinDestroyHelpInstance( hwndHelp );
	 hwndHelp = NULLHANDLE;
      } else { // everything o.k.
	 WinAssociateHelpInstance( hwndHelp, hwndFrame );
      }
   }
   if( hwndHelp == NULLHANDLE ){
	 // is not an else case because hwndHelp may
	 // have been changed in the if above
      PSZ tmpMsg = new CHAR[200];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_NOHELP, 200, 
		     tmpMsg );

      WinMessageBox( HWND_DESKTOP, hwndFrame, tmpMsg,
		     pszAppName, 0, MB_OK | MB_CRITICAL );
      delete[] tmpMsg;
   }
    
      //  Zum Testen rausgenommen  WinSetHook( hab, hmq, HK_HELP, (PFN)help_hook, NULLHANDLE ); // neu

      // now we can make the window visible
   WinSetWindowPos( hwndFrame, 0L, 0L, 0L, 0L, 0L, SWP_SHOW );

   while (WinGetMsg (hab, &qmsg, 0, 0, 0))
      WinDispatchMsg (hab, &qmsg);
      // WM_QUIT has been received, now clean up:

   WriteProfile( Profile, hwndFrame, pszAppName );
    
      // Zum Testen rausgenommen    WinReleaseHook( hab, hmq, HK_HELP, (PFN)help_hook, NULLHANDLE ); // neu

   if( hwndHelp != NULLHANDLE ){
      WinAssociateHelpInstance( NULLHANDLE, hwndFrame );
      WinDestroyHelpInstance( hwndHelp );
      hwndHelp = NULLHANDLE;
   }

   WinDestroyWindow (hwndFrame);
   WinDestroyMsgQueue (hmq);
   WinTerminate (hab);

      // here we can free the memory that has been allocated for resource 
      // strings:
   delete[] pszFullAppName;
   delete[] pszAppName;
   delete[] pszVersion;
   delete[] pszIniName;

   return 0;
}

   // neu:
BOOL EXPENTRY help_hook( HAB hab, SHORT sMode, USHORT usTopic,
			 USHORT usSubTopic, PRECTL prclPos )
{
   if( (sMode == HLPM_WINDOW) && (hwndHelp != NULLHANDLE) ){
      WinSendMsg( hwndHelp, HM_DISPLAY_HELP, 
		  MPFROMLONG( MAKELONG( usTopic, 0 ) ),
		  MPFROMSHORT( HM_RESOURCEID ) );
      return TRUE;
   } else
      return FALSE;
}

MRESULT EXPENTRY NewFrameProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   switch( msg ){

	 /*
	   case WM_PAINT:
	   if( WinQueryWindowULong( hwnd, QWL_STYLE ) & WS_MINIMIZED ){
	   HPS hps = WinBeginPaint( hwnd, 0, 0 );
	   WinEndPaint( hps );
	   WinBroadcastMsg( hwnd, msg, mp1, mp2,
	   BMSG_POST | BMSG_DESCENDANTS );
	      //	WinPostMsg( hwndMain, msg, mp1, mp2 );
	      return (MRESULT)FALSE;	// to indicate that painting is done
	      }
	      break;
	      */

   case WM_QUERYTRACKINFO: {
	 // let standard procedure initialize all
      MRESULT mReturn = (*OldFrameProc)(hwnd, msg, mp1, mp2 );
      PTRACKINFO pti = (PTRACKINFO)mp2;
	 // adjust the interesting values
      CrazyClock.SetMinTrackSize( pti->ptlMinTrackSize );
      return mReturn;
   }
   }	// switch ( msg )
   return (*OldFrameProc) (hwnd, msg, mp1, mp2 );
}


MRESULT EXPENTRY MainWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg){
   case WM_CREATE:
      if (_beginthread (&WorkerThread, NULL, 8192, (VOID*)NULL) == -1){
	 PSZ tmpMsg = new CHAR[100];
	 WinLoadString( HAB(0L), NULLHANDLE, STRG_NO2NDTHREAD, 100, 
			tmpMsg );
	 PSZ pszAppName = new CHAR[50];
	 WinLoadString( HAB(0L), NULLHANDLE, STRG_APPNAME, 50, 
			pszAppName );
	 WinMessageBox (HWND_DESKTOP, hwnd, tmpMsg, pszAppName,
			0, MB_OK | MB_CUACRITICAL);
	 delete[] tmpMsg;
	 delete[] pszAppName;
	 return 0;
      }
      colorset.cb = sizeof( COL_DEFS );
      return 0;

   case WM_ACK:	// is only received at the very start
      WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
      return 0;

   case WM_SIZE:
	 // each WM_SIZE is followed by a WM_PAINT msg, so
	 // I can reduce the amount of work by skipping this one
	 // WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
      return 0;

   case WM_MOVE:		// parent is moved	
   case WM_PAINT:
	 // WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
      WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
      break;
	 // return 0;		// very dangerous!!

   case WM_ERASEBACKGROUND:
	 // if window is minimized and frame visible, let OS paint the
	 // background:
      if( (WinQueryWindowULong( hwndFrame, QWL_STYLE )  & WS_MINIMIZED)
	  && CrazyClock.QueryfIconFrameIsVisible() ){
	 WinSendMsg( hwndWorker, WM_FRAMEREGION, 0, 0 );
      }
	 // otherwise we do the painting ourselves
      return (MRESULT) FALSE;

   case WM_CONTEXTMENU: {
      POINTL ptl;
      BOOL bKeyboardUsed = SHORT2FROMMP( mp2 );
      if( !bKeyboardUsed ){
	 ptl.x = SHORT1FROMMP( mp1 );
	 ptl.y = SHORT2FROMMP( mp1 );
      } else {
	 RECTL rect;
	 WinQueryWindowRect( hwnd, &rect );
	 ptl.x = rect.xRight / 2;
	 ptl.y = rect.yTop / 2;
      }
      if( hwndPopupMenu != NULLHANDLE ){
	 WinPopupMenu( hwnd, hwnd, hwndPopupMenu,
		       ptl.x, ptl.y, ID_OPTIONS,
		       PU_HCONSTRAIN | PU_VCONSTRAIN |
		       PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 );
      }
   }
   break;

   case WM_BUTTON1CLICK:
      {
	 if( !CrazyClock.QueryfReadNumbers() ) return 0;
	 POINTL ptl;
	 WinSendMsg( hwndWorker, WM_END_PAINT, 0, 0 );	// stop clock
	 ptl.x = SHORT1FROMMP( mp1 );
	 ptl.y = SHORT2FROMMP( mp1 );
	 WinPostMsg( hwndWorker, WM_READ_ONE,
		     MPFROMLONG( CrazyClock.PositionToRead( ptl )),
		     0 );
      }
   break;
			
   case WM_BUTTON1MOTIONSTART:	// user tries to drag the window
   case WM_BUTTON2MOTIONSTART:	
	 // do nothing when frame is visible
      if( CrazyClock.QueryfFrameIsVisible() ) break;	
      WinPostMsg( hwndFrame, WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), 0 );
      break;	// message processed

   case WM_VRNENABLED:
	 // don't do anything if "float to top" is disabled
      if( !CrazyClock.QueryfFloatToTop() ) break;

	 // to bring the window to the front if it gets covered:
      WinSetWindowPos( hwndFrame, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER );
      break;

   case WM_FOCUSCHANGE:
	 // not necessary if frame is visible
      if( CrazyClock.QueryfIconFrameIsVisible() ) break;
      WinPostMsg( hwndWorker, WM_BEGIN_PAINT,
		  MPFROMLONG( WinQueryWindowULong( hwndFrame, QWL_STYLE )
			      & WS_MINIMIZED ),
		     // repaint only when minimized
		  0);
      break;	

	 //	the program is no longer forced to the desktop	   
   case WM_MINMAXFRAME: {
	 // the window is being minimized, maximized or restored
      SWP *swp = (SWP *)mp1;

      if( !( swp->fl & SWP_MINIMIZE ) ){
	    // window is not to be minimized,
	    // i. e. window is being restored or maximized
	 if( WinQueryWindowULong( hwndFrame, QWL_STYLE ) 
	     & WS_MINIMIZED ){
	       // window is minimized right now
	    if( !CrazyClock.QueryfFrameIsVisible() ){
		  // controls must be hidden
	       ClkHideFrameControls( hwndFrame );
	    }
	 }
      }
   }
   break;


   case WM_COMMAND:
      switch (SHORT1FROMMP(mp1)){
      case ID_COLORS: {
	 BOOL FirstPaint;
	 FirstPaint = WinLoadDlg( HWND_DESKTOP, hwndMain, ColorDlgProc,
				  (HMODULE)0, DLG_COLORS, 
				  (VOID *)&colorset );
	 WinPostMsg( hwndWorker, WM_BEGIN_PAINT,
		     MPFROMLONG( FirstPaint ), 0);
	    // here a WM_PAINT msg is only received when the window
	    // was covered by the dialog box, so it is safer to post
	    // the begin-paint msg, too
      }
      break;
      case ID_SETTINGS: {
	 BOOL FirstPaint;
	 FirstPaint = WinLoadDlg( HWND_DESKTOP, hwndMain, 
				  SettingsDlgProc, (HMODULE)0, 
				  DLG_SETTINGS, NULL );
	 WinPostMsg( hwndWorker, WM_BEGIN_PAINT,
		     MPFROMLONG( FirstPaint ), 0);
	    // here a WM_PAINT msg is only received when the window
	    // was covered by the dialog box, so it is safer to post
	    // the begin-paint msg, too
      }
      break;
      case ID_ALARM: {
	 BOOL FirstPaint;
	 FirstPaint = WinLoadDlg( HWND_DESKTOP, hwndMain, 
				  AlarmDlgProc, (HMODULE)0, 
				  DLG_ALARM, NULL );
	    //	    WinPostMsg( hwndWorker, WM_BEGIN_PAINT,
	    //			MPFROMLONG( FirstPaint ), 0);
	    // here a WM_PAINT msg is only received when the window
	    // was covered by the dialog box, so it is safer to post
	    // the begin-paint msg, too
      }
      break;
      case ID_WINDOWSIZE: {
	 INT Width, Height;
	 RECTL RectInner, RectOuter;

	 CrazyClock.CalcMinWindowSize( &Width, &Height );
	 WinQueryWindowRect( hwnd, &RectInner );
	 WinQueryWindowRect( hwndFrame, &RectOuter );
	 RectInner.xRight = Width + RectOuter.xRight - RectInner.xRight;
	 RectInner.yTop = Height + RectOuter.yTop - RectInner.yTop;
	 WinQueryWindowRect( hwndFrame, &RectOuter );
	 WinMapWindowPoints( hwndFrame, HWND_DESKTOP, 
			     (PPOINTL)(&RectOuter), 2 );
	 if( RectOuter.xLeft < 0 ) RectOuter.xLeft = 0;
	 if( RectOuter.yBottom < 0 ) RectOuter.yBottom = 0;
	 WinSetWindowPos( hwndFrame, NULLHANDLE, RectOuter.xLeft,
			  RectOuter.yBottom, RectInner.xRight,
			  RectInner.yTop,
			  SWP_SIZE | SWP_MOVE );
	    // check whether menu entry doubling has affected the size:
	 WinQueryWindowRect( hwnd, &RectInner );
	 if( RectInner.yTop != Height ){
	    WinQueryWindowRect( hwndFrame, &RectOuter );
	    RectInner.yTop = Height + RectOuter.yTop - RectInner.yTop;
	    WinSetWindowPos( hwndFrame, NULLHANDLE, 0,
			     0, RectInner.xRight, RectInner.yTop,
			     SWP_SIZE );
	 }
						
      }
      break;
				
      case ID_MINIMIZE:
	 if( !CrazyClock.QueryfFrameIsVisible() ){
	       // frame controls must be reassigned to the window
	    ClkShowFrameControls ( hwndFrame );
	    CrazyClock.SetfFrameIsVisible( FALSE );	// correct the flag
	 }	// kleine Transplantation
	 WinSetWindowPos( hwndFrame, 0L, 0L, 0L, 0L,
			  0L, SWP_MINIMIZE | SWP_SHOW );
	 break;

      case ID_READ_ALL:	// (not in use right now! )
	    // read the numbers that are currently being displayed
	 WinPostMsg( hwndWorker, WM_READ_ALL, 0, 0 );
	 break;

      case ID_EXIT:
	 WinPostMsg( hwnd, WM_CLOSE, 0, 0 );
	 break;

      case ID_ENLARGED:
	 WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
	 ToggleEnlarged();
	 WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
	 break;
					
      case ID_ICONFRAME:
	 WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
	 ToggleIconFrame();
	 WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
	 break;
					
      case ID_HELP_INDEX:		// help index requested
	 WinSendMsg( hwndHelp, HM_HELP_INDEX, (MPARAM)0, (MPARAM)0 );
	 break;	

      case ID_EXT_HELP:		// general help requested
	 WinSendMsg( hwndHelp, HM_EXT_HELP, (MPARAM)0, (MPARAM)0 );
	 break;

      case ID_DISPLAY_HELP:	// help on help (system page)
	 WinSendMsg( hwndHelp, HM_DISPLAY_HELP, (MPARAM)0, (MPARAM)0 );
	 break;

      case ID_KEYS_HELP:
	 WinSendMsg( hwndHelp, HM_KEYS_HELP, (MPARAM)0, (MPARAM)0 );
	 break;	

      case ID_ABOUT:
	 WinLoadDlg( HWND_DESKTOP, hwndMain, AboutDlgProc,
		     (HMODULE)0, DLG_ABOUT, NULL );
	 return 0;
      default:
	 return WinDefWindowProc(hwnd,msg,mp1,mp2);
      }	// end switch ( SHORT1FROMMP )
      break;	// case WM_COMMAND

   case HM_QUERY_KEYS_HELP:		// system asks which page to display
      return MRFROMSHORT( HELP_KEYS_HELP );
			
   case WM_CLOSE:
	 // here was 	WriteProfile( WinQueryAnchorBlock( hwndFrame ) );
      WinPostMsg(hwndWorker, WM_CLOSE, mp1, mp2);
      return 0;
   }	// end switch( msg )
   return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


VOID WorkerThread (VOID *dummy)
{
   HAB  hab;
   HMQ  hmq;
   QMSG qmsg;

      // wcprintf("Start von WorkerThread");	
   dummy = NULL;	// to avoid compiler error messages
   hab = WinInitialize (0);
   hmq = WinCreateMsgQueue(hab, 0);
   PSZ psz2ndThreadName = new CHAR[50];
   WinLoadString( HAB(0L), NULLHANDLE, STRG_2NDTHREADNAME, 50, 
		  psz2ndThreadName );

   WinRegisterClass(hab, psz2ndThreadName, WorkWndProc, 0, 0);
   hwndWorker = WinCreateWindow( HWND_OBJECT, psz2ndThreadName, "",
				 0, 0, 0, 0, 0, HWND_OBJECT, 
				 HWND_BOTTOM, 0, NULL, NULL );

   WinSendMsg( hwndMain, WM_ACK, 0, 0 );

      // free the temporary string memory
   delete[] psz2ndThreadName;
   while( WinGetMsg ( hab, &qmsg, 0, 0, 0 ))
      WinDispatchMsg ( hab, &qmsg );

   WinPostMsg( hwndMain, WM_QUIT, 0, 0 );
   WinDestroyWindow( hwndWorker );
   WinDestroyMsgQueue( hmq );
   WinTerminate (hab);
   _endthread ();
}


MRESULT EXPENTRY WorkWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   static BOOL Paint = FALSE;
   static BOOL Refresh = FALSE;	// flag for window refresh
   static HPS  hps;

   switch (msg) {
   case WM_CREATE: {
      if (!WinStartTimer(WinQueryAnchorBlock(hwnd), hwnd, 1, 1000)){
	 PSZ tmpMsg = new CHAR[100];
	 WinLoadString( HAB(0L), NULLHANDLE, STRG_NOTIMER, 100, 
			tmpMsg );
	 PSZ tmpTitle = new CHAR[20];
	 WinLoadString( HAB(0L), NULLHANDLE, STRG_ERROR, 20, 
			tmpTitle );

	 WinMessageBox( HWND_DESKTOP, hwnd, tmpMsg, tmpTitle, 0, 
			MB_CUACRITICAL | MB_OK );
	 delete[] tmpTitle;
	 delete[] tmpMsg;
      }
      hps = WinGetPS(hwndMain);
	 // Init sound stuff:
      CHAR file[10];  // the filenames n0.wav .. n9.wav are expected
      strcpy( file, "fe0.wav" );
      for( char i = 0; i < 10; i++ ){
	 file[2] = (CHAR)(i + '0');	// number of file
	 number[i] = new Sound( file );
      }	// for
   }
   return 0;

      /*    case WM_READ_ALL:	// read all numbers that are currently being displayed
	    {
	    for( int i = ( curtime[5] ? 5 : 4 );	
	       // if earlier than 10 o'clock, don't read leading 0
	       i >= (seconds ? 0 : 2);	// if seconds, read them
	       i-- ){
	       PlaySound( number[curtime[i]], CrazyClock.QueryfSound() );
	       DosSleep( 300 );
	       }
	       }
	       break;
	       */

   case WM_READ_ONE: {	// read the number on which has been clicked
      LONG NrToRead = LONGFROMMP( mp1 );
	 // check if number is valid and turn it into the correct index:
      if( CrazyClock.RetrieveNumberToRead( &NrToRead ) )
	 number[NrToRead]->Play();
	 // continue painting	
      Refresh = FALSE;
      Paint = TRUE;	
   }
   return 0;
			
   case WM_BEGIN_PAINT: // first message parameter determines whether
	 // the size must be recalculated (TRUE)
	 // and the whole window be painted or not
      Refresh = (BOOL) LONGFROMMP( mp1 );
      Paint = TRUE;
      return 0;
	    
   case WM_END_PAINT:
      Paint = FALSE;
      return 0;

   case WM_TIMER:
	 //	    DosEnterCritSec();
      if( Refresh )
	 CrazyClock.PaintBackground( hwndMain, hps  ); 
      if ( Paint )
	 CrazyClock.WorkPaint( hps, Refresh  );
      Refresh = FALSE;   // must be reset to FALSE for painting optimization

	 // check for hour chime:
      if( CrazyClock.CheckChime() )
	 chime.Play(); // Ding Dong

	 // check for alarm:
      if( CrazyClock.CheckAlarm() ){
	 alarm.Play();
	    
	 if( CrazyClock.QueryfAlarmWnd() )	// open modal dialog box:
	    WinDlgBox( HWND_DESKTOP, hwndFrame, AlarmMsgDlgProc,
		       (HMODULE)0, DLG_ALARMMSG, NULL );
      }
      return 0;

   case WM_FRAMEREGION:
      {
	 SIZEL FrameThickness = {
	    WinQuerySysValue( HWND_DESKTOP, SV_CXSIZEBORDER ),
	    WinQuerySysValue( HWND_DESKTOP, SV_CYSIZEBORDER ) };
	 RECTL Rectl;
	 WinQueryWindowRect( hwndFrame, &Rectl );
	 BOOL fSuccess = WinDrawBorder( hps, &Rectl, FrameThickness.cx,
					FrameThickness.cy, 0, 0, DB_STANDARD );
	    //DB_AREAATTRS ohne weiteres geht nicht					

	    //			if( !fSuccess ) wcprintf("WinDrawBorder Error!");			
	    // 			HRGN hrgn = GpiCreateRegion( hps, 1L, &Rectl );
	    //			GpiFrameRegion( hps, hrgn, &FrameThickness );
      }
   return 0;
			
   case WM_DESTROY:
      WinStopTimer(WinQueryAnchorBlock(hwnd), hwnd, 0);
      WinReleasePS(hps);
      FreeMMPMSupport();	// will do no harm if it has not been loaded
      return 0;
   }
   return WinDefWindowProc (hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY ColorDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   SHORT x, y, index = 0;
   static BOOL Change = FALSE;
   static COL_DEFS *colorptr;
   static COL_DEFS oldColors;	// save old colors in case of "cancel"
   switch( msg ){
   case WM_INITDLG:
      colorptr = (COL_DEFS *)mp2;

      oldColors.backgnd = colorptr->backgnd;
      oldColors.lighton = colorptr->lighton;
      oldColors.lightoff = colorptr->lightoff;
      oldColors.border = colorptr->border;
			
      for( y = 1; y < 6; y++ ){
	 for( x = 1; x < 4; x++ ){
	    WinSendDlgItemMsg( hwnd, COLOR_BACKGND, VM_SETITEM,
			       MPFROM2SHORT( x, y ),
			       MPFROMLONG( Colors[index]) );
	    WinSendDlgItemMsg( hwnd, COLOR_LIGHT_BORDER, VM_SETITEM,
			       MPFROM2SHORT( x, y ),
			       MPFROMLONG( Colors[index]) );
	    WinSendDlgItemMsg( hwnd, COLOR_LIGHT_ON, VM_SETITEM,
			       MPFROM2SHORT( x, y ),
			       MPFROMLONG( Colors[index]) );
	    WinSendDlgItemMsg( hwnd, COLOR_LIGHT_OFF, VM_SETITEM,
			       MPFROM2SHORT( x, y ),
			       MPFROMLONG( Colors[index]) );
	    index++;	
	 }
      }
	 // set cursors to current colors
      WinSendDlgItemMsg( hwnd, COLOR_BACKGND, VM_SELECTITEM,
			 MPFROM2SHORT( colorptr->backgnd % 3 + 1,
				       colorptr->backgnd / 3 + 1 ), NULL );
      WinSendDlgItemMsg( hwnd, COLOR_LIGHT_BORDER, VM_SELECTITEM,
			 MPFROM2SHORT( colorptr->border % 3 + 1,
				       colorptr->border / 3 + 1 ),	NULL );
      WinSendDlgItemMsg( hwnd, COLOR_LIGHT_ON, VM_SELECTITEM,
			 MPFROM2SHORT( colorptr->lighton % 3 + 1,
				       colorptr->lighton / 3 + 1 ), NULL );
      WinSendDlgItemMsg( hwnd, COLOR_LIGHT_OFF, VM_SELECTITEM,
			 MPFROM2SHORT( colorptr->lightoff % 3 + 1,
				       colorptr->lightoff / 3 + 1 ), NULL );
      break;	// WM_INITDLG

   case WM_CONTROL:	// the user clicked in the window
      switch( SHORT2FROMMP( mp1 ) ){
	 MRESULT mReply;

      case VN_SELECT:	// the others are not interesting
	 Change = TRUE;
	 switch( SHORT1FROMMP( mp1 ) ){
	 case COLOR_BACKGND:
	    mReply = WinSendDlgItemMsg( hwnd, COLOR_BACKGND,
					VM_QUERYSELECTEDITEM, 0, 0 );
	    colorptr->backgnd = ( SHORT2FROMMR(mReply) - 1 ) * 3
	       + SHORT1FROMMR(mReply) - 1;
	    break;
	 case COLOR_LIGHT_BORDER:
	    mReply = WinSendDlgItemMsg( hwnd, COLOR_LIGHT_BORDER,
					VM_QUERYSELECTEDITEM, 0, 0 );
	    colorptr->border = ( SHORT2FROMMR(mReply) - 1 ) * 3
	       + SHORT1FROMMR(mReply) - 1; 
	    break;
	 case COLOR_LIGHT_ON:
	    mReply = WinSendDlgItemMsg( hwnd, COLOR_LIGHT_ON,
					VM_QUERYSELECTEDITEM, 0, 0 );
	    colorptr->lighton = ( SHORT2FROMMR(mReply) - 1 ) * 3
	       + SHORT1FROMMR(mReply) - 1;
	    break;
	 case COLOR_LIGHT_OFF:
	    mReply = WinSendDlgItemMsg( hwnd, COLOR_LIGHT_OFF,
					VM_QUERYSELECTEDITEM, 0, 0 );
	    colorptr->lightoff = ( SHORT2FROMMR(mReply) - 1 ) * 3
	       + SHORT1FROMMR(mReply) - 1;
	    break;
	 default:
	    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
	 }
	    // notify window of the change:
	 WinSendMsg( hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0 );
	 break;	// case VN_SELECT
      default:
	 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }	// switch SHORT2...
      break;	// case WM_CONTROL
	
   case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) ){
      case DID_OK:
	 WinDismissDlg( hwnd, FALSE );	// no repainting necessary
	 return (MRESULT) 0;	// case DID_OK
      case DID_CANCEL:
	 if( Change ){
	       // reset color values if they were changed
	    colorptr->lighton = oldColors.lighton;
	    colorptr->lightoff = oldColors.lightoff;
	    colorptr->border = oldColors.border;
	    colorptr->backgnd = oldColors.backgnd;
	 }
	 WinDismissDlg( hwnd, Change );
	 return (MRESULT) 0;	 // case DID_CANCEL
      case DID_HELP:
	 WinSendMsg( hwndHelp, HM_EXT_HELP, (MPARAM)0, (MPARAM)0 );
	 return (MRESULT) 0;  // don't let this get to DefDlgProc
      default:
	 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }	// end switch
      break;	// case WM_CONTROL
			
   default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
   }	// switch( msg )
   return (MRESULT) 0;
}

MRESULT EXPENTRY AboutDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg)
   {
   case WM_COMMAND:
      WinDismissDlg(hwnd,TRUE);
      return 0;
   default:
      return WinDefDlgProc(hwnd,msg,mp1,mp2);
   }
}

MRESULT EXPENTRY SettingsDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   static BOOL Change = FALSE;
   static BOOL Oldseconds, OldFrameIsVisible, OldfReadNumbers, OldShow24h,
      OldfChime, OldfFloatToTop;
      // stores the old values for Cancel

   switch( msg ){
   case WM_INITDLG:
	 // save current values and set checkbuttons to current values
      WinCheckButton (hwnd, SET_SECONDS, Oldseconds = 
		      CrazyClock.QueryfSeconds() );
      WinCheckButton (hwnd, SET_TITLEBAR, 
		      OldFrameIsVisible = 
		      CrazyClock.QueryfFrameIsVisible() );
      WinCheckButton (hwnd, SET_24HOUR, OldShow24h = 
		      CrazyClock.QueryfShow24h() );
      WinCheckButton (hwnd, SET_CHIME, OldfChime = 
		      CrazyClock.QueryfChime() );
      WinCheckButton (hwnd, SET_READNUMBERS, OldfReadNumbers = 
		      CrazyClock.QueryfReadNumbers() );
      WinCheckButton (hwnd, SET_FLOATTOTOP, OldfFloatToTop = 
		      CrazyClock.QueryfFloatToTop() );
      break;	// WM_INITDLG

   case WM_CONTROL:	// the user clicked in the window
      switch( SHORT2FROMMP( mp1 ) ){
	    // the others are not interesting
      case BN_CLICKED:
	 Change = TRUE;
	 switch( SHORT1FROMMP( mp1 ) ){

	 case SET_SECONDS:
	    CrazyClock.TogglefSeconds();
	    break;

	 case SET_24HOUR:
	    CrazyClock.TogglefShow24h();
	    break;

	 case SET_TITLEBAR:
	    if( CrazyClock.QueryfFrameIsVisible() )  
		  // these funct.s adjust the FrameIsVisible flag themselves
	       ClkHideFrameControls ( hwndFrame );
	    else
	       ClkShowFrameControls ( hwndFrame );
	    break;

	 case SET_READNUMBERS:
	    CrazyClock.TogglefReadNumbers();
	    break;

	 case SET_CHIME:
	    CrazyClock.TogglefChime();
	    break;

	 case SET_FLOATTOTOP:
	    CrazyClock.TogglefFloatToTop();
	    break;

	 default:
	    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
	 }
	    // notify window of the change:
	    // no WM_PAINT msg is received, so we have to restart the
	    // paint thread ourselves
	 WinSendMsg( hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0 );
	 break;	// case BN_CLICKED
      default:
	 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }	// switch SHORT2...
      break;	// case WM_CONTROL
	
   case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) ){
      case DID_OK:
	 WinDismissDlg( hwnd, FALSE );	// no repainting necessary
	 return (MRESULT) 0;	// case DID_OK
      case DID_CANCEL:
	 if( Change ){
	    CrazyClock.SetfSeconds( Oldseconds );
	    CrazyClock.SetfShow24h( OldShow24h );
	    CrazyClock.SetfReadNumbers( OldfReadNumbers );
	    if (OldFrameIsVisible != CrazyClock.QueryfFrameIsVisible() ){
	       if( OldFrameIsVisible )  
		     // these functions adjust the FrameIsVisible flag
		  ClkShowFrameControls ( hwndFrame );
	       else
		  ClkHideFrameControls ( hwndFrame );
	    }
	    CrazyClock.SetfChime( OldfChime );
	       //		CrazyClock.SetfSound( OldfSound );
	    CrazyClock.SetfFloatToTop( OldfFloatToTop );
	 }
	 WinDismissDlg( hwnd, Change );  // repaint is done by caller
	 return (MRESULT) 0;	 // case DID_CANCEL			  
      case DID_HELP:
	 WinSendMsg( hwndHelp, HM_EXT_HELP, (MPARAM)0, (MPARAM)0 );
	 return (MRESULT) 0;  // don't let this get to DefDlgProc

      default:
	 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }	// end switch
      break;	// case WM_CONTROL
			
   default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
   }	// switch( msg )
   return (MRESULT) 0;
}

PCHAR achTimeArray[] = {  // for a nice time in the alarm dialog
   "00", "01", "02", "03", "04", "05", "06", "07", "08", "09",
   "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
   "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
   "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
   "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
   "50", "51", "52", "53", "54", "55", "56", "57", "58", "59" };

MRESULT EXPENTRY AlarmDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg){
   case WM_INITDLG:
      WinCheckButton( hwnd, ALM_ON, CrazyClock.QueryfAlarmOn() );
      WinCheckButton( hwnd, ALM_SOUND, !CrazyClock.QueryfAlarmWnd() );
      WinCheckButton( hwnd, ALM_WINDOW, CrazyClock.QueryfAlarmWnd() );
      WinSendDlgItemMsg( hwnd, ALM_HOUR, SPBM_SETLIMITS,
			 MPFROMLONG( 23 ), MPFROMLONG( 0 ) );
      WinSendDlgItemMsg( hwnd, ALM_MINUTE, SPBM_SETARRAY,
			 MPFROMP( achTimeArray ), MPFROMSHORT( 60 ) );
      WinSendDlgItemMsg( hwnd, ALM_SECOND, SPBM_SETARRAY,
			 MPFROMP( achTimeArray ), MPFROMSHORT( 60 ) );

      WinSendDlgItemMsg( hwnd, ALM_HOUR, SPBM_SETMASTER,
			 MPFROMHWND( 
				    WinWindowFromID( hwnd, ALM_SECOND )), 0 );
      WinSendDlgItemMsg( hwnd, ALM_MINUTE, SPBM_SETMASTER,
			 MPFROMHWND( 
				    WinWindowFromID( hwnd, ALM_SECOND )), 0 );

      WinSendDlgItemMsg( hwnd, ALM_HOUR, SPBM_SETCURRENTVALUE,
			 MPFROMLONG( CrazyClock.QueryAlarmTimeHours() ), 
			 MPFROMLONG(0) );
      WinSendDlgItemMsg( hwnd, ALM_MINUTE, SPBM_SETCURRENTVALUE,
			 MPFROMLONG( CrazyClock.QueryAlarmTimeMinutes() ), 
			 MPFROMLONG(0) );
      WinSendDlgItemMsg( hwnd, ALM_SECOND, SPBM_SETCURRENTVALUE,
			 MPFROMLONG( CrazyClock.QueryAlarmTimeSeconds() ), 
			 MPFROMLONG(0) );
      WinEnableControl( hwnd, ALM_HOUR,   CrazyClock.QueryfAlarmOn() );
      WinEnableControl( hwnd, ALM_MINUTE, CrazyClock.QueryfAlarmOn() );
      WinEnableControl( hwnd, ALM_SECOND, CrazyClock.QueryfAlarmOn() );
      WinEnableControl( hwnd, ALM_SOUND,  CrazyClock.QueryfAlarmOn() );
      WinEnableControl( hwnd, ALM_WINDOW, CrazyClock.QueryfAlarmOn() );
      WinEnableControl( hwnd, ALM_TEXT,   CrazyClock.QueryfAlarmOn() );
      break;

   case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) ){
      case DID_OK: {
	 CrazyClock.SetfAlarmWnd( WinQueryButtonCheckstate( hwnd,
							    ALM_WINDOW ) );
	 if( WinQueryButtonCheckstate( hwnd, ALM_ON ) ) {
	    LONG h, m, s;
	    WinSendDlgItemMsg( hwnd, ALM_HOUR, SPBM_QUERYVALUE,
			       MPFROMP( &h ), 
			       MPFROM2SHORT( 0, SPBQ_DONOTUPDATE) );
	    CHAR achTmp[4];
	    WinSendDlgItemMsg( hwnd, ALM_MINUTE, SPBM_QUERYVALUE,
			       MPFROMP( achTmp ), 
			       MPFROM2SHORT( 4, SPBQ_DONOTUPDATE) );
	    m = atoi(achTmp);
	    WinSendDlgItemMsg( hwnd, ALM_SECOND, SPBM_QUERYVALUE,
			       MPFROMP( achTmp ), 
			       MPFROM2SHORT( 4, SPBQ_DONOTUPDATE) );
	    s = atoi(achTmp);
	    CrazyClock.ActivateAlarm(h, m, s);
	 } else {
	    CrazyClock.DisableAlarm();
	 }
	 WinDismissDlg( hwnd, TRUE );	
      }
      return (MRESULT) 0;	// case DID_OK
      case DID_CANCEL:
	 WinDismissDlg( hwnd, TRUE );
	 return (MRESULT) 0;	 // case DID_CANCEL 
      case DID_HELP:
	 WinSendMsg( hwndHelp, HM_EXT_HELP, (MPARAM)0, (MPARAM)0 );
	 return (MRESULT) 0;  // don't let this get to DefDlgProc
      default:
	 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }	// end switch
      break;	// case WM_COMMAND

   case WM_CONTROL: {
      USHORT State;
      switch( SHORT1FROMMP( mp1) ){
	    
      case ALM_ON:       // "Alarm on" checkbox
	 switch( SHORT2FROMMP( mp1 ) ){
	 case BN_CLICKED:        // check box has been clicked upon
	    State = (USHORT)WinQueryButtonCheckstate( hwnd, ALM_ON );
	    WinEnableControl( hwnd, ALM_HOUR,   State );
	    WinEnableControl( hwnd, ALM_MINUTE, State );
	    WinEnableControl( hwnd, ALM_SECOND, State );
	    WinEnableControl( hwnd, ALM_SOUND,  State );
	    WinEnableControl( hwnd, ALM_WINDOW, State );
	    WinEnableControl( hwnd, ALM_TEXT,   State );
	    return (MRESULT) 0;  // case BN_CLICKED
	 default:  // for SHORT2FROMMP( mp1 )
	    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
	 }
	 return (MRESULT) 0;  // case ALM_ON
      default:  // for SHORT1FROMMP( mp1 )
	 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }     
   }
   return (MRESULT) 0;  // case WM_CONTROL

			
   default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
   }	// switch( msg )
   return (MRESULT) 0;
}

MRESULT EXPENTRY AlarmMsgDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg)
   {
   case WM_COMMAND:
      WinDismissDlg(hwnd,TRUE);
      return 0;
   default:
      return WinDefDlgProc(hwnd,msg,mp1,mp2);
   }
}

   /****************************************************************\
 *
 *  Name:ClkHideFrameControls
 *
 *  Purpose:Hide the title bar and associted controls
 *
\****************************************************************/
VOID ClkHideFrameControls ( HWND hwndFrame )
{

   WinSetParent ( hwndTitleBar, HWND_OBJECT, FALSE );
   WinSetParent ( hwndSysMenu,  HWND_OBJECT, FALSE );
   WinSetParent ( hwndMinMax,   HWND_OBJECT, FALSE );
   WinSetParent ( hwndMenu,     HWND_OBJECT, FALSE );

   WinSendMsg ( hwndFrame, WM_UPDATEFRAME,
		MPFROMLONG( FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX | 
			    FCF_MENU ), MPVOID) ;
   WinInvalidateRect ( hwndFrame, NULL, TRUE ) ;
      // force a repaint now!

   CrazyClock.SetfFrameIsVisible( FALSE );
}

   /****************************************************************\
 *
 *  Name:ClkShowFrameControls
 *
 *  Purpose:Show the title bar and associated contols

 *  Returns:
 *	    VOID
 *
\****************************************************************/


VOID ClkShowFrameControls ( HWND hwndFrame )
{
   WinSetParent ( hwndTitleBar, hwndFrame, FALSE );
   WinSetParent ( hwndSysMenu,  hwndFrame, FALSE );
   WinSetParent ( hwndMinMax,   hwndFrame, FALSE );
   WinSetParent ( hwndMenu,     hwndFrame, FALSE );

   WinSendMsg ( hwndFrame, WM_UPDATEFRAME,
		MPFROMLONG( FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX | 
			    FCF_MENU ), MPVOID);
   WinInvalidateRect ( hwndFrame, NULL, TRUE );

   CrazyClock.SetfFrameIsVisible( TRUE );
}


VOID InitDefault()
{
   colorset.backgnd = 13;	// position of CLR_BLACK in Colors[]
   colorset.lighton = 1; // pos. of CLR_RED;
   colorset.lightoff = 13;
   colorset.border = 13; // CLR_BLACK;
}


VOID AdditionsToSysMenu()
{
   MENUITEM mi;
   HWND hwndSysSubMenu;

   WinSendMsg( WinWindowFromID( hwndFrame, FID_SYSMENU ), MM_QUERYITEMBYPOS,
	       0L, MAKE_16BIT_POINTER(&mi) );
   hwndSysSubMenu = mi.hwndSubMenu;
   mi.iPosition = MIT_END;
   mi.afStyle = MIS_SEPARATOR;
   mi.afAttribute = 0;
   mi.id = -1;
   mi.hwndSubMenu = 0;
   mi.hItem = 0;
   WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ), NULL );
   mi.afStyle = MIS_TEXT;
   mi.id = ID_ENLARGED;
   if( CrazyClock.QueryfDotsEnlarged() ){
      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_REDUCEDOTSIZE, 50, 
		     pszMenuText );
      WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		  pszMenuText );
      delete[] pszMenuText;

   } else {

      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_ENLARGEDOTSIZE, 50, 
		     pszMenuText );
      WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		  pszMenuText );
      delete[] pszMenuText;
   }
								
   mi.afStyle = MIS_TEXT;
   mi.id = ID_ICONFRAME;
   if( CrazyClock.QueryfIconFrameIsVisible() ){
      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_HIDEICONFRAME, 50, 
		     pszMenuText );
      WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		  pszMenuText );
      delete[] pszMenuText;

   } else {

      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_SHOWICONFRAME, 50, 
		     pszMenuText );
      WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		  pszMenuText );
      delete[] pszMenuText;
   }
								
}					

VOID ToggleEnlarged()
{
   MENUITEM mi;

   WinSendMsg( WinWindowFromID( hwndFrame, FID_SYSMENU ), MM_QUERYITEMBYPOS,
	       0L, MAKE_16BIT_POINTER(&mi) );

   if( CrazyClock.QueryfDotsEnlarged() ){
      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_ENLARGEDOTSIZE, 50, 
		     pszMenuText );
      WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		  MPFROMSHORT( ID_ENLARGED ),
		  MPFROMP( pszMenuText ) );
      delete[] pszMenuText;
   } else {
      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_REDUCEDOTSIZE, 50, 
		     pszMenuText );
      WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		  MPFROMSHORT( ID_ENLARGED ),
		  MPFROMP( pszMenuText ) );
      delete[] pszMenuText;
   }

   CrazyClock.TogglefDotsEnlarged();
}


VOID ToggleIconFrame()
{
   MENUITEM mi;

   WinSendMsg( WinWindowFromID( hwndFrame, FID_SYSMENU ), MM_QUERYITEMBYPOS,
	       0L, MAKE_16BIT_POINTER(&mi) );

   if( CrazyClock.QueryfIconFrameIsVisible() ){
      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_SHOWICONFRAME, 50, 
		     pszMenuText );
      WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		  MPFROMSHORT( ID_ICONFRAME ),
		  MPFROMP( pszMenuText ) );
      delete[] pszMenuText;
   } else {
      PSZ pszMenuText = new CHAR[50];
      WinLoadString( HAB(0L), NULLHANDLE, STRG_HIDEICONFRAME, 50, 
		     pszMenuText );
      WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		  MPFROMSHORT( ID_ICONFRAME ),
		  MPFROMP( pszMenuText ) );
      delete[] pszMenuText;
   }

   CrazyClock.TogglefIconFrameIsVisible();
}

   // beeps like an annoying alarm clock
void AlarmSoundFunc()
{
   DosBeep( 500, 150 );
   DosSleep( 20 );
   DosBeep( 500, 150 );
   DosSleep( 20 );
   DosBeep( 500, 150 );
   DosSleep( 20 );
   DosBeep( 500, 150 );
   DosSleep( 350 );
   DosBeep( 500, 150 );
   DosSleep( 20 );
   DosBeep( 500, 150 );
   DosSleep( 20 );
   DosBeep( 500, 150 );
   DosSleep( 20 );
   DosBeep( 500, 150 );
}

   // plays Big Ben
void ChimeSoundFunc()
{
   DosBeep( 493, 200 ); // H
   DosSleep( 50 );
   DosBeep( 391, 200 ); // G
   DosSleep( 50 );
   DosBeep( 440, 200 ); // A
   DosSleep( 50 );
   DosBeep( 294, 200 ); // D
   DosSleep( 250 );
   DosBeep( 294, 200 ); // D
   DosSleep( 50 );
   DosBeep( 440, 200 ); // A
   DosSleep( 50 );
   DosBeep( 493, 200 ); // H
   DosSleep( 50 );
   DosBeep( 391, 200 ); // G
}
