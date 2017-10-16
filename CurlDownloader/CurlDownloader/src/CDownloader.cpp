#include "CDownloader.h"

using namespace std;

CDownloader::CDownloader(const char * strRemotePath,const char * strLocalPath,const char * strTaskInfoFilePath, int nThreadCounts) : m_objTaskManager(strRemotePath,strLocalPath,strTaskInfoFilePath),m_objThreadManager(strLocalPath){

	m_strRemotePath = strRemotePath;
	m_strLocalPath = strLocalPath;
	m_strTaskInfoFilePath = strTaskInfoFilePath; 
	m_bIsDownloadedFromRemote = false;

	m_nThreadCounts = nThreadCounts;
	m_nDownloadStatus = DOWNLOAD_PAUSE;
	m_objDownloadInfo.m_dwTime = GetTickCount();
	m_hDownloaderThreadHandle = (HANDLE)_beginthreadex(NULL,0,&CDownloader::downloaderThread,(LPVOID)this,0,NULL);
}

//下载初始化
errorcode CDownloader::downloadInit(){
	//创建线程
	if (m_objThreadManager.createThreads(m_nThreadCounts) == 0)
	{
		return localfilerror;
	}

	//加载任务信息
	errorcode err;
	err = m_objTaskManager.loadDownloadTask();

	if (err != noerror)
	{
		LOG(INFO) <<" [Error]: "<<"error to loadDownloadTask";
		return err;
	}

	m_bIsDownloadedFromRemote = false;

	return noerror;
}

//下载开始
errorcode CDownloader::downloadBegin () {
	m_nDownloadStatus = DOWNLOAD_RUN;
	return noerror;
}

//下载停止
errorcode CDownloader::downloadStop(){
	m_nDownloadStatus = DOWNLOAD_STOP;
	return noerror;
}

//下载暂停
errorcode CDownloader::downloadPause(){
	m_nDownloadStatus = DOWNLOAD_PAUSE;
	return noerror;
}

//获取下载信息
int CDownloader::getDownloadInfo(){

	CDownloadInfo objDownloadInfo;
	string strDownloadSpeed;
	long long llLengthPerSecond = 0;
	double dPercent = 0.00;

	objDownloadInfo.m_dwTime = GetTickCount();

	objDownloadInfo.m_llTotalDownloadedLength = m_objTaskManager.getTotalDownloadedLength();

	if ((objDownloadInfo.m_dwTime - m_objDownloadInfo.m_dwTime) == 0)
	{
		return 1;
	}

	//获取并刷新总下载时间
	objDownloadInfo.m_llTotalDownloadTime = m_objTaskManager.freshDownloadTime(objDownloadInfo.m_dwTime - m_objDownloadInfo.m_dwTime);
	
	//计算即时下载速度
	llLengthPerSecond = (objDownloadInfo.m_llTotalDownloadedLength - m_objDownloadInfo.m_llTotalDownloadedLength)*1000/(objDownloadInfo.m_dwTime - m_objDownloadInfo.m_dwTime);
	//转换成可读字符串
	strDownloadSpeed = m_objTaskManager.convertToAboutContentLength(llLengthPerSecond);
	strDownloadSpeed.append("/S");
	objDownloadInfo.m_strSpeed = strDownloadSpeed;

	//计算即时下载进度
	dPercent = objDownloadInfo.m_llTotalDownloadedLength*1.0/m_objTaskManager.m_llContentLength;
	dPercent = ((int)(dPercent*10000))/100.0;
	objDownloadInfo.m_dPercent = dPercent;

	//计算平均下载速度
	string strAverageSpeed;
	strAverageSpeed = m_objTaskManager.convertToAboutContentLength(objDownloadInfo.getAverageSpeed());
	strAverageSpeed.append("/S");
	objDownloadInfo.m_strAverageSpeed = strAverageSpeed;

	//保存进成员变量
	m_objDownloadInfo = objDownloadInfo;
	//LOG(INFO) << " Percent: " << dPercent << "%, Speed: " << strDownloadSpeed.c_str() << ",AverageSpeed: " << strAverageSpeed;
	return 1;
}

//下载者子线程
unsigned _stdcall  CDownloader::downloaderThread (void * pParam){
	CDownloader * p = (CDownloader * )pParam;
	double eachTPercent = 0.0;
	while(true){
		//结束下载
		if (p->m_nDownloadStatus == DOWNLOAD_STOP)
		{
			//仍有下载线程在处理
			if (p->m_objThreadManager.haveThreadsRun())
			{
				LOG(INFO) << " Stopping Download," << "Collect Threads";
				//回收线程
				p->m_objThreadManager.collectThreads();
				//回收任务
				p->m_objTaskManager.collectTasks();
				//更新下载信息
				p->getDownloadInfo();
			}
			//所有线程都空闲，校验任务状态，若任务未完成，生成任务信息文件。结束线程，结束下载。
			else{
				if (!p->m_objTaskManager.isTasksFinished())
				{
					LOG(INFO) << " Stopping Download," << "Mission Failed";
					p->m_objTaskManager.writeToFile();
				}
				p->m_objThreadManager.stopThreads();
				//更新下载信息
				p->getDownloadInfo();
				LOG(INFO) <<" Stopping Download,"<<"Update DownloadInfo";
				p->m_objTaskManager.writeToFile();
				LOG(INFO) << " Download Stopped！";
				break;
			}
		}
		//暂停下载
		else if (p->m_nDownloadStatus == DOWNLOAD_PAUSE)
		{
			//休眠1秒
			Sleep(1);
		}
		//下载
		else if (p->m_nDownloadStatus == DOWNLOAD_RUN)
		{
			if (p->m_objDownloadInfo.m_dPercent/10 > eachTPercent)
			{	
				eachTPercent = eachTPercent + 1;
				p->m_objTaskManager.writeToFile();
			}
			//如果仍有下载任务需要处理，或仍有线程在处理下载任务，表示下载仍没有完成。
			if (p->m_objTaskManager.haveTasksNotComplete() || p->m_objThreadManager.haveThreadsRun())
			{
				//LOG(INFO) << " download run !" ;
				//是否从远程下载置为true
				p->m_bIsDownloadedFromRemote = true;
				//回收线程
				p->m_objThreadManager.collectThreads();
				//回收任务
				p->m_objTaskManager.collectTasks();
				//获取下载信息
				p->getDownloadInfo();
				//有任务需要处理且有线程空闲时，则将空闲线程分配任务。
				while(p->m_objTaskManager.haveTasksTodo()&&p->m_objThreadManager.haveThreadsFree()){
					p->m_objThreadManager.giveTaskToAThreadTodo(p->m_objTaskManager.popOneTaskTodo());
				}
				//WaitForSingleObject(p->m_objThreadManager.m_hPauseEvent, INFINITE);
				Sleep(1);
			}
			//否则，下载完成。校验完成结果，退出。
			else{
				p->m_nDownloadStatus = DOWNLOAD_STOP;
				Sleep(100);
				LOG(INFO) << " Download will stop";
			}
		}
	}
	return 1;
}

int CDownloader::clear() {
	m_objTaskManager.clearTask();
	m_objThreadManager.clearThreads();
	m_nDownloadStatus = DOWNLOAD_PAUSE;
	m_objDownloadInfo.m_dwTime = GetTickCount();
	m_hDownloaderThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &CDownloader::downloaderThread, (LPVOID)this, 0, NULL);

	return 1;
}

