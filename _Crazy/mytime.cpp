#define INCL_GPI
#define INCL_WIN
#include <os2.h>

#include "Sound.h" 
#include "defs.h"
#include "mytime.h"

   // #define WCDEBUG  // this enables watchcat
   // #include "..\_Standard\wcapi.h"

// Globals

ClockFace CrazyClock;

COL_DEFS colorset;
LONG Colors[15] = {
	CLR_BLUE,
	CLR_RED,
	CLR_PINK,
	CLR_GREEN,
	CLR_CYAN,
	CLR_YELLOW,
	CLR_DARKGRAY,
	CLR_DARKBLUE,
	CLR_DARKRED,
	CLR_DARKPINK,
	CLR_DARKGREEN,
	CLR_DARKCYAN,
	CLR_PALEGRAY,
	CLR_BLACK,
	CLR_WHITE
};


// declaration of static class members:
INT RowOfDots::Convert[7][10] = {
    { 1, 0, 1, 1, 0, 1, 1, 1, 1, 1 }, 
    { 1, 1, 1, 1, 1, 0, 0, 1, 1, 1 },
    { 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 },
    { 1, 0, 1, 0, 0, 0, 1, 0, 1, 0 },
    { 1, 0, 0, 0, 1, 1, 1, 0, 1, 1 },
    { 0, 0, 1, 1, 1, 1, 1, 0, 1, 1 } 
    };

LONG RowOfDots::Delta;

// Class member functions:
void RowOfDots::DrawOutlines( HPS hps )
{
    POINTL Center = { LeftCenter.x, LeftCenter.y };

    for( char j = 0; j < 7; j++, Center.x += Delta ){
	GpiMove (hps, &Center );
	GpiFullArc( hps, DRO_OUTLINE, MAKEFIXED( 1, 0) );
    }
}

void RowOfDots::DrawDots( HPS hps, BOOL LightsOff )
{
    POINTL Center = { LeftCenter.x, LeftCenter.y };
    for( char j = 0; j < 7; j++, Center.x += Delta) {
	GpiMove (hps, &Center );
	if( LightsOff ) // don't paint leading 0
	    GpiSetColor(hps, Colors[colorset.lightoff] );
	else 
	    GpiSetColor ( hps, (Convert[j][CurrentTime] == 0) ?
			  Colors[colorset.lightoff] :
			  Colors[colorset.lighton] );
	GpiFullArc ( hps, DRO_FILL, MAKEFIXED( 1, 0 ) );
    }
}

   ////////////////////////////////////////////////////////////
Time::Time()
{
   fIsDue = FALSE;

      // set both times to 1.1.1971:
   tmRefTime.tm_mday = 1;
   tmRefTime.tm_mon = 1;
   tmRefTime.tm_year = 71;

   tmCurTime.tm_mday = 1;
   tmCurTime.tm_mon = 1;
   tmCurTime.tm_year = 71;

      // set the reference time to 0:00:00
   tmRefTime.tm_hour = 0;
   tmRefTime.tm_min = 0;
   tmRefTime.tm_sec = 0;

      // create tRefTime:
   tRefTime = mktime( &tmRefTime );
    
}


   // today specifies whether the day, month, year should be set to today
   // or to be left as they are
   // (the constructor initializes them to 1.1.1970) 
VOID Time::SetReference( INT h, INT m, INT s, BOOL today )
{
   if( today ) {
      tRefTime = time( NULL );
      _localtime( &tRefTime, &tmRefTime );
	 // get a complete struct
   }

      // fill in the desired time
   tmRefTime.tm_sec = s;
   tmRefTime.tm_min = m;
   tmRefTime.tm_hour = h;

   tRefTime = mktime( &tmRefTime );
}

   // this function adjusts only the hour of the reference time
VOID Time::SetReferenceHour( INT h )
{
   tmRefTime.tm_hour = h;
   tRefTime = mktime( &tmRefTime );
}

VOID Time::SetCurrent( INT h, INT m, INT s, BOOL today )
{
   if( today ) {
      tCurTime = time( NULL );
      _localtime( &tCurTime, &tmCurTime );
	 // get a complete struct
   }

      // fill in the desired time
   tmCurTime.tm_sec = s;
   tmCurTime.tm_min = m;
   tmCurTime.tm_hour = h;

   tCurTime = mktime( &tmCurTime );
}


   // this function returns TRUE if the desired time is reached
BOOL Time::Compare( INT AdvanceMinutes )
{
   if( tCurTime < tRefTime ){ // it's not time yet
      if( difftime( tRefTime, tCurTime ) < AdvanceMinutes * 60 )
         fIsDue = TRUE;
   } else {
      if( fIsDue ) {
         fIsDue = FALSE;
         return TRUE;
      }
   }
   return FALSE;
}

   // returns TRUE if the reference time is already passed
BOOL Time::IsPassed()
{
   return (tRefTime < tCurTime);
}

   // increases the reference time by 1 day
VOID Time::IncreaseReference()
{
   tRefTime += 86400;  // number of seconds in one day
   _localtime( &tRefTime, &tmRefTime );  // update the other representation
}
   

   ////////////////////////////////////////////////////////////

ClockFace::ClockFace()
{
    ArcParams.lP = ArcParams.lQ = ArcParams.lR = ArcParams.lS = 0;
    // initialize the flags:
    fDotsEnlarged = FALSE;
    fSeconds = TRUE;
    fFrameIsVisible = FALSE;
    fReadNumbers = FALSE;
    fShow24h = TRUE;
    fIconFrameIsVisible = TRUE;
    fAlarmOn = FALSE;
    fAlarmWnd = FALSE;
    fChime = FALSE;
    fFloatToTop = FALSE;
}


VOID ClockFace::SetTime()
{
    DATETIME DateTime;                     /* get the time	*/
    DosGetDateTime(&DateTime);

    if( !fShow24h )	// use 12-hour-convention
	if( DateTime.hours > 12 ) DateTime.hours -= 12;
    
    DotRow[5].SetTime( DateTime.hours   / 10 );
    DotRow[4].SetTime( DateTime.hours   % 10 );
    DotRow[3].SetTime( DateTime.minutes / 10 );
    DotRow[2].SetTime( DateTime.minutes % 10 );
    DotRow[1].SetTime( DateTime.seconds / 10 );
    DotRow[0].SetTime( DateTime.seconds % 10 );
}	

VOID ClockFace::CalcMinWindowSize( INT *Width, INT *Height )
{
	*Width = Delta * 8;
	*Height = Delta * ( fSeconds ? 7 : 5 );
}

// calculates the size of the circles and draws the black outlines
VOID ClockFace::PaintBackground( HWND hwnd, HPS hps )
{
    INT k, j;
    POINTL Center;
    RECTL   rect;			// window rectangle
    ULONG ulStyle;
    
    WinQueryWindowRect(hwnd, &rect);
    ulStyle = WinQueryWindowULong( WinQueryWindow( hwnd, QW_PARENT),
				   QWL_STYLE );


    if( (ulStyle & WS_MINIMIZED) && fIconFrameIsVisible ){
	// only change painting when program minimized and when desired
	// adjust the drawing rectangle:
	LONG cxSizeBorder = WinQuerySysValue( HWND_DESKTOP, SV_CXSIZEBORDER );
	LONG cySizeBorder = WinQuerySysValue( HWND_DESKTOP, SV_CYSIZEBORDER );
	rect.xLeft += cxSizeBorder;
	rect.xRight -= cxSizeBorder;
	rect.yBottom += cySizeBorder;
	rect.yTop -= cySizeBorder;
    }


    k = (rect.xRight - rect.xLeft) / 8;
    j = (rect.yTop - rect.yBottom) / (fSeconds ? 7 : 5);
    Delta = k < j ? k : j;
    DotRow[0].SetDelta( Delta );// distance between two circle centers
    ArcParams.lP = ArcParams.lQ = Delta / 3 + 2;    

    // calculate lower left circle center
    Center.x = Delta + rect.xLeft +
	(rect.xRight - rect.xLeft - 8 * Delta ) / 2;
    Center.y = Delta + rect.yBottom +
	(rect.yTop - rect.yBottom - (fSeconds ? 7 : 5) * Delta ) / 2;

    // set the positions of the DotRows:
    for( k = (fSeconds ? 0 : 2); k < 6; k++, Center.y += Delta )
	DotRow[k].SetLeftCenter( Center );

    WinFillRect( hps, &rect, Colors[colorset.backgnd] );

    if( ulStyle & WS_MINIMIZED ){	// adjust the arcparams and quit
	ArcParams.lP -= 2;
	if( fDotsEnlarged )
	    ArcParams.lQ--;	// make elliptic dots
	else
	    ArcParams.lQ -= 2;	
	return;			// don't paint the circle borders
    }

    GpiSetArcParams( hps, &ArcParams );
    GpiSetColor( hps, Colors[colorset.border] );
    for( k = (fSeconds ? 0 : 2); k < 6; k++ )
	DotRow[k].DrawOutlines( hps );

    ArcParams.lP = ArcParams.lQ -= 2;
}


VOID ClockFace::WorkPaint( HPS hps, BOOL FirstPaint )
{
    INT k;
    INT maxdraw = 2;	/* default: only paint lowest two rows	*/

    SetTime();

    if( FirstPaint ){
	maxdraw = 6;	// paint all
    } else {
	if( DotRow[0].Zero() ){	        /* hh.mm.s0 */
	    maxdraw++;			/* paint three lowest rows */
	    if( DotRow[1].Zero() ){	/* hh.mm.00 */
		maxdraw++;		/* paint four lowest rows  */
		if( DotRow[2].Zero() ){	/* hh.m0.00 */
		    maxdraw++;		/* paint five lowest rows	*/
		    if( DotRow[3].Zero() ){	/* hh.00.00 */
			maxdraw++;	/* paint all */
		    }
		}
	    }
	}
    }

    GpiSetArcParams( hps, &ArcParams );
    for(k = (fSeconds ? 0 : 2); k < maxdraw; k++ ) 
	DotRow[k].DrawDots( hps, (k == 5 && DotRow[5].Zero()) );
	// lights are off for the topmost row if it's zero
}

LONG ClockFace::PositionToRead( POINTL ptl )
{
	ptl.y -= ( DotRow[0].GetY() - Delta / 2 );
	if( ptl.y < 0 || ptl.y > ( fSeconds ? 6 : 4 ) * Delta )
		return 10L;	// no valid position; Abbruchbedingung
	return ( ptl.y	/ Delta + (fSeconds ? 0 : 2) );
}
	


// calculates how small the window may be
VOID ClockFace::SetMinTrackSize( POINTL &ptl )
{
    	ptl.x = 6 * 6;
	ptl.y = ( fSeconds ? 5 : 4 ) * 3 * 2;
}


BOOL ClockFace::RetrieveNumberToRead( LONG *NrToRead )
{
    if( *NrToRead > 6 )	// check whether mouseclick was valid
	return FALSE;
    if( *NrToRead == 5 && DotRow[5].Zero() )
	// don't read leading 0 if it's earlier than 10 o'clock:
	return FALSE; 
    *NrToRead = DotRow[*NrToRead].GetTime();  // which figure must be read?
    return TRUE;
}


BOOL ClockFace::CheckAlarm()
{
    if( !fAlarmOn ) return FALSE;

	 // set the current time
    Alarm.SetCurrent( 10*DotRow[5].GetTime()+DotRow[4].GetTime(),
		      10*DotRow[3].GetTime()+DotRow[2].GetTime(),
		      10*DotRow[1].GetTime()+DotRow[0].GetTime(), TRUE );

    // wake up, time to die!
    BOOL RetVal = Alarm.Compare( 60 );
    if( RetVal )
       	 Alarm.IncreaseReference();
    
    return RetVal;
} // ClockFace::CheckAlarm()


BOOL ClockFace::CheckChime()
{
   if( !fChime ) return FALSE;

	 // set the current time
      Chime.SetCurrent( 0, 10*DotRow[3].GetTime()+DotRow[2].GetTime(),
			10*DotRow[1].GetTime()+DotRow[0].GetTime(), FALSE );
	 // set the desired chime time hour:
      Chime.SetReferenceHour( DotRow[3].GetTime() < 3 ? 0 : 1 );
	 // if the minutes are less than 30, set hour to 0, otherwise to 1

      return Chime.Compare( 15 );
}  // ClockFace::CheckChime()
	
VOID ClockFace::ActivateAlarm( UCHAR h, UCHAR m, UCHAR s )
{
   fAlarmOn = TRUE;
   
   Alarm.SetReference( h, m, s, TRUE );

   if( Alarm.IsPassed() )   // check if today's time is already passed
      Alarm.IncreaseReference();
}
