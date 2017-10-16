#include <stdafx.h>
#include <CurlDownloader.h>
#include "CDownloader.h"

INITIALIZE_EASYLOGGINGPP
int __declspec(dllexport) CurlDownloadFile(const char *szURL, const char *szFilePath, long long &llTotalContent, long long &llCurrentContent, int ThreadCount)
{
	string strLocalPath;
	strLocalPath = szFilePath;
	string strTaskInfoFilePath;
	string strLogFilePath;

	strTaskInfoFilePath = strLocalPath;
	strTaskInfoFilePath.append(".xml");

	strLogFilePath = strLocalPath;
	strLogFilePath.append(".log");

	//设置日志信息
	el::Configurations LogConf;
	LogConf.setToDefault();
	LogConf.set(el::Level::Info, el::ConfigurationType::Format, "%datetime--[Thread](%thread) %msg");
	LogConf.set(el::Level::Info, el::ConfigurationType::Enabled, "true");
	LogConf.set(el::Level::Info, el::ConfigurationType::ToStandardOutput, "true");
	LogConf.set(el::Level::Info, el::ConfigurationType::ToFile, "true");
	//日志记录在下载目录
	LogConf.set(el::Level::Info, el::ConfigurationType::Filename, strLogFilePath.c_str());
	el::Loggers::reconfigureLogger("default", LogConf);

	LOG(INFO) <<" Log Begin!";

	CDownloader objDownloader(szURL, szFilePath, strTaskInfoFilePath.c_str(), 10);

	if (objDownloader.downloadInit() != noerror)
	{
		objDownloader.downloadStop();
		//等待下载线程退出
		WaitForSingleObject(objDownloader.m_hDownloaderThreadHandle, INFINITE);
		return -1;
	}
	objDownloader.downloadBegin();

	//下载速度为0的连续时间。
	long long llCurrenttime = 0;
	long long llCurrentDownloadedLength = 0;
	while (true) {
		//检测网络是否连通，否则报错,停止下载。
		DWORD flags;
		if (!InternetGetConnectedState(&flags, 0))
		{
			objDownloader.downloadStop();
			//等待下载线程退出
			WaitForSingleObject(objDownloader.m_hDownloaderThreadHandle, INFINITE);

			return -2;
		}
		if (objDownloader.m_objDownloadInfo.m_llTotalDownloadedLength > llCurrentDownloadedLength)
		{
			//如果有下载速度 当前下载量更新 当前下载时间更新
			llCurrentDownloadedLength = objDownloader.m_objDownloadInfo.m_llTotalDownloadedLength;
			llCurrenttime = objDownloader.m_objDownloadInfo.m_dwTime;
		}
		else if (objDownloader.m_objDownloadInfo.m_llTotalDownloadedLength = llCurrentDownloadedLength)
		{
			//超过30秒没有速度则认为下载失败
			if (objDownloader.m_objDownloadInfo.m_dwTime - llCurrenttime >= 30000)
			{
				objDownloader.downloadStop();
				//等待下载线程退出
				WaitForSingleObject(objDownloader.m_hDownloaderThreadHandle, INFINITE);
				return -3;
			}
		}


		if (objDownloader.m_nDownloadStatus == THREAD_STOP)
		{
			//等待下载线程退出
			WaitForSingleObject(objDownloader.m_hDownloaderThreadHandle, INFINITE);
			//更新下载信息
			objDownloader.getDownloadInfo();

			//如果下载完成，且下载从远程服务器下载数据
			if (objDownloader.m_objDownloadInfo.m_llTotalDownloadedLength >= objDownloader.m_objTaskManager.m_llContentLength)
			{
				if (objDownloader.m_bIsDownloadedFromRemote == false)
				{
					return true;
				}
				long long llAverageSpeed = objDownloader.m_objDownloadInfo.getAverageSpeed();
				return 1;
			}
			else {
				return -1;
			}
		}
		llTotalContent = objDownloader.m_objTaskManager.m_llContentLength;
		llCurrentContent = objDownloader.m_objDownloadInfo.m_llTotalDownloadedLength;
		//CDownLoadImpl::_tagetDelegate->onDowloadDelegate(objDownloader.m_objDownloadInfo.m_llTotalDownloadedLength, objDownloader.m_objTaskManager.m_llContentLength, 0);
	}
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}