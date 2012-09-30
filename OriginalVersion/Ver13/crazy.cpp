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
#include "crazy.h"
#include "profile.h"

#include "mmsound.h"


// #include "..\_Standard\wcapi.h"
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

BOOL seconds;			// TRUE : displays seconds
BOOL FrameIsVisible;	// TRUE : Title Bar etc. are visible
BOOL DotsEnlarged;	// TRUE : dots on minimized program are painted elliptic
BOOL fReadNumbers;	// not to be stored in the .INI file!
BOOL fSound = FALSE;	// indicates whether sound is played via MMPM
BOOL Show24h = TRUE;	// TRUE : time is displayed in 24-h-mode
BOOL IconFrameIsVisible = TRUE;	// TRUE : frame is drawn around the min. prog.
BOOL AlarmOn = FALSE;   // indicates whether alarm has been activated
BOOL fAlarmWnd = FALSE;  // i. w. alarm should open a window
BOOL fChime = FALSE;   // indicates whether chime has been activated
DATETIME AlarmTime;     // holds the alarm time

// old frame procedure before subclassing
PFNWP OldFrameProc;

// sound variables; must not call constructors before MMPM-Support is loaded
mem_wav  *number[10];
mem_wav  *alarm, *chime;

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
    ULONG   flFrameFlags =  FCF_TITLEBAR      |     FCF_SYSMENU  |
	FCF_SIZEBORDER    |     FCF_MINMAX   |
	FCF_TASKLIST 		 |		 FCF_MENU		|
	FCF_ACCELTABLE; //    | FCF_AUTOICON;


    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);

    WinRegisterClass (hab, pszAppName, MainWndProc, CS_SIZEREDRAW |
		      CS_MOVENOTIFY, 0);
    // frame sends a WM_MOVE message to client

    hwndFrame = WinCreateStdWindow (HWND_DESKTOP, 0,
				    &flFrameFlags, pszAppName, NULL,
				    0, NULLHANDLE, IDR_MAIN, &hwndMain);

    hwndTitleBar = WinWindowFromID ( hwndFrame , FID_TITLEBAR ) ;
    hwndSysMenu = WinWindowFromID ( hwndFrame , FID_SYSMENU ) ;
    hwndMinMax = WinWindowFromID ( hwndFrame , FID_MINMAX ) ;
    hwndMenu = WinWindowFromID ( hwndFrame , FID_MENU ) ;

    hwndPopupMenu = WinLoadMenu( HWND_OBJECT, 0, IDR_POPUP );


    // set "Crazy Clock" as window title, even when started
    // from the command line
    WinSetWindowText( hwndFrame, pszAppName );

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
    ReadProfile( hab );

    
    AdditionsToSysMenu();	// first DotsEnlarged must be set, then
// the appropriate system menu entry can be added

    InitNumbers();

    if( seconds ){	// add checkmark to menu entry of both menues
	WinSendMsg( hwndMenu, MM_SETITEMATTR,
		    MPFROM2SHORT( ID_SECONDS, TRUE ),	// searches in submenus
		    MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );
	WinSendMsg( hwndPopupMenu, MM_SETITEMATTR,
		    MPFROM2SHORT( ID_SECONDS, TRUE ),	// searches in submenus
		    MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );
    }

    if( !FrameIsVisible ){	// hide frame, adjust menu entry text
	if( ! (WinQueryWindowULong( hwndFrame, QWL_STYLE ) & WS_MINIMIZED) )
	    ClkHideFrameControls ( hwndFrame );		// only if prog is not minimized
	WinSendMsg( hwndMenu, MM_SETITEMTEXT,	// change menu text
		    MPFROMSHORT( ID_FRAME ),
		    MPFROMP( "Show ~Title Bar" ) );
	WinSendMsg( hwndPopupMenu, MM_SETITEMTEXT,	// change menu text
		    MPFROMSHORT( ID_FRAME ),
		    MPFROMP( "Show ~Title Bar" ) );
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
    hiInit.pszHelpWindowTitle = "Crazy Clock - Help";
    hiInit.fShowPanelId = CMIC_HIDE_PANEL_ID;
    hiInit.pszHelpLibraryName = "crazy.hlp";
    hwndHelp = WinCreateHelpInstance( hab, &hiInit );
    if( hwndHelp != NULLHANDLE ){
	if( hiInit.ulReturnCode != 0 ){	// error
	    WinDestroyHelpInstance( hwndHelp );
	    hwndHelp = NULLHANDLE;
	} else { // everything o.k.
	    WinAssociateHelpInstance( hwndHelp, hwndFrame );
	}
    }
    if( hwndHelp == NULLHANDLE )	// is not an else case because hwndHelp may
// have been changed in the if above
	WinMessageBox( HWND_DESKTOP, hwndFrame,
		       "The help instance could not be created. Help will be disabled.",
		       pszAppName, 0, MB_OK | MB_CRITICAL );

// now we can make the window visible
    WinSetWindowPos( hwndFrame, 0L, 0L, 0L, 0L, 0L, SWP_SHOW );

    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
	WinDispatchMsg (hab, &qmsg);
    // WM_QUIT has been received, now clean up:

    WriteProfile( hab );

    if( hwndHelp != NULLHANDLE ){
	WinAssociateHelpInstance( NULLHANDLE, hwndFrame );
	WinDestroyHelpInstance( hwndHelp );
	hwndHelp = NULLHANDLE;
    }

    WinDestroyWindow (hwndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);
    return 0;
}

MRESULT EXPENTRY NewFrameProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
switch( msg ){
/*		case WM_PAINT:
if( WinQueryWindowULong( hwnd, QWL_STYLE ) & WS_MINIMIZED ){
HPS hps = WinBeginPaint( hwnd, 0, 0 );
WinEndPaint( hps );
WinBroadcastMsg( hwnd, msg, mp1, mp2,
BMSG_POST | BMSG_DESCENDANTS );
//				WinPostMsg( hwndMain, msg, mp1, mp2 );
return (MRESULT)FALSE;	// to indicate that painting is done
}
break;
*/

case WM_QUERYTRACKINFO:
{
    // let standard procedure initialize all
	   MRESULT mReturn = (*OldFrameProc)(hwnd, msg, mp1, mp2 );
    PTRACKINFO pti = (PTRACKINFO)mp2;
    // adjust the interesting values
	   pti->ptlMinTrackSize.x = 6 * 6;
    pti->ptlMinTrackSize.y = ( seconds ? 5 : 4 ) * 3 * 2;
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
	    WinMessageBox (HWND_DESKTOP, hwnd,
			   "Creation of second thread failed!", pszAppName,
			   0, MB_OK | MB_CUACRITICAL);
	    return 0;
	}
	colorset.cb = sizeof( COL_DEFS );
	return 0;

    case WM_ACK:	// is only received at the very start
	WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
	return 0;

    case WM_SIZE:	// each WM_SIZE is followed by a WM_PAINT msg, so
				// I can reduce the amount of work by skipping this one
//			WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
	return 0;

    case WM_MOVE:		// parent is moved	
    case WM_PAINT:
//			WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
	WinPostMsg(hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0);
	break;
//			return 0;		// very dangerous!!

    case WM_ERASEBACKGROUND:
	// if window is minimized and frame visible, let OS paint the
	// background:
	if( (WinQueryWindowULong( hwndFrame, QWL_STYLE )  & WS_MINIMIZED)
	    && IconFrameIsVisible ){
	    WinSendMsg( hwndWorker, WM_FRAMEREGION, 0, 0 );
	}
	return (MRESULT) FALSE;		// otherwise we do the painting ourselves

    case WM_CONTEXTMENU:
	{
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
	    if( !fReadNumbers ) return 0;
	    POINTL ptl;
	    WinSendMsg( hwndWorker, WM_END_PAINT, 0, 0 );	// stop clock
	    ptl.x = SHORT1FROMMP( mp1 );
	    ptl.y = SHORT2FROMMP( mp1 );
	    WinPostMsg( hwndWorker, WM_READ_ONE,
			MPFROMLONG( PositionToRead( ptl, seconds )), 0 );
	}
	break;
			
    case WM_BUTTON1MOTIONSTART:	// window is dragged
	if( FrameIsVisible ) break;	// do nothing when frame is visible
	WinPostMsg( hwndFrame, WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), 0	);
	break;	// message processed

    case WM_FOCUSCHANGE:
	if( IconFrameIsVisible) break;	// not necessary if frame is visible
	WinPostMsg( hwndWorker, WM_BEGIN_PAINT,
		    MPFROMLONG( WinQueryWindowULong( hwndFrame, QWL_STYLE )
				& WS_MINIMIZED ),
				// repaint only when minimized
		    0);
	break;	

//			the program is no longer forced to the desktop		
    case WM_MINMAXFRAME:		// the window is being minimized, maximized
				// or restored
	{
	    SWP *swp = (SWP *)mp1;

	    if( !( swp->fl & SWP_MINIMIZE ) ){	// window is not to be minimized,
				// i. e. window is being restored or maximized
		if( WinQueryWindowULong( hwndFrame, QWL_STYLE ) & WS_MINIMIZED ){
				// window is minimized right now
		    if( !FrameIsVisible ){	// controls must be hidden
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
	    //	WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
	    FirstPaint = WinLoadDlg( HWND_DESKTOP, hwndMain, ColorDlgProc,
				     (HMODULE)0, DLG_COLORS, (VOID *)&colorset );
	    WinPostMsg( hwndWorker, WM_BEGIN_PAINT,
			MPFROMLONG( FirstPaint ), 0);
	    // here a WM_PAINT msg is only received when the window
	    // was covered by the dialog box, so it is safer to post
	    // the begin-paint msg, too
	}
	    break;
	case ID_SETTINGS: {
	    BOOL FirstPaint;
//		    WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
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
//		    WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
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

	    CalcMinWindowSize( seconds, &Width, &Height );
	    WinQueryWindowRect( hwnd, &RectInner );
	    WinQueryWindowRect( hwndFrame, &RectOuter );
	    RectInner.xRight = Width + RectOuter.xRight - RectInner.xRight;
	    RectInner.yTop = Height + RectOuter.yTop - RectInner.yTop;
	    WinQueryWindowRect( hwndFrame, &RectOuter );
	    WinMapWindowPoints( hwndFrame, HWND_DESKTOP, (PPOINTL)(&RectOuter), 2 );
	    if( RectOuter.xLeft < 0 ) RectOuter.xLeft = 0;
	    if( RectOuter.yBottom < 0 ) RectOuter.yBottom = 0;
	    WinSetWindowPos( hwndFrame, NULLHANDLE, RectOuter.xLeft,
			     RectOuter.yBottom, RectInner.xRight,
			     RectInner.yTop,
			     SWP_SIZE | SWP_MOVE );
				// check whether menu entry doubling has affected the size
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
	    if( !FrameIsVisible ){	// frame controls must be reassigned to
				// the window
		ClkShowFrameControls ( hwndFrame );
		FrameIsVisible = FALSE;		// correct the flag
	    }	// kleine Transplantation
	    WinSetWindowPos( hwndFrame, 0L, 0L, 0L, 0L,
			     0L, SWP_MINIMIZE | SWP_SHOW );
	    break;

/*				case ID_READ:	// read the numbers that are currently being
				// displayed
				WinPostMsg( hwndWorker, WM_READ, 0, 0 );
				break;
				*/
	case ID_EXIT:
//				wcprintf("WM_CLOSE wird geschickt");
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
	WinRegisterClass(hab, "Crazy Clock B", WorkWndProc, 0, 0);
	hwndWorker = WinCreateWindow( HWND_OBJECT, "Crazy Clock B", "",
			 0, 0, 0, 0, 0, HWND_OBJECT, HWND_BOTTOM, 0, NULL, NULL );

	WinSendMsg( hwndMain, WM_ACK, 0, 0 );

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
	static INT curtime[6];	// to keep the time for reading
    case WM_CREATE:
	    if (!WinStartTimer(WinQueryAnchorBlock(hwnd), hwnd, 1, 1000))
		WinMessageBox(HWND_DESKTOP,hwnd,"Could not start timer!",
			      "Error",0,MB_CUACRITICAL | MB_OK);
	hps = WinGetPS(hwndMain);
	// Init sound stuff:
				// LoadMMPMSupport returns FALSE on success
	if( !LoadMMPMSupport() ){
	    CHAR error = 0, i;
	    CHAR file[10];  // the filenames n0.wav .. n9.wav are expected
	    strcpy( file, "fe0.wav" );
	    for( i = 0; i < 10 && !error; i++ ){
		file[2] = i + '0';	// number of file
		number[i] = new mem_wav( file, &error );
	    }	// for

	    if( !error ){
		strcpy( file, "alarm.wav" );
		alarm = new mem_wav( file, &error );
	    }
	    if( !error ){
		strcpy( file, "chime.wav" );
		chime = new mem_wav( file, &error );
	    }
	    // error handler:
	    if( error ){
		for( i = 0; i < 10; i++ ) 
		    if( number[i] != NULL ) delete number[i];
		if( alarm != NULL ) delete alarm;
		if( chime != NULL ) delete chime;
	    } else 
		fSound = TRUE;  // no error occurred, play sounds via MMPM
	}  // if( !LoadMMPMSupport() )
	return 0;

    case WM_READ:	// read all numbers that are currently being displayed
	    {
		for( int i = ( curtime[5] ? 5 : 4 );	
		    // if earlier than 10 o'clock, don't read leading 0
		    i >= (seconds ? 0 : 2);	// if seconds, read them
		    i-- ){
		    PlaySound( number[curtime[i]], fSound );
		    DosSleep( 300 );
		}
	    }
	break;

    case WM_READ_ONE:	// read the number on which has been clicked
	    {
		LONG NrToRead = LONGFROMMP( mp1 );
		if( NrToRead < 6 )	// check whether mouseclick was valid
		    if( NrToRead != 5 || curtime[5] )	// don't read leading 0
				// if it's earlier than 10 o'clock
			PlaySound( number[ curtime[ NrToRead ] ], fSound );
				// continue painting	
		Refresh = FALSE;
		Paint = TRUE;	
	    }
	return 0;
			
    case WM_ALARM:
	    if( fSound )
		PlaySound( alarm, fSound );
	    else {
		DosBeep( 500, 150 );
	    }
	if( fAlarmWnd ) 	// open modal dialog box:
	    WinDlgBox( HWND_DESKTOP, hwndFrame, AlarmMsgDlgProc,
		       (HMODULE)0, DLG_ALARMMSG, NULL );
	return 0;

    case WM_BEGIN_PAINT:	// first message parameter determines whether
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
		InitSpace( hwndMain, hps, seconds, IconFrameIsVisible ); 
	if ( Paint )
	    WorkPaint( hps, Refresh, seconds, curtime, Show24h );
	Refresh = FALSE;   // must be reset to FALSE for painting optimization
	if( AlarmOn )
	    CheckAlarm( hwnd, curtime, AlarmTime );
	if( fChime ){
	    // do we have any hours or minutes?
	    if( !curtime[0] && !curtime[1] && !curtime[2] && !curtime[3] )
		PlaySound( chime, fSound );		// Ding Dong
	}
//	DosExitCritSec();
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
	OldfChime;
    // stores the old values for Cancel

    switch( msg ){
    case WM_INITDLG:
	// save current values and set checkbuttons to current values
	WinCheckButton (hwnd, SET_SECONDS, Oldseconds = seconds );
	WinCheckButton (hwnd, SET_TITLEBAR, 
			OldFrameIsVisible = FrameIsVisible);
	WinCheckButton (hwnd, SET_READNUMBERS, OldfReadNumbers = fReadNumbers);
	WinCheckButton (hwnd, SET_24HOUR, OldShow24h = Show24h);
	WinCheckButton (hwnd, SET_CHIME, OldfChime = fChime);
	break;	// WM_INITDLG

    case WM_CONTROL:	// the user clicked in the window
	switch( SHORT2FROMMP( mp1 ) ){
	    // the others are not interesting
	case BN_CLICKED:
		Change = TRUE;
//	    WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
	    switch( SHORT1FROMMP( mp1 ) ){

	    case SET_SECONDS:
		seconds = !seconds;
		WinCheckButton (hwnd, SET_SECONDS, seconds );
		// no WM_PAINT msg is received, so we have to restart the
   		// paint thread ourselves
		break;

	    case SET_24HOUR:
		Show24h = !Show24h;
		break;

	    case SET_TITLEBAR:
		if( FrameIsVisible )  
		    // these functions adjust the FrameIsVisible flag
		    ClkHideFrameControls ( hwndFrame );
		else
		    ClkShowFrameControls ( hwndFrame );
		break;

	    case SET_READNUMBERS:
		fReadNumbers = !fReadNumbers;
		break;

	    case SET_CHIME:
		fChime = !fChime;
		break;

	    default:
		return WinDefDlgProc( hwnd, msg, mp1, mp2 );
	    }
				// notify window of the change:
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
//		WinSendMsg(hwndWorker, WM_END_PAINT, 0, 0);
		seconds = Oldseconds;
		Show24h = OldShow24h;
		fReadNumbers = OldfReadNumbers;
		if (OldFrameIsVisible != FrameIsVisible ){
		    if( OldFrameIsVisible )  
			// these functions adjust the FrameIsVisible flag
			ClkShowFrameControls ( hwndFrame );
		    else
			ClkHideFrameControls ( hwndFrame );
		}
		fChime = OldfChime;
//		WinSendMsg( hwndWorker, WM_BEGIN_PAINT, MPFROMLONG(1), 0 );
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

PCHAR achTimeArray[] = {  // for a nice time
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
	WinCheckButton( hwnd, ALM_ON, AlarmOn );
	WinCheckButton( hwnd, ALM_SOUND, !fAlarmWnd );
	WinCheckButton( hwnd, ALM_WINDOW, fAlarmWnd );
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
			   MPFROMLONG( AlarmTime.hours ), MPFROMLONG(0) );
	WinSendDlgItemMsg( hwnd, ALM_MINUTE, SPBM_SETCURRENTVALUE,
			   MPFROMLONG( AlarmTime.minutes ), MPFROMLONG(0) );
	WinSendDlgItemMsg( hwnd, ALM_SECOND, SPBM_SETCURRENTVALUE,
			   MPFROMLONG( AlarmTime.seconds ), MPFROMLONG(0) );
	WinEnableControl( hwnd, ALM_HOUR, AlarmOn );
	WinEnableControl( hwnd, ALM_MINUTE, AlarmOn );
	WinEnableControl( hwnd, ALM_SECOND, AlarmOn );
	WinEnableControl( hwnd, ALM_SOUND, AlarmOn );
	WinEnableControl( hwnd, ALM_WINDOW, AlarmOn );
	WinEnableControl( hwnd, ALM_TEXT, AlarmOn );
	break;

    case WM_COMMAND:
	switch( SHORT1FROMMP( mp1 ) ){
	case DID_OK: {
	    LONG tmp;
	    AlarmOn = WinQueryButtonCheckstate( hwnd, ALM_ON );
	    fAlarmWnd = WinQueryButtonCheckstate( hwnd, ALM_WINDOW );
	    WinSendDlgItemMsg( hwnd, ALM_HOUR, SPBM_QUERYVALUE,
                           MPFROMP( &tmp ), 
			       MPFROM2SHORT( 0, SPBQ_DONOTUPDATE) );
	    AlarmTime.hours = (UCHAR)tmp;
	    CHAR achTmp[4];
	    WinSendDlgItemMsg( hwnd, ALM_MINUTE, SPBM_QUERYVALUE,
                           MPFROMP( achTmp ), 
			       MPFROM2SHORT( 4, SPBQ_DONOTUPDATE) );
	    AlarmTime.minutes = (UCHAR)atoi(achTmp);
	    WinSendDlgItemMsg( hwnd, ALM_SECOND, SPBM_QUERYVALUE,
                           MPFROMP( achTmp ), 
			       MPFROM2SHORT( 4, SPBQ_DONOTUPDATE) );
	    AlarmTime.seconds = (UCHAR)atoi(achTmp);
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
		State = WinQueryButtonCheckstate( hwnd, ALM_ON );
		WinEnableControl( hwnd, ALM_HOUR, State );
		WinEnableControl( hwnd, ALM_MINUTE, State );
		WinEnableControl( hwnd, ALM_SECOND, State );
		WinEnableControl( hwnd, ALM_SOUND, State );
		WinEnableControl( hwnd, ALM_WINDOW, State );
		WinEnableControl( hwnd, ALM_TEXT, State );
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

    WinSetParent ( hwndTitleBar , HWND_OBJECT , FALSE ) ;
    WinSetParent ( hwndSysMenu , HWND_OBJECT , FALSE ) ;
    WinSetParent ( hwndMinMax , HWND_OBJECT , FALSE ) ;
    WinSetParent ( hwndMenu , HWND_OBJECT , FALSE ) ;

    WinSendMsg ( hwndFrame , WM_UPDATEFRAME ,
		 MPFROMLONG( FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX | FCF_MENU ) ,
		 MPVOID) ;
    WinInvalidateRect ( hwndFrame , NULL , TRUE ) ;
    	// force a repaint now!

	FrameIsVisible = FALSE;
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
    WinSetParent ( hwndTitleBar , hwndFrame , FALSE ) ;
    WinSetParent ( hwndSysMenu , hwndFrame , FALSE ) ;
    WinSetParent ( hwndMinMax , hwndFrame , FALSE ) ;
    WinSetParent ( hwndMenu , hwndFrame , FALSE ) ;

    WinSendMsg ( hwndFrame , WM_UPDATEFRAME ,
		 MPFROMLONG( FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX | FCF_MENU ) ,
		 MPVOID) ;
    WinInvalidateRect ( hwndFrame , NULL , TRUE ) ;

    FrameIsVisible = TRUE;
}



VOID InitDefault()
{
    colorset.backgnd = 13;	// position of CLR_BLACK in Colors[]
    colorset.lighton = 1; // pos. of CLR_RED;
    colorset.lightoff = 13;
    colorset.border = 13; // CLR_BLACK;
    FrameIsVisible = FALSE;
    DotsEnlarged = FALSE;
    seconds = TRUE;
    fReadNumbers = FALSE;		// will always be false on startup
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
    if( DotsEnlarged )
	WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		    "Reduce ~Dot Size");
    else
	WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		    "Enlarge ~Dot Size");
								
    mi.afStyle = MIS_TEXT;
    mi.id = ID_ICONFRAME;
    if( IconFrameIsVisible )
	WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		    "Hide ~Icon Frame");
    else
	WinSendMsg( hwndSysSubMenu, MM_INSERTITEM, MPFROMP( &mi ),
		    "Show ~Icon Frame");
								
}					

VOID ToggleEnlarged()
{
    MENUITEM mi;

    WinSendMsg( WinWindowFromID( hwndFrame, FID_SYSMENU ), MM_QUERYITEMBYPOS,
		0L, MAKE_16BIT_POINTER(&mi) );

    if( DotsEnlarged ){
	WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		    MPFROMSHORT( ID_ENLARGED ),
		    MPFROMP( "Enlarge ~Dot Size" ) );
    	DotsEnlarged = FALSE;				
    } else {
	WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		    MPFROMSHORT( ID_ENLARGED ),
		    MPFROMP( "Reduce ~Dot Size" ) );
	DotsEnlarged = TRUE;
    }
}


VOID ToggleIconFrame()
{
    MENUITEM mi;

    WinSendMsg( WinWindowFromID( hwndFrame, FID_SYSMENU ), MM_QUERYITEMBYPOS,
		0L, MAKE_16BIT_POINTER(&mi) );

    if( IconFrameIsVisible ){
	WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		    MPFROMSHORT( ID_ICONFRAME ),
		    MPFROMP( "Show ~Icon Frame" ) );
    	IconFrameIsVisible = FALSE;				
    } else {
	WinSendMsg( mi.hwndSubMenu, MM_SETITEMTEXT,	// change menu text
		    MPFROMSHORT( ID_ICONFRAME ),
		    MPFROMP( "Hide ~Icon Frame" ) );
	IconFrameIsVisible = TRUE;
    }
}

