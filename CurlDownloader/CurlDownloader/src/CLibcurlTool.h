#pragma  once

#include <WinBase.h>
#include <CDownloadThread.h>

class CDownloadThread;

//libcurltool��װ��libcurl���һЩ���÷�����
class CLibcurlTool {
public:
	//һ��libcurltool����һ��curl interfaceָ��
	CURL * m_pCurl;
	CLibcurlTool(){
		//��ʼ��curl interfaceָ��
		m_pCurl = curl_easy_init();
	}
	~CLibcurlTool(){
		//ʹ��������ͷ�
		curl_easy_cleanup(m_pCurl);
	}
	//curl������ת��
	int figureError(CURLcode err);	
	//http��ȡ����
	CURLcode httpGetContent(const char * strUrl , long long llStartPos , long long llEndPos , char * pResponseData);    
	//http��ȡheader��һ���ȡǰ500�ֽڵ����ݣ�
	CURLcode httpGetHeader(const char * strUrl , char * pResponseData);	
	//http�����ļ�
	/*CURLcode httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDivide * pDivide);*/
	//http�����ļ�
	CURLcode httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDownloadThread * pThread);	
	//httpget
	CURLcode httpGet(const char * strUrl , long long llStartPos , long long llEndPos , int nHeaderFlag , char * pResponseData);
};