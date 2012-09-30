/* This is defs.h.
 * It contains standard definitions for messages, menu and other IDs.
 * In this file standard C comments must be used because it is put
 * through the GNU Preprocessor cpp which doesn't understand C++-comments
 */


/* undocumented stuff */

#define MM_QUERYITEMBYPOS	0x01F3
#define MAKE_16BIT_POINTER(p) \
		((PVOID)MAKEULONG(LOUSHORT(p),(HIUSHORT(p) << 3) | 7))

/* user messages:	*/

#define WM_BEGIN_PAINT  WM_USER+1
#define WM_END_PAINT    WM_USER+2
#define WM_ACK          WM_USER+3
#define WM_READ_ALL	WM_USER+4
#define WM_READ_ONE	WM_USER+5
#define WM_FRAMEREGION	WM_USER+6
#define WM_ALARM        WM_USER+7
#define WM_CHIME        WM_USER+8

/* Menu ID's:	*/

#define IDR_MAIN		100
#define IDR_POPUP		110
#define IDR_ICON		120

#define ID_OPTIONS	        200
#define ID_SECONDS 	        201
#define ID_SHOW24H	        202
#define ID_COLORS	 	203
#define ID_FRAME		204
#define ID_MINIMIZE	        205
#define ID_WINDOWSIZE	        206
#define ID_SETTINGS             207
#define ID_ALARM                208

#define ID_SOUND		210
#define ID_SOUND_ENABLE	        211
#define ID_READ_ONE	        212
#define ID_READ_ALL	        213

#define ID_EXIT		        220

#define ID_ENLARGED	        240
#define ID_ICONFRAME	        241

#define ID_HELP			300
#define ID_DISPLAY_HELP	        301
#define ID_EXT_HELP		302
#define ID_HELP_INDEX	        303
#define ID_KEYS_HELP		304

#define ID_ABOUT		310


/* constants used in the dialogs:	*/

/* Dialog ID's */

#define DID_HELP                750

#define DLG_COLORS            800
#define COLOR_BACKGND         801
#define COLOR_LIGHT_ON        802
#define COLOR_LIGHT_OFF       803
#define COLOR_LIGHT_BORDER    804

#define DLG_ABOUT	      900

#define DLG_SETTINGS                1000
#define SET_SECONDS                 1001
#define SET_24HOUR                  1002
#define SET_TITLEBAR                1003
#define SET_WINDOWSIZE              1004
#define SET_READNUMBERS             1005
#define SET_CHIME                   1006
#define SET_ENABLEMMPM              1007
#define SET_FLOATTOTOP              1008

#define DLG_ALARM                   1100
#define ALM_ON                      1101
#define ALM_HOUR                    1102
#define ALM_MINUTE                  1103
#define ALM_SECOND                  1104
#define ALM_WINDOW                  1105
#define ALM_TEXT                    1106
#define ALM_SOUND                   1107

#define DLG_ALARMMSG                1200

/* constants for string tables */
/* used in crazy.cpp */
      /* general string constants (used in all applications) */
#define STRG_APPNAME                1500
#define STRG_VERSION                1501
#define STRG_ININAME                1502
#define STRG_HELPFILE               1503
#define STRG_HELPTITLE              1504
#define STRG_NOHELP       	    1505
#define STRG_NO2NDTHREAD  	    1506
#define STRG_2NDTHREADNAME	    1507
#define STRG_ERROR        	    1508

      /* special string constants (used only in Crazy Clock) */
#define STRG_SHOWTITLEBAR           1510
#define STRG_NOTIMER      	    1511
#define STRG_REDUCEDOTSIZE	    1512
#define STRG_ENLARGEDOTSIZE 	    1513
#define STRG_HIDEICONFRAME  	    1514
#define STRG_SHOWICONFRAME  	    1515
/* used in mytime.cpp */

/*
#define 			    
#define 			    
#define 			    
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define 
#define STRG_PROFILE                1502

#define STRG_
#define STRG_
#define STRG_
#define STRG_
#define STRG_
#define STRG_
#define STRG_
#define STRG_
*/

/* helptables	*/
/* here must not be any tabs!!	*/

#define HELP_CLOCK 2000
#define SUBHELP_CLOCK 2001
#define EXTHELP_CLOCK 2002
#define SUBHELP_COLORS 2004
#define EXTHELP_COLORS 2005
#define SUBHELP_ABOUT 2006
#define SUBHELP_SETTINGS 2007
#define EXTHELP_SETTINGS 2008
#define SUBHELP_ALARM 2009

#define HELP_OPTIONS_MENU 2100
#define HELP_OPTIONS_SETTINGS 2101
#define HELP_OPTIONS_SETTINGS_TOPICS 2102
#define HELP_OPTIONS_COLORS 2103
#define HELP_OPTIONS_ALARM 2104

#define HELP_HELP_MENU 2200
#define HELP_SYS_MENU 2250
#define HELP_ID_ABOUT 2300
#define HELP_KEYS_HELP 2350

#define ID_COLORS_HELP 2400

#define PANEL_CIRCLES 2500
#define PANEL_ABOUT 2501
#define PANEL_POSTCARD 2502
#define PANEL_MINIMIZE 2503
#define PANEL_CREDITS 2504
 

