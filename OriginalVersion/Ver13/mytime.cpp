#define INCL_GPI
#define INCL_WIN
#include <os2.h>

#include "mmsound.h" 
#include "crazy.h"
#include "mytime.h"

// #include "..\_Standard\wcapi.h"

// Globals

INT convert[7][10];
LONG delta;
POINTL LowerLeft;
ARCPARAMS arcparams;
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

VOID InitNumbers()
{
	convert[0][0] = 1; convert[1][0] = 1; convert[2][0] = 1; convert[3][0] = 1; convert[4][0] = 1; convert[5][0] = 1; convert[6][0] = 0;
	convert[0][1] = 0; convert[1][1] = 1; convert[2][1] = 1; convert[3][1] = 0; convert[4][1] = 0; convert[5][1] = 0; convert[6][1] = 0;
	convert[0][2] = 1; convert[1][2] = 1; convert[2][2] = 0; convert[3][2] = 1; convert[4][2] = 1; convert[5][2] = 0; convert[6][2] = 1;
	convert[0][3] = 1; convert[1][3] = 1; convert[2][3] = 1; convert[3][3] = 1; convert[4][3] = 0; convert[5][3] = 0; convert[6][3] = 1;
	convert[0][4] = 0; convert[1][4] = 1; convert[2][4] = 1; convert[3][4] = 0; convert[4][4] = 0; convert[5][4] = 1; convert[6][4] = 1;
	convert[0][5] = 1; convert[1][5] = 0; convert[2][5] = 1; convert[3][5] = 1; convert[4][5] = 0; convert[5][5] = 1; convert[6][5] = 1;
	convert[0][6] = 1; convert[1][6] = 0; convert[2][6] = 1; convert[3][6] = 1; convert[4][6] = 1; convert[5][6] = 1; convert[6][6] = 1;
	convert[0][7] = 1; convert[1][7] = 1; convert[2][7] = 1; convert[3][7] = 0; convert[4][7] = 0; convert[5][7] = 0; convert[6][7] = 0;
	convert[0][8] = 1; convert[1][8] = 1; convert[2][8] = 1; convert[3][8] = 1; convert[4][8] = 1; convert[5][8] = 1; convert[6][8] = 1;
	convert[0][9] = 1; convert[1][9] = 1; convert[2][9] = 1; convert[3][9] = 1; convert[4][9] = 0; convert[5][9] = 1; convert[6][9] = 1;

} /* end InitNumbers  	*/

VOID EvalDots( DATETIME DateTime, INT *curtime, BOOL Show24h )
{
	if( !Show24h )	// use 12-hour-convention
		if( DateTime.hours > 12 ) DateTime.hours -= 12;
	curtime[5] = DateTime.hours / 10;
	curtime[4] = DateTime.hours % 10;
	curtime[3] = DateTime.minutes / 10;
	curtime[2] = DateTime.minutes % 10;
   curtime[1] = DateTime.seconds / 10;
	curtime[0] = DateTime.seconds % 10;
}	

VOID CalcMinWindowSize( BOOL seconds, INT *Width, INT *Height )
{
	*Width = delta * 8;
	*Height = delta * ( seconds ? 7 : 5 );
}

VOID InitSpace( HWND hwnd, HPS hps, BOOL seconds, BOOL IconFrameIsVisible )
	// calculates the size of the circles and draws the black outlines
{
	INT k, j;
   POINTL Center;
	RECTL   rect;			// window rectangle
	ULONG ulStyle;

   WinQueryWindowRect(hwnd, &rect);
   ulStyle = WinQueryWindowULong( WinQueryWindow( hwnd, QW_PARENT),
											 QWL_STYLE );


   if( (ulStyle & WS_MINIMIZED) && IconFrameIsVisible ){
   		// only change painting when program minimized and when desired
			// adjust the drawing rectangle:
		LONG cxSizeBorder = WinQuerySysValue( HWND_DESKTOP, SV_CXSIZEBORDER );
		LONG cySizeBorder = WinQuerySysValue( HWND_DESKTOP, SV_CYSIZEBORDER );
		rect.xLeft += cxSizeBorder;
		rect.xRight -= cxSizeBorder;
		rect.yBottom += cySizeBorder;
		rect.yTop -= cySizeBorder;
	}

	arcparams.lR = arcparams.lS = 0;

	k = (rect.xRight - rect.xLeft) / 8;
	j = (rect.yTop - rect.yBottom) / (seconds ? 7 : 5);
	delta = ( k < j ? k : j);	// distance between two circle centers
	arcparams.lP = arcparams.lQ = delta / 3 + 2;

	// calculate lower left circle center
	LowerLeft.x = delta + rect.xLeft +
					 (rect.xRight - rect.xLeft - 8 * delta ) / 2;
	LowerLeft.y = Center.y = delta + rect.yBottom +
					  (rect.yTop - rect.yBottom - (seconds ? 7 : 5) * delta ) / 2;
	WinFillRect( hps, &rect, Colors[colorset.backgnd] );

	if( ulStyle & WS_MINIMIZED ){	// adjust the arcparams and quit
//		arcparams.lP = arcparams.lQ -= 2;
		arcparams.lP -= 2;
		if( DotsEnlarged )
			arcparams.lQ--;	// make elliptic dots
		else
			arcparams.lQ -= 2;	
		return;			// don't paint the circle borders
	}

	GpiSetArcParams( hps, &arcparams );
	GpiSetColor( hps, Colors[colorset.border] );
	for(k=0; k < (seconds ? 6 : 4); k++, Center.y += delta)
	{
		Center.x = LowerLeft.x;
		for (j=0; j < 7; j++, Center.x += delta)
		{
			GpiMove (hps, &Center );
			GpiFullArc( hps, DRO_OUTLINE, MAKEFIXED( 1, 0) );
		}
	}
	arcparams.lP = arcparams.lQ -= 2;
}


VOID WorkPaint ( HPS hps, BOOL FirstPaint, BOOL seconds, INT *curtime,
						BOOL Show24h )
{
    DATETIME DateTime;                     /* get the time	*/
//	INT curtime [6];
    INT k, j;
    POINTL Center;
    INT maxdraw = 2;	/* default: only paint lowest two rows	*/

    DosGetDateTime(&DateTime);
	
    EvalDots( DateTime, curtime, Show24h );
    if( FirstPaint ){
	maxdraw = 6;	// paint all
    } else {
	if( curtime[0] == 0){	/* hh.mm.s0 */
	    maxdraw++;				/* paint three lowest rows	*/
	    if (curtime[1] == 0){	/* hh.mm.00 */
		maxdraw++;			/* paint four lowest rows	*/
		if( curtime[2] == 0 ){	/* hh.m0.00 */
		    maxdraw++;		/* paint five lowest rows	*/
		    if (curtime[3] == 0){	/* hh.00.00 */
			maxdraw++;	/* paint all */
//						if (curtime[4] == 0)	/* h0.00.00	*/
//							maxdraw++;	/* paint all	*/
		    }
		}
	    }
	}
    }
    Center.y = LowerLeft.y;
    GpiSetArcParams( hps, &arcparams );
    for(k = (seconds ? 0 : 2); k < maxdraw; k++, Center.y += delta) {
	Center.x = LowerLeft.x;
	for (j=0; j < 7; j++, Center.x += delta) {
	    GpiMove (hps, &Center );
	    if(k == 5 && curtime[5] == 0)
		GpiSetColor(hps, Colors[colorset.lightoff] );
				// don't paint leading 0
	    else 
		GpiSetColor ( hps, (convert[j][curtime[k]] == 0) ?
			      Colors[colorset.lightoff] :
			      Colors[colorset.lighton] );
	    GpiFullArc ( hps, DRO_FILL, MAKEFIXED( 1, 0 ) );
	}
    }
}

LONG PositionToRead( POINTL ptl, BOOL seconds )
{
	ptl.y -= ( LowerLeft.y - delta / 2 );
	if( ptl.y < 0 || ptl.y > ( seconds ? 6 : 4 ) * delta )
		return 10L;	// no valid position; Abbruchbedingung
	return ( ptl.y	/ delta + (seconds ? 0 : 2) );
}
	

VOID CheckAlarm( HWND hwndWorker, INT *current, DATETIME wanted ){
    if( (10*current[5]+current[4]) != wanted.hours ) return;
    if( (10*current[3]+current[2]) != wanted.minutes ) return;
    if( (10*current[1]+current[0]) != wanted.seconds ) return;
    // wake up, time to die!
    WinSendMsg( hwndWorker, WM_ALARM, 0, 0 );
}

	
	







