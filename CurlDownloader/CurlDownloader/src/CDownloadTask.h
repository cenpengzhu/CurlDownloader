#pragma  once
#include <CTask.h>
#include <string>

class CDownloadTask : public CTask {
public:
	//���ص�ַURL
	string m_strRemotePath;
	//���ص�ַ
	string m_strLocalPath;
	//��ʼλ��
	long long m_llStartPos;
	//����λ��
	long long m_llEndPos;
	//������λ��
	long long m_llDownloadedPos;
};