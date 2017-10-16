#pragma once

#include <tchar.h>
#include <CThread.h>
#include <vector>

class CThreadManager {
public:
	//线程总数
	int m_nThreadsCount;
	//线程数组
	vector<CThread *> m_vecThreads;
	//空闲线程数组
	vector<CThread *> m_vecFreeThreads;
	//繁忙线程数组
	vector<CThread *> m_vecBusyThreads;
	//互斥量
	HANDLE m_hDownloadThreadMutex;
	//暂停事件
	HANDLE m_hPauseEvent;

	CThreadManager(){
		//创建互斥量
		m_hDownloadThreadMutex = CreateMutexW(NULL,false,_T("mutex_for_downloadthread"));
		m_hPauseEvent = CreateEvent(NULL, 0, 1, NULL);
		m_nThreadsCount = 0;
	}

	//用于停止下载时，结束所有线程。
	int stopThreads(){
		for(vector<CThread *>::const_iterator iterPThread = m_vecFreeThreads.begin();iterPThread != m_vecFreeThreads.end();iterPThread++){
			CThread * p = *iterPThread;
			p->m_nThreadStatus = THREAD_STOP;
		}
		return 1;
	}

	int collectThreads(){
		for(vector<CThread *>::const_iterator iterPThread = m_vecBusyThreads.begin();iterPThread != m_vecBusyThreads.end();){
			CThread * p = *iterPThread;
			if (p->m_nThreadStatus == THREAD_PAUSE)
			{
				iterPThread = m_vecBusyThreads.erase(iterPThread);
				m_vecFreeThreads.push_back(p);
			}
			else{
				iterPThread++;
			}
		}
		return 1;
	}

	CThread * getOneFreeThreads(){
		CThread * pThread;
		if (m_vecFreeThreads.size()!=0)
		{
			pThread = m_vecFreeThreads.back();
			m_vecFreeThreads.pop_back();
			return pThread;
		}
		else{
			return NULL;
		}

	}

	int giveTaskToAThreadTodo(CTask * pTask){
		CThread * pThread;
		pThread = getOneFreeThreads();
		if (pThread != NULL)
		{
			pThread->doTask(pTask);
			m_vecBusyThreads.push_back(pThread);
			return 1;
		}
		else {
			return 0;
		}
	}

	virtual int createThreads(int nThreadsCount){
		CThread * pThread;
		for (int i = 1; i <= nThreadsCount ; i++)
		{
			pThread = new CThread(m_hDownloadThreadMutex, m_hPauseEvent);
			pThread->init();
			m_vecFreeThreads.push_back(pThread);
		}
		return nThreadsCount;
	}

	int haveThreadsRun(){
		if (m_vecBusyThreads.size() != 0)
		{
			return 1;
		}
		else{
			return 0;
		}
	}

	int haveThreadsFree(){
		if (m_vecFreeThreads.size() != 0)
		{
			return 1;
		}
		else{
			return 0;
		}
	}



};