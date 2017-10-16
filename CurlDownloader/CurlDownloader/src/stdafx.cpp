// stdafx.cpp : source file that includes just the standard includes
//	UIlib.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information
#include "stdafx.h"

#if _MSC_VER>=1900
#include "stdio.h" 
_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned);
#ifdef __cplusplus 
extern "C"
#endif 
#ifndef __iob_func
FILE* __cdecl __iob_func(unsigned i) {
	return __acrt_iob_func(i);
}
#endif // !__iob_func
#endif /* _MSC_VER>=1900 */

#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "comctl32.lib" )
