
extern PSZ pszAppName;
extern HWND hwndFrame;

extern BOOL seconds;
extern BOOL FrameIsVisible;
extern BOOL DotsEnlarged;
extern BOOL Show24h;
extern BOOL IconFrameIsVisible;
extern BOOL AlarmOn;
extern BOOL fAlarmWnd;
extern DATETIME AlarmTime;
extern BOOL fChime;

BOOL ReadProfile( HAB hab );
BOOL WriteProfile( HAB hab );

