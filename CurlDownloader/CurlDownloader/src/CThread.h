#pragma  once
#include <process.h>
#include <CTask.h>
#include <string>
#include <windows.h>
#include <sstream>
#include <atomic>

using namespace std;

//�߳�״̬��
#define THREAD_PAUSE 2
#define THREAD_RUN 1
#define THREAD_STOP 0

class CThread {
public:
	//�߳̾��
	HANDLE m_hThreadHandle;
	//�߳�״̬
	std::atomic<int> m_nThreadStatus;
	//�߳�����
	CTask * m_pThreadTask;
	//д�ļ����������
	HANDLE m_hDownloadThreadMutex;
	//RUN�¼����
	HANDLE m_hRunEvent;
	//Pause�¼����
	HANDLE m_hPauseEvent;
	//��־�ļ�
	FILE * m_pLogFile;
	//��־�ļ�·��
	string m_strLogFilePath;



	CThread(HANDLE hDownloadThreadMutex,HANDLE hPauseEvent){
		m_hDownloadThreadMutex = hDownloadThreadMutex;
		m_nThreadStatus = THREAD_PAUSE;
		m_hRunEvent = CreateEvent(NULL, 0, 0, NULL);
		m_hPauseEvent = hPauseEvent;
	}
	//�̺߳���
	static unsigned _stdcall  ThreadFunc (void * pParam){
		CThread * p = (CThread * )pParam;
		while(true){ 
			if (p->m_nThreadStatus == THREAD_PAUSE)
			{
				WaitForSingleObject(p->m_hRunEvent, INFINITE);
			}
			else if (p->m_nThreadStatus == THREAD_STOP)
			{
				break;
			}
			else if (p->m_nThreadStatus == THREAD_RUN)
			{
				p->taskBusiness();
				p->m_nThreadStatus = THREAD_PAUSE;
				//SetEvent(p->m_hPauseEvent);
			}
		}
		return 1;
	}

	//��ʼ��
	int init(){
		m_hThreadHandle = (HANDLE)_beginthreadex(NULL,0,&CThread::ThreadFunc,(LPVOID)this,0,NULL);
		stringstream strstrLogFilePath;
		strstrLogFilePath<<m_hThreadHandle<<".log";
		m_strLogFilePath = "c:\\";
		m_strLogFilePath.append(strstrLogFilePath.str().c_str());

		//m_pLogFile = fopen(m_strLogFilePath.c_str(),"w");
		//if (m_pLogFile == NULL)
		//{
			//return 0;
	//	}
		return 1;
	}
	//ִ��Task
	int doTask(CTask * pTask){
		setTask(pTask);
		Run();
		return 1;
	}
	//����Task
	int setTask(CTask * pTask){
		m_pThreadTask = pTask;
		return 1;
	}
	//��ʼִ��
	int Run(){
		m_nThreadStatus = THREAD_RUN;
		SetEvent(m_hRunEvent);
		return 1;
	}

	//�����ҵ����
	virtual int taskBusiness() {
		return 1;
	}

};