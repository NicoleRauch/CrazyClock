/* global variables */

typedef struct  {
	USHORT cb;		// contains the length of the struct in bytes	
	LONG backgnd;
	LONG lighton;
	LONG lightoff;
	LONG border;
} COL_DEFS;

extern COL_DEFS colorset;
extern LONG Colors[15];
extern BOOL DotsEnlarged;



/* global functions */

VOID InitNumbers();

VOID EvalDots( DATETIME DateTime, INT *curtime, BOOL Show24h );

VOID WorkPaint ( HPS hps, BOOL FirstPaint, BOOL seconds, INT *curtime,
						BOOL Show24h );

VOID CalcMinWindowSize( BOOL seconds, INT *Width, INT *Height );

VOID InitSpace( HWND hwnd, HPS hps, BOOL seconds, BOOL IconFrameIsVisible );

LONG PositionToRead( POINTL ptl, BOOL seconds );

VOID CheckAlarm( HWND hwndFrame, INT *current, DATETIME wanted );
MRESULT EXPENTRY AlarmMsgDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);


