#pragma  once
#include <stdafx.h>
#include <CDownloadTaskManager.h>
#include <CDownloadThreadManager.h>
#include <CDownloadInfo.h>

using namespace std;

//下载状态宏
#define DOWNLOAD_RUN 1
#define DOWNLOAD_PAUSE 2
#define DOWNLOAD_STOP 0

class  CDownloader {
public:
	//远程下载地址url
	string m_strRemotePath;
	//本地下载地址
	string m_strLocalPath;
	//下载任务信息文件地址
	string m_strTaskInfoFilePath;
	//线程池线程数
	int m_nThreadCounts;
	//下载状态
	int m_nDownloadStatus;
	//下载者线程句柄
	HANDLE m_hDownloaderThreadHandle;
	//是否从远程下载
	bool m_bIsDownloadedFromRemote;

	//下载信息
	CDownloadInfo m_objDownloadInfo;

	CDownloadTaskManager m_objTaskManager;
	CDownloadThreadManager m_objThreadManager;

	CDownloader(const char * strRemotePath,const char * strLocalPath,const char * strTaskInfoFilePath, int nThreadCounts = 1);
	~CDownloader(){}	

	//下载初始化
	errorcode downloadInit();
	//下载开始
	errorcode downloadBegin();	
	//下载停止
	errorcode downloadStop();	
	//下载暂停
	errorcode downloadPause();
	//获取下载信息
	int getDownloadInfo();
	//归零
	int clear();
	//下载者子线程
	static unsigned _stdcall  downloaderThread (void * pParam);

};

