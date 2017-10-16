#pragma  once

#define KILOBYTES (1024)
#define MEGABYTES (1024*KILOBYTES)
#define GIGABYTES (1024*MEGABYTES)
#define TERABYTES (1024*GIGABYTES)


#include <CTaskManager.h>
#include <CDownloadTask.h>

using namespace std;

enum errorcode {
	//û�д���
	noerror = 1,
	//��Զ���ļ���һ��
	inconsisdent = 2,
	//���ػ�����Ϣ����
	dividerror = 3,
	//�������ļ�����
	downloadederror = 4,
	//�ļ���������
	filerror = 5,
	//Զ���ļ�����
	remotefilerror = 6,
	//��������·������
	localfilerror = 7,
};

class CDownloadTaskManager : public CTaskManager {
public:
	//����������Ϣ�ļ���ַ
	string m_strTaskInfoFilePath;
	//����Զ�̵�ַurl
	string m_strRemotePath;
	//���ر��ص�ַ
	string m_strLocalPath;
	//�����ļ���С���ֽ���
	long long m_llContentLength;
	//����ʱ��
	long long m_llDownloadTime;

	CDownloadTaskManager(const char * strRemotePath,const char * strLocalPath,const char * strTaskInfoFilePath);
	//������������--�½�����������
	errorcode generateDownloadTask();	
	//���ļ��м�����������--�Ѵ��ڵ�����
	int loadTaskFromFile();
	//д�����������ļ�
	int writeToFile();
	//������������
	errorcode loadDownloadTask();	
	//����������Ϣ�ļ��Ƿ����
	int isTaskInfoFileExisted(); 
	//��ȡ�����ļ��Ĵ�С
	long long getContentLength();
	//ת���ֽ���������λ���ַ���
	string convertLLContentLengthToString(long long llContentLength);
	//����λ�ַ���ת���ɴ�Լֵ�ַ���
	string convertToAboutContentLength(string strContentLength);
	//�ֽ���ת���ɴ�Լֵ�ַ���
	string convertToAboutContentLength(long long llContentLength);
	//У������������Ϣ
	errorcode checkTaskInfo();
	//�����Ƿ����
	int isTasksFinished();
	//��ȡ����������
	long long getTotalDownloadedLength();
	//��ȡ������Ϣ--���ؽ��ȣ������ٶȣ�ʣ������ʱ��ȡ�
	int getDownloadInfo();
	//�����������
	int clearDownloadTask();
	//ˢ������ʱ��
	long long freshDownloadTime(int nTime);

};