#if !defined(AFX_STDAFX_H__E30B2003_188B_4EB4_AB99_3F3734D6CE6C__INCLUDED_)
#define AFX_STDAFX_H__E30B2003_188B_4EB4_AB99_3F3734D6CE6C__INCLUDED_

#pragma once

#ifdef __GNUC__
// 怎么都没找到min，max的头文件-_-
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#endif

#ifndef __FILET__
#define __DUILIB_STR2WSTR(str)	L##str
#define _DUILIB_STR2WSTR(str)	__DUILIB_STR2WSTR(str)
#ifdef _UNICODE
#define __FILET__	_DUILIB_STR2WSTR(__FILE__)
#define __FUNCTIONT__	_DUILIB_STR2WSTR(__FUNCTION__)
#else
#define __FILET__	__FILE__
#define __FUNCTIONT__	__FUNCTION__
#endif
#endif

#define _CRT_SECURE_NO_DEPRECATE

// Remove pointless warning messages
#ifdef _MSC_VER
#pragma warning (disable : 4511) // copy operator could not be generated
#pragma warning (disable : 4512) // assignment operator could not be generated
#pragma warning (disable : 4702) // unreachable code (bugs in Microsoft's STL)
#pragma warning (disable : 4786) // identifier was truncated
#pragma warning (disable : 4996) // function or variable may be unsafe (deprecated)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS // eliminate deprecation warnings for VS2005
#endif
#endif // _MSC_VER
#ifdef __BORLANDC__
#pragma option -w-8027		   // function not expanded inline
#endif

#define ELPP_STL_LOGGING
#define ELPP_NO_DEFAULT_LOG_FILE

// Required for VS 2008 (fails on XP and Win2000 without this fix)
#  ifdef _WIN32_WINNT  
#   undef  _WIN32_WINNT  
#  endif  
#   define _WIN32_WINNT 0x0500  
#  ifndef WINVER  
#    define WINVER 0x0500  
#  endif  

#include <easylogging++.h>
#include <Windows.h>
#include <Wininet.h>
#include <curl/curl.h>
#include <tinyxml2.h>
#include <map>
#include <string>
#include <fstream>

#define  DOWNLOADER_EXPORT

#ifdef _DEBUG
#    pragma  comment(lib,"libcurld_mt.lib")
#    pragma  comment(lib,"tinyxml2d_mt.lib")
#else
#    pragma  comment(lib,"libcurl_mt.lib")
#    pragma  comment(lib, "libssh2.lib")
/*
#    pragma  comment(lib, "libeay32MT.lib")
#    pragma  comment(lib, "ssleay32MT.lib")*/
#    pragma  comment(lib,"tinyxml2_mt.lib")
#endif
#pragma  comment(lib,"legacy_stdio_definitions.lib")
#pragma comment(lib,"Wininet.lib")

using namespace std;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E30B2003_188B_4EB4_AB99_3F3734D6CE6C__INCLUDED_)
