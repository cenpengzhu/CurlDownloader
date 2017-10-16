#pragma  once
#include <CThreadManager.h>
#include <CDownloadThread.h>


class CDownloadThreadManager : public CThreadManager {
public:
	//本地文件指针
	FILE * m_pLocalFile;
	//本地文件地址
	string m_strLocalFilePath;

	CDownloadThreadManager(const char * strLocalFilePath){
		m_strLocalFilePath = strLocalFilePath;
	}
	~CDownloadThreadManager(){
		if (m_pLocalFile != NULL)
		{
			fclose(m_pLocalFile);
		}
	}


	virtual int createThreads(int nThreadsCount){
		CDownloadThread * pThread;	
		if (isLocalFileExited())
		{
	        LOG(INFO)<<m_strLocalFilePath.c_str();
			m_pLocalFile = fopen(m_strLocalFilePath.c_str(),"rb+");
		}
		else{
			m_pLocalFile =fopen(m_strLocalFilePath.c_str(),"wb+");
		}
		if (m_pLocalFile == NULL)
		{
			return 0;
		}
		for (int i = 1; i <= nThreadsCount ; i++)
		{
			pThread = new CDownloadThread(m_hDownloadThreadMutex,m_pLocalFile,m_hPauseEvent);
			pThread->init();
			m_vecFreeThreads.push_back(pThread);
		}
		return nThreadsCount;
	}
	int clearThreads() {
		if (m_pLocalFile != NULL)
		{
			fclose(m_pLocalFile);
		}
		m_vecBusyThreads.clear();
		m_vecFreeThreads.clear();
		m_vecThreads.clear();
		return 1;
	}

private:
	int isLocalFileExited(){
		fstream fsLocalFile;
		fsLocalFile.open(m_strLocalFilePath.c_str(),ios::in);

		if (!fsLocalFile) {
			return 0;
		}
		else {
			return 1;
		}
	}

};

