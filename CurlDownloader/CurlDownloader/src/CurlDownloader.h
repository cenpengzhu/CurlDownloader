#pragma once
#include <stdafx.h>

#ifdef DOWNLOADER_STATIC
#define DOWNLOADERAPI
#elifdef  DOWNLOADER_EXPORT
#define DOWNLOADERAPI __declspec(dllexport)
#else
#define DOWNLOADERAPI __declspec(dllimport)
#endif


//����ֵ-1,��������ʼ��ʧ�� -2,��������ʧ�� -3,����30sû�ٶ� 1,���سɹ�
int  __declspec(dllexport) CurlDownloadFile(const char *szURL, const char *szFilePath,long long &llTotalContent,long long &llCurrentContent ,int ThreadCount = 1);
