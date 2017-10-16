#pragma  once

#include <WinBase.h>
#include <CDownloadThread.h>

class CDownloadThread;

//libcurltool封装了libcurl里的一些常用方法。
class CLibcurlTool {
public:
	//一个libcurltool持有一个curl interface指针
	CURL * m_pCurl;
	CLibcurlTool(){
		//初始化curl interface指针
		m_pCurl = curl_easy_init();
	}
	~CLibcurlTool(){
		//使用完必须释放
		curl_easy_cleanup(m_pCurl);
	}
	//curl错误码转换
	int figureError(CURLcode err);	
	//http获取内容
	CURLcode httpGetContent(const char * strUrl , long long llStartPos , long long llEndPos , char * pResponseData);    
	//http获取header（一般获取前500字节的数据）
	CURLcode httpGetHeader(const char * strUrl , char * pResponseData);	
	//http下载文件
	/*CURLcode httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDivide * pDivide);*/
	//http下载文件
	CURLcode httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDownloadThread * pThread);	
	//httpget
	CURLcode httpGet(const char * strUrl , long long llStartPos , long long llEndPos , int nHeaderFlag , char * pResponseData);
};