/*************************************************************************\
 * profile.cpp
 *	Any profile (crazy.ini) related stuff is located here.
 * Set tabs to 3 to get a readable source code.
\*************************************************************************/
#define INCL_WINSHELLDATA
#define INCL_WINWINDOWMGR
#define INCL_WINSYS
#include <os2.h>
#include <string.h>

#include "mytime.h"
#include "profile.h"

// #include "..\_Standard\wcapi.h"


#define MaxVersionLen 8

struct {
	PSZ pszVersion;
	PSZ pszColors;
	PSZ pszSeconds;
	PSZ pszWinPos;		// relict from Ver. 1.0 and 1.1
	PSZ pszWinInfo;
	PSZ pszFrame;
	PSZ pszDotsEnlarged;
	PSZ pszIsMinimized;
	PSZ psz24h;
	PSZ pszIconFrame;
        PSZ pszAlarmOn;
        PSZ pszfAlarmWnd;
        PSZ pszAlarmTime;
        PSZ pszfChime;
} PrfKeys = {
	"Version",
	"Colors",
	"SecondsFlag",
	"WindowPos",			// not since 1.2
	"WindowInfo",			// since 1.2
	"FrameFlag",		// should be labelled "TitleBarFlag" now
	"DotsEnlargedFlag",	// since 1.1
	"MinimizedFlag",		// since 1.1
	"24-h-Flag",			// since 1.2
	"IconFrameFlag",		// since 1.2
	"AlarmFlag",                    // since 1.3
        "AlarmWindow",                  // since 1.3
	"AlarmTime",                    // since 1.3
	"ChimeFlag",                     // since 1.3
};

PSZ pszAppName = "Crazy Clock";
PSZ pszIniName = "crazy.ini";
PSZ pszVersion = "1.3.0";



/*************************************************************************\
 * function StoreWindowPos
 * This function is meant as a replacement for the os/2 library function
 * WinStoreWindowPos. It stores the current size and postion of the window
 * hwnd (window handle) to the profile hini (profile handle).
 * I use my own function because the original always stores it's
 * information in os2.ini which slows down system performace. 
 * Note: The function WinStoreWindowPos does a bit more. It also saves
 * the presentation parameters (i.e. color, fonts etc) of the window.
 * Because I don't need this it's not included here. (It would be a lot of
 * stupid work.)
 * Like the original this function returns TRUE if it was successful and
 * FALSE otherwise.
 *
 * The Handle passed is the frame window handle
\*************************************************************************/
BOOL StoreWindowPos(	const HINI hini, const PSZ pszAppName,
							const PSZ pszKeyName, const HWND hwnd )
{
	USHORT usWinInfo[6];
	
	// retrieve the position and width for the restored window
	// and the position of the minimized window

	SWP swp;
	WinQueryWindowPos( hwnd, &swp );
	if( (swp.fl & SWP_MINIMIZE) || (swp.fl & SWP_MAXIMIZE) ){
		// window is minimized or maximized, so use the window words
		usWinInfo[0] = WinQueryWindowUShort( hwnd, QWS_XRESTORE );
		usWinInfo[1] = WinQueryWindowUShort( hwnd, QWS_YRESTORE );
		usWinInfo[2] = WinQueryWindowUShort( hwnd, QWS_CXRESTORE );
		usWinInfo[3] = WinQueryWindowUShort( hwnd, QWS_CYRESTORE );
	} else {
		// use the current window position since the window words don't
		// contain the correct information right now
		usWinInfo[0] = swp.x;
		usWinInfo[1] = swp.y;
		usWinInfo[2] = swp.cx;
		usWinInfo[3] = swp.cy;
	}	

	usWinInfo[4] = WinQueryWindowUShort( hwnd, QWS_XMINIMIZE );
	usWinInfo[5] = WinQueryWindowUShort( hwnd, QWS_YMINIMIZE );

	/* write these values only */
	if ( !PrfWriteProfileData( hini, pszAppName, pszKeyName,
										usWinInfo, 6 * sizeof( USHORT ) )
		) return FALSE;
	return TRUE;
}

/*************************************************************************\
 * function RestoreWindowPos
 * Analoguous to StoreWindowPos but the contrary action.
\*************************************************************************/
BOOL RestoreWindowPos( const HINI hini, const PSZ pszAppName,
										const PSZ pszKeyName, const HWND hwnd )
{
	USHORT usWinInfo[6];
	ULONG ulDataLen = 6 * sizeof( USHORT );
	LONG cxScreen = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
	LONG cyScreen = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
	BOOL bTmp;
	ULONG lNumRead;

	/* Overwrite the values of the frame size / position stuff */
	if ( !PrfQueryProfileData( hini, pszAppName, pszKeyName,
										usWinInfo, &ulDataLen )
		) return FALSE;
		
	if( usWinInfo[0] + usWinInfo[2] >= cxScreen ){	// wrong position
		if( usWinInfo[2] < cxScreen - 1 )
			usWinInfo[0] = cxScreen - usWinInfo[2] - 1;
		else {
			usWinInfo[0] = cxScreen / 4;	// move the window to the lower left quarter
			if( usWinInfo[0] + usWinInfo[2] >= cxScreen ){	// still too big
				usWinInfo[2] = cxScreen - usWinInfo[0] - 10;	// adjust window width
			}
		}
	}
	if( usWinInfo[1] + usWinInfo[3] >= cyScreen ){	// wrong position
		if( usWinInfo[3] < cyScreen - 1 )	// width is okay
			usWinInfo[1] = cyScreen - usWinInfo[3] - 1;
		else {
			usWinInfo[1] = cyScreen / 4;	// move the window to the lower left quarter
			if( usWinInfo[1] + usWinInfo[3] >= cyScreen ){	// still too big
				usWinInfo[3] = cyScreen - usWinInfo[1] - 10;	// adjust window width
			}
		}
	}

	// set new positions / sizes
	WinSetWindowUShort( hwnd, QWS_XRESTORE, usWinInfo[0] );
	WinSetWindowUShort( hwnd, QWS_YRESTORE, usWinInfo[1] );
	WinSetWindowUShort( hwnd, QWS_CXRESTORE, usWinInfo[2] );
	WinSetWindowUShort( hwnd, QWS_CYRESTORE, usWinInfo[3] );
	
	// adjust minimized positions
	WinSetWindowUShort( hwnd, QWS_XMINIMIZE, usWinInfo[4] );
	WinSetWindowUShort( hwnd, QWS_YMINIMIZE, usWinInfo[5] );
		
	/* Set new position */
	WinSetWindowPos( hwnd, NULLHANDLE,
		  usWinInfo[0], usWinInfo[1], usWinInfo[2], usWinInfo[3],
		  SWP_SIZE | SWP_MOVE );

	// retrieve IsMinimized flag
	lNumRead = sizeof( BOOL );		// ULONG!
	if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszIsMinimized,
		 &bTmp, &lNumRead ) )
	{
		if( bTmp ){
			// set window to be minimized
			WinSetWindowPos( hwndFrame, 0L, 0L, 0L, 0L, 0L, SWP_MINIMIZE );
		}
	}

	return TRUE;
}



/*************************************************************************\
 * function ReadProfile()
 * Tries to open the file "connect4.ini" and sets game specific options taken
 * from the profile data. If the file isn't found or the data is invalid
 * FALSE is returned otherwise the function returns TRUE.
\*************************************************************************/
BOOL ReadProfile( HAB hab )
{
    CHAR pszVersionFound[MaxVersionLen] = "";
    ULONG lNumRead;
    HINI hini;
    PSZ pszIniNameCopy;
    BOOL bTmp;
    COL_DEFS colTmp;
    DATETIME dtTmp;


    pszIniNameCopy = new CHAR[ strlen( pszIniName ) + 1];
    strcpy( pszIniNameCopy, pszIniName );
    if ( !(hini = PrfOpenProfile( hab, pszIniNameCopy )) ){
	PrfCloseProfile( hini );
	return FALSE;
    }

    PrfQueryProfileString( hini, pszAppName, PrfKeys.pszVersion, NULL,
			   pszVersionFound, MaxVersionLen );
    if ( !strcmp( "1.1.0", pszVersionFound ) ||
    !strcmp( "1.0.0", pszVersionFound ) ){		
	// profile was created with Ver. 1.0.0 or 1.1.0
	// delete the pszWinPos entry since it is replaced by pszWinInfo
	PrfWriteProfileData( hini, pszAppName, PrfKeys.pszWinPos, NULL, 0 );
    }

    // retrieve color settings
    lNumRead = sizeof( COL_DEFS );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszColors,
			     (PVOID)&colTmp, &lNumRead ) ){
	// success; copy retrieved values into the appropriate variables
	colorset.backgnd = colTmp.backgnd;
	colorset.lighton = colTmp.lighton;
	colorset.lightoff = colTmp.lightoff;
	colorset.border = colTmp.border;
    }

    // retrieve frame flag
    lNumRead = sizeof( BOOL );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszFrame,
			     &bTmp, &lNumRead ) )
	FrameIsVisible = bTmp;

    // retrieve dotsEnlarged flag
    lNumRead = sizeof( BOOL );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszDotsEnlarged,
			     &bTmp, &lNumRead ) )
	DotsEnlarged = bTmp;

    // retrieve Seconds flag
    lNumRead = sizeof( BOOL );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszSeconds,
			     &bTmp, &lNumRead ) )
	seconds = bTmp;

    // retrieve 24h flag
    lNumRead = sizeof( BOOL );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.psz24h,
			     &bTmp, &lNumRead ) )
	Show24h = bTmp;

    // retrieve AlarmOn flag
    lNumRead = sizeof( BOOL );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszAlarmOn,
			     &bTmp, &lNumRead ) )
	AlarmOn = bTmp;

    // retrieve fAlarmWnd flag
    lNumRead = sizeof( BOOL );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszfAlarmWnd,
			     &bTmp, &lNumRead ) )
	fAlarmWnd = bTmp;

    // retrieve AlarmTime 
    lNumRead = sizeof( DATETIME );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszAlarmTime,
			     &dtTmp, &lNumRead ) ){
	AlarmTime.hours = dtTmp.hours;
	AlarmTime.minutes = dtTmp.minutes;
	AlarmTime.seconds = dtTmp.seconds;
    }

    // retrieve fChime flag
    lNumRead = sizeof( BOOL );
    if( PrfQueryProfileData( hini, pszAppName, PrfKeys.pszfChime,
			     &bTmp, &lNumRead ) )
	fChime = bTmp;

    RestoreWindowPos( hini, pszAppName, PrfKeys.pszWinInfo, hwndFrame );

    PrfCloseProfile( hini );
    return TRUE;
}

/*************************************************************************\
 * function WriteProfile()
 * Trys to open the file "profile.ini" and saves game specific options 
 * from the profile data. 
\*************************************************************************/
BOOL WriteProfile( HAB hab )
{
    HINI hini;
    PSZ pszIniNameCopy;
    BOOL bTmp;


    pszIniNameCopy = new CHAR[ strlen( pszIniName ) + 1];
    strcpy( pszIniNameCopy, pszIniName );
    if ( !(hini = PrfOpenProfile( hab, pszIniNameCopy )) ){
	PrfCloseProfile( hini );
	return FALSE;
    }
	
    PrfWriteProfileString( hini, pszAppName,
			   PrfKeys.pszVersion, pszVersion );

    // write color settings
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszColors,
			 (PVOID)&colorset, (ULONG)sizeof( COL_DEFS ) );

    // write frame flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszFrame,
			 &FrameIsVisible, (ULONG)sizeof( BOOL ) );

    // write dotsEnlarged flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszDotsEnlarged,
			 &DotsEnlarged, (ULONG)sizeof( BOOL ) );

    // write IsMinimized flag
    bTmp = WinQueryWindowULong( hwndFrame, QWL_STYLE ) & WS_MINIMIZED;
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszIsMinimized,
			 &bTmp, (ULONG)sizeof( BOOL ) );

    // write Seconds flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszSeconds,
			 &seconds, sizeof( BOOL ) );

    // write 24h flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.psz24h,
			 &Show24h, sizeof( BOOL ) );

    // write IconFrame flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszIconFrame,
			 &IconFrameIsVisible, sizeof( BOOL ) );

    // write AlarmOn flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszAlarmOn,
			 &AlarmOn, (ULONG)sizeof( BOOL ) );

    // write fAlarmWnd flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszfAlarmWnd,
			 &fAlarmWnd, (ULONG)sizeof( BOOL ) );

    // write Alarm time
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszAlarmTime,
			 (PVOID)&AlarmTime, (ULONG)sizeof( DATETIME ) );

    // write fChime flag
    PrfWriteProfileData( hini, pszAppName, PrfKeys.pszfChime,
			 &fChime, (ULONG)sizeof( BOOL ) );

    StoreWindowPos( hini, pszAppName, PrfKeys.pszWinInfo, hwndFrame );

    PrfCloseProfile( hini );
    return TRUE;
}






