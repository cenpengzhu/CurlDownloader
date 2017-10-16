#pragma once
#include <stdafx.h>

#ifdef DOWNLOADER_STATIC
#define DOWNLOADERAPI
#elifdef  DOWNLOADER_EXPORT
#define DOWNLOADERAPI __declspec(dllexport)
#else
#define DOWNLOADERAPI __declspec(dllimport)
#endif


//返回值-1,下载器初始化失败 -2,网络连接失败 -3,超过30s没速度 1,下载成功
int  __declspec(dllexport) CurlDownloadFile(const char *szURL, const char *szFilePath,long long &llTotalContent,long long &llCurrentContent ,int ThreadCount = 1);
