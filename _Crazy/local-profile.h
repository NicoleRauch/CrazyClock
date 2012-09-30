//////////////////////////////////////////////////////////////
//
//  local-profile.h
//  This file provides the functions that are based on the
//  standard profile functions (which will hopefully go into a DLL
//  some day ;-).
//
//////////////////////////////////////////////////////////////

#ifndef _LOCAL_PROFILE_H_
#define _LOCAL_PROFILE_H_

#include "profile.h"

VOID ReadProfile ( PROFILE& Profile, HWND hwndFrame, PSZ pszAppName );
VOID WriteProfile( PROFILE& Profile, HWND hwndFrame, PSZ pszAppName );


#endif  /* _LOCAL_PROFILE_H_ */
