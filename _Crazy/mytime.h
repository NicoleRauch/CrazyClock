/* global variables */

#ifndef _MYTIME_H_
#define _MYTIME_H_

#include <time.h>

struct COL_DEFS {
	USHORT cb;		// contains the length of the struct in bytes	
	LONG backgnd;
	LONG lighton;
	LONG lightoff;
	LONG border;
};

extern COL_DEFS colorset;
extern LONG Colors[15];

// Classes:
class RowOfDots 
{
    static INT Convert[7][10];
    static LONG Delta;
    POINTL LeftCenter;     // where is the center of the leftmost one?
    INT CurrentTime;       // what number the dot displays

public:
    VOID DrawOutlines( HPS hps );
    VOID DrawDots( HPS hps, BOOL LightsOff );
    VOID SetDelta( LONG d ) { Delta = d; }
    VOID SetLeftCenter( POINTL p ){ LeftCenter.x=p.x; LeftCenter.y=p.y; }
    VOID SetTime( INT t ) { CurrentTime = t; }
    INT GetTime() { return CurrentTime; }
    BOOL Zero() { return !CurrentTime; }
    LONG GetY() { return LeftCenter.y; }
};


class Time {
   time_t    tRefTime;    // the Reference time (2 representations)
   struct tm tmRefTime;
   time_t    tCurTime;   // the current time
   struct tm tmCurTime;
   
   BOOL fIsDue;

public:
   Time();
   VOID SetReference( INT h, INT m, INT s, BOOL today );
   VOID SetReferenceHour( INT h );
   VOID SetCurrent( INT h, INT m, INT s, BOOL today );
   BOOL Compare( INT AdvanceMinutes );
   BOOL IsPassed();
   VOID IncreaseReference();  // increases the reference time by one day
   INT GetRefHours()   { return tmRefTime.tm_hour; }
   INT GetRefMinutes() { return tmRefTime.tm_min;  }
   INT GetRefSeconds() { return tmRefTime.tm_sec;  }
};

   //////////////////////////////////////////////////////////////////

class ClockFace 
{
   RowOfDots DotRow[6];
   ARCPARAMS ArcParams;  // the same for all rows
   LONG Delta;
   Time Alarm, Chime;

      // flags that determine the clock's appearance:
   BOOL fDotsEnlarged;
   BOOL fSeconds;			// TRUE : displays seconds
   BOOL fFrameIsVisible;	// TRUE : Title Bar etc. are visible
   BOOL fReadNumbers;	// not to be stored in the .INI file!
      //    BOOL fSound;	// indicates whether sound is played via MMPM
      //    BOOL fSoundPossible; // indicates whether MMPM is active at all
   BOOL fShow24h;	// TRUE : time is displayed in 24-h-mode
   BOOL fIconFrameIsVisible;// TRUE : frame is drawn around the min. prog.
   BOOL fAlarmOn;   // indicates whether alarm has been activated
   BOOL fAlarmWnd;  // i. w. alarm should open a window
   BOOL fChime;   // indicates whether chime has been activated
   BOOL fFloatToTop;  // should the clock always come to the front?


public:    
   ClockFace();
   VOID SetDelta( LONG d ) { Delta = d; }
   VOID SetTime();
   VOID CalcMinWindowSize( INT *Width, INT *Height );
   VOID PaintBackground( HWND hwnd, HPS hps );
   VOID WorkPaint( HPS hps, BOOL FirstPaint );
   LONG PositionToRead( POINTL ptl );
   BOOL CheckAlarm();
   BOOL CheckChime();
   VOID SetMinTrackSize( POINTL &ptl );
   BOOL RetrieveNumberToRead( LONG *NrToRead );

      // set the clock flags:
   VOID SetfDotsEnlarged      ( BOOL v ) { fDotsEnlarged = v; }
   VOID SetfSeconds           ( BOOL v ) { fSeconds = v; }
   VOID SetfFrameIsVisible    ( BOOL v ) { fFrameIsVisible = v; }     
   VOID SetfReadNumbers       ( BOOL v ) { fReadNumbers = v; }        
      //    VOID SetfSound             ( BOOL v ) { fSound = fSoundPossible && v; }
   VOID SetfShow24h           ( BOOL v ) { fShow24h = v; }            
   VOID SetfIconFrameIsVisible( BOOL v ) { fIconFrameIsVisible = v; } 
   VOID SetfAlarmWnd          ( BOOL v ) { fAlarmWnd = v; }           
   VOID SetfChime             ( BOOL v ) { fChime = v; }           
   VOID SetfFloatToTop        ( BOOL v ) { fFloatToTop = v; }

      // this one is only called at create time:
      //    VOID ActivateMMPM          () { fSound = fSoundPossible = TRUE; }
      // query the clock flags:
   BOOL QueryfDotsEnlarged()       { return fDotsEnlarged; }
   BOOL QueryfSeconds()            { return fSeconds; }      
   BOOL QueryfFrameIsVisible()     { return fFrameIsVisible; }      
   BOOL QueryfReadNumbers()        { return fReadNumbers; }         
      //    BOOL QueryfSound()              { return fSoundPossible && fSound; }
      //    BOOL QueryfSoundPossible()      { return fSoundPossible; }       
   BOOL QueryfShow24h()            { return fShow24h; }
   BOOL QueryfIconFrameIsVisible() { return fIconFrameIsVisible; }
   BOOL QueryfAlarmOn()            { return fAlarmOn; }
   BOOL QueryfAlarmWnd()           { return fAlarmWnd; }           
   BOOL QueryfChime()              { return fChime; }              
   BOOL QueryfFloatToTop()         { return fFloatToTop; }

      // toggle the clock flags:
   VOID TogglefSeconds()            { fSeconds = !fSeconds; }      
   VOID TogglefReadNumbers()        { fReadNumbers = !fReadNumbers; }         
   VOID TogglefShow24h()            { fShow24h = !fShow24h; }
   VOID TogglefChime()              { fChime = !fChime; } 
      // include security check:
      //    VOID TogglefSound()              { fSound = fSoundPossible && !fSound; }

   VOID TogglefDotsEnlarged()       { fDotsEnlarged = !fDotsEnlarged; }
   VOID TogglefFrameIsVisible()     { fFrameIsVisible = !fFrameIsVisible; }
   VOID TogglefIconFrameIsVisible() { fIconFrameIsVisible = 
					 !fIconFrameIsVisible; }
   VOID TogglefAlarmWnd()           { fAlarmWnd = !fAlarmWnd; }           
   VOID TogglefFloatToTop()         { fFloatToTop = !fFloatToTop; }

      // Alarm time stuff:
   VOID ActivateAlarm( UCHAR h, UCHAR m, UCHAR s );
   VOID DisableAlarm() { fAlarmOn = FALSE; }
   
   INT QueryAlarmTimeHours  () { return Alarm.GetRefHours();   }
   INT QueryAlarmTimeMinutes() { return Alarm.GetRefMinutes(); }
   INT QueryAlarmTimeSeconds() { return Alarm.GetRefSeconds(); }
};
    
extern ClockFace CrazyClock;

/* global functions */

MRESULT EXPENTRY AlarmMsgDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);


#endif
