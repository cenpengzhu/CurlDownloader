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

//���س�ʼ��
errorcode CDownloader::downloadInit(){
	//�����߳�
	if (m_objThreadManager.createThreads(m_nThreadCounts) == 0)
	{
		return localfilerror;
	}

	//����������Ϣ
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

//���ؿ�ʼ
errorcode CDownloader::downloadBegin () {
	m_nDownloadStatus = DOWNLOAD_RUN;
	return noerror;
}

//����ֹͣ
errorcode CDownloader::downloadStop(){
	m_nDownloadStatus = DOWNLOAD_STOP;
	return noerror;
}

//������ͣ
errorcode CDownloader::downloadPause(){
	m_nDownloadStatus = DOWNLOAD_PAUSE;
	return noerror;
}

//��ȡ������Ϣ
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

	//��ȡ��ˢ��������ʱ��
	objDownloadInfo.m_llTotalDownloadTime = m_objTaskManager.freshDownloadTime(objDownloadInfo.m_dwTime - m_objDownloadInfo.m_dwTime);
	
	//���㼴ʱ�����ٶ�
	llLengthPerSecond = (objDownloadInfo.m_llTotalDownloadedLength - m_objDownloadInfo.m_llTotalDownloadedLength)*1000/(objDownloadInfo.m_dwTime - m_objDownloadInfo.m_dwTime);
	//ת���ɿɶ��ַ���
	strDownloadSpeed = m_objTaskManager.convertToAboutContentLength(llLengthPerSecond);
	strDownloadSpeed.append("/S");
	objDownloadInfo.m_strSpeed = strDownloadSpeed;

	//���㼴ʱ���ؽ���
	dPercent = objDownloadInfo.m_llTotalDownloadedLength*1.0/m_objTaskManager.m_llContentLength;
	dPercent = ((int)(dPercent*10000))/100.0;
	objDownloadInfo.m_dPercent = dPercent;

	//����ƽ�������ٶ�
	string strAverageSpeed;
	strAverageSpeed = m_objTaskManager.convertToAboutContentLength(objDownloadInfo.getAverageSpeed());
	strAverageSpeed.append("/S");
	objDownloadInfo.m_strAverageSpeed = strAverageSpeed;

	//�������Ա����
	m_objDownloadInfo = objDownloadInfo;
	//LOG(INFO) << " Percent: " << dPercent << "%, Speed: " << strDownloadSpeed.c_str() << ",AverageSpeed: " << strAverageSpeed;
	return 1;
}

//���������߳�
unsigned _stdcall  CDownloader::downloaderThread (void * pParam){
	CDownloader * p = (CDownloader * )pParam;
	double eachTPercent = 0.0;
	while(true){
		//��������
		if (p->m_nDownloadStatus == DOWNLOAD_STOP)
		{
			//���������߳��ڴ���
			if (p->m_objThreadManager.haveThreadsRun())
			{
				LOG(INFO) << " Stopping Download," << "Collect Threads";
				//�����߳�
				p->m_objThreadManager.collectThreads();
				//��������
				p->m_objTaskManager.collectTasks();
				//����������Ϣ
				p->getDownloadInfo();
			}
			//�����̶߳����У�У������״̬��������δ��ɣ�����������Ϣ�ļ��������̣߳��������ء�
			else{
				if (!p->m_objTaskManager.isTasksFinished())
				{
					LOG(INFO) << " Stopping Download," << "Mission Failed";
					p->m_objTaskManager.writeToFile();
				}
				p->m_objThreadManager.stopThreads();
				//����������Ϣ
				p->getDownloadInfo();
				LOG(INFO) <<" Stopping Download,"<<"Update DownloadInfo";
				p->m_objTaskManager.writeToFile();
				LOG(INFO) << " Download Stopped��";
				break;
			}
		}
		//��ͣ����
		else if (p->m_nDownloadStatus == DOWNLOAD_PAUSE)
		{
			//����1��
			Sleep(1);
		}
		//����
		else if (p->m_nDownloadStatus == DOWNLOAD_RUN)
		{
			if (p->m_objDownloadInfo.m_dPercent/10 > eachTPercent)
			{	
				eachTPercent = eachTPercent + 1;
				p->m_objTaskManager.writeToFile();
			}
			//�����������������Ҫ�����������߳��ڴ����������񣬱�ʾ������û����ɡ�
			if (p->m_objTaskManager.haveTasksNotComplete() || p->m_objThreadManager.haveThreadsRun())
			{
				//LOG(INFO) << " download run !" ;
				//�Ƿ��Զ��������Ϊtrue
				p->m_bIsDownloadedFromRemote = true;
				//�����߳�
				p->m_objThreadManager.collectThreads();
				//��������
				p->m_objTaskManager.collectTasks();
				//��ȡ������Ϣ
				p->getDownloadInfo();
				//��������Ҫ���������߳̿���ʱ���򽫿����̷߳�������
				while(p->m_objTaskManager.haveTasksTodo()&&p->m_objThreadManager.haveThreadsFree()){
					p->m_objThreadManager.giveTaskToAThreadTodo(p->m_objTaskManager.popOneTaskTodo());
				}
				//WaitForSingleObject(p->m_objThreadManager.m_hPauseEvent, INFINITE);
				Sleep(1);
			}
			//����������ɡ�У����ɽ�����˳���
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

