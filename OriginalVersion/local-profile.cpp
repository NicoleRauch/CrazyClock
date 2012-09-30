//////////////////////////////////////////////////////////////
//
//  local-profile.cpp
//  This file provides the functions that are based on the
//  standard profile functions (which will hopefully go into a DLL
//  some day ;-).
//
//////////////////////////////////////////////////////////////

#define INCL_GPI
#include <os2.h>
#include "mytime.h"

#include "local-profile.h"


struct {
    PSZ pszColors;
    PSZ pszSeconds;
    PSZ pszWindowPos;       // remainder from ver. 1.2
    PSZ pszFrame;
    PSZ pszDotsEnlarged;
    PSZ psz24h;
    PSZ pszIconFrame;
    PSZ pszAlarmOn;
    PSZ pszfAlarmWnd;
    PSZ pszAlarmTime;
    PSZ pszfChime;      
    PSZ pszfFloatToTop;
} PrfKeys = {
    "Colors",
    "SecondsFlag",
    "WindowPos",                    // not since 1.2
    "FrameFlag",            // should be labelled "TitleBarFlag" now
    "DotsEnlargedFlag",     // since 1.1
    "24-h-Flag",                    // since 1.2
    "IconFrameFlag",                // since 1.2
    "AlarmFlag",                    // since 1.3
    "AlarmWindow",                  // since 1.3
    "AlarmTime",                    // since 1.3
    "ChimeFlag",                     // since 1.3
    "FloatToTop",                   // since 1.4
};



VOID ReadProfile( PROFILE &Profile, HWND hwndFrame, PSZ pszApp )
{
    ULONG lNumRead;
    BOOL bTmp;
    COL_DEFS colTmp;
    DATETIME dtTmp;

    Profile.Open();
    Profile.ReadWindowData( hwndFrame );

    // if it's an older version, it might still contain the WindowPos key
    if( Profile.QueryVersionChange() )
	Profile.DeleteKey( pszApp, PrfKeys.pszWindowPos );

    // retrieve dotsEnlarged flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.pszDotsEnlarged, &bTmp, &lNumRead ) )
	CrazyClock.SetfDotsEnlarged( bTmp );

    // retrieve Seconds flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.pszSeconds, &bTmp, &lNumRead ) )
        CrazyClock.SetfSeconds( bTmp );

    // retrieve frame flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.pszFrame, &bTmp, &lNumRead ) )
	CrazyClock.SetfFrameIsVisible( bTmp );

    // retrieve 24h flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.psz24h, &bTmp, &lNumRead ) )
        CrazyClock.SetfShow24h( bTmp );

    // retrieve IconFrame flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.pszIconFrame, &bTmp, &lNumRead ) )
	CrazyClock.SetfIconFrameIsVisible( bTmp );

    // retrieve fAlarmWnd flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.pszfAlarmWnd, &bTmp, &lNumRead ) )
        CrazyClock.SetfAlarmWnd( bTmp );

    // retrieve fChime flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.pszfChime, &bTmp, &lNumRead ) )
        CrazyClock.SetfChime( bTmp );

    // retrieve fFloatToTop flag
    lNumRead = sizeof( BOOL );
    if( Profile.ReadData( pszApp, PrfKeys.pszfFloatToTop, &bTmp, &lNumRead ) )
        CrazyClock.SetfFloatToTop( bTmp );

    // retrieve color settings
    lNumRead = sizeof( COL_DEFS );
    if( Profile.ReadData( pszApp, PrfKeys.pszColors, (PVOID)&colTmp, 
			  &lNumRead ) ){
        colorset.backgnd = colTmp.backgnd;
        colorset.lighton = colTmp.lighton;
        colorset.lightoff = colTmp.lightoff;
        colorset.border = colTmp.border;
    }

    // retrieve AlarmTime
    lNumRead = sizeof( DATETIME );
    if( Profile.ReadData( pszApp, PrfKeys.pszAlarmTime, &dtTmp, &lNumRead ) ){
	   // retrieve AlarmOn flag
	lNumRead = sizeof( BOOL );
	if( Profile.ReadData( pszApp, PrfKeys.pszAlarmOn, &bTmp,
			      &lNumRead ) ) {
	      // first, load the saved data into Crazy Clock:
	   CrazyClock.ActivateAlarm( dtTmp.hours, dtTmp.minutes,
				     dtTmp.seconds );
	      // then check if alarm should be on or off:
	   if( !bTmp )
	      CrazyClock.DisableAlarm();
	} else // could not read alarm flag, default to off:
	   CrazyClock.DisableAlarm(); 
    } else // could not read alarm time
       CrazyClock.DisableAlarm(); 

    Profile.Close();	      
}			      



VOID WriteProfile( PROFILE &Profile, HWND hwndFrame, PSZ pszApp )
{
    ULONG lNumWrite;
    BOOL bTmp;
    COL_DEFS colTmp;
    DATETIME dtTmp;

    Profile.Open();

    Profile.WriteWindowData( hwndFrame );

    // write dotsEnlarged flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfDotsEnlarged();
    Profile.WriteData( pszApp, PrfKeys.pszDotsEnlarged, (PVOID)&bTmp, 
		       lNumWrite );

    // write Seconds flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfSeconds();
    Profile.WriteData( pszApp, PrfKeys.pszSeconds, (PVOID)&bTmp, lNumWrite );

    // write frame flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfFrameIsVisible();
    Profile.WriteData( pszApp, PrfKeys.pszFrame, (PVOID)&bTmp, lNumWrite );

    // write 24h flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfShow24h();
    Profile.WriteData( pszApp, PrfKeys.psz24h, (PVOID)&bTmp, lNumWrite );

    // write IconFrame flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfIconFrameIsVisible();
    Profile.WriteData( pszApp, PrfKeys.pszIconFrame, (PVOID)&bTmp, lNumWrite );

    // write AlarmOn flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfAlarmOn();
    Profile.WriteData( pszApp, PrfKeys.pszAlarmOn, (PVOID)&bTmp, lNumWrite );

    // write fAlarmWnd flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfAlarmWnd();
    Profile.WriteData( pszApp, PrfKeys.pszfAlarmWnd, (PVOID)&bTmp, 
		       lNumWrite );

    // write fChime flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfChime();
    Profile.WriteData( pszApp, PrfKeys.pszfChime, (PVOID)&bTmp, lNumWrite );

    // write fFloatToTop flag
    lNumWrite = sizeof( BOOL );
    bTmp = CrazyClock.QueryfFloatToTop();
    Profile.WriteData( pszApp, PrfKeys.pszfFloatToTop, (PVOID)&bTmp, 
		       lNumWrite );

    // write color Querytings
    lNumWrite = sizeof( COL_DEFS );
    Profile.WriteData( pszApp, PrfKeys.pszColors, (PVOID)&colorset, 
		       lNumWrite );

    // write AlarmTime
    lNumWrite = sizeof( DATETIME );
    dtTmp.hours = CrazyClock.QueryAlarmTimeHours();
    dtTmp.minutes = CrazyClock.QueryAlarmTimeMinutes();
    dtTmp.seconds = CrazyClock.QueryAlarmTimeSeconds();
    Profile.WriteData( pszApp, PrfKeys.pszAlarmTime, (PVOID)&dtTmp, 
		       lNumWrite ); 

    Profile.Close();
}





