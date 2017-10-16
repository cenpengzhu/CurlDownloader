#pragma  once
#include <stdafx.h>
#include <CDownloadTaskManager.h>
#include <CDownloadThreadManager.h>
#include <CDownloadInfo.h>

using namespace std;

//����״̬��
#define DOWNLOAD_RUN 1
#define DOWNLOAD_PAUSE 2
#define DOWNLOAD_STOP 0

class  CDownloader {
public:
	//Զ�����ص�ַurl
	string m_strRemotePath;
	//�������ص�ַ
	string m_strLocalPath;
	//����������Ϣ�ļ���ַ
	string m_strTaskInfoFilePath;
	//�̳߳��߳���
	int m_nThreadCounts;
	//����״̬
	int m_nDownloadStatus;
	//�������߳̾��
	HANDLE m_hDownloaderThreadHandle;
	//�Ƿ��Զ������
	bool m_bIsDownloadedFromRemote;

	//������Ϣ
	CDownloadInfo m_objDownloadInfo;

	CDownloadTaskManager m_objTaskManager;
	CDownloadThreadManager m_objThreadManager;

	CDownloader(const char * strRemotePath,const char * strLocalPath,const char * strTaskInfoFilePath, int nThreadCounts = 1);
	~CDownloader(){}	

	//���س�ʼ��
	errorcode downloadInit();
	//���ؿ�ʼ
	errorcode downloadBegin();	
	//����ֹͣ
	errorcode downloadStop();	
	//������ͣ
	errorcode downloadPause();
	//��ȡ������Ϣ
	int getDownloadInfo();
	//����
	int clear();
	//���������߳�
	static unsigned _stdcall  downloaderThread (void * pParam);

};

