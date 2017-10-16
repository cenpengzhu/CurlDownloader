#include "stdafx.h"
#include "CLibcurlTool.h"



using namespace std;

//curl请求回调函数
size_t writeData(void *ptr, size_t size, size_t nmemb, void * pstrstrResponseData){

	stringstream * p = (stringstream *)pstrstrResponseData;
	(*p)<<(char *)ptr;

	//strcpy((char *)pResponseData,(char *)ptr);

	return nmemb;

}  

size_t downloadFunc(void *ptr, size_t size, size_t nmemb, void * pThread){
	CDownloadThread * pDownloadThread = (CDownloadThread  *)pThread;
	CDownloadTask * pDownloadTask = (CDownloadTask *)pDownloadThread->m_pThreadTask;

	//写入文件大小
	size_t nWritten;

	//日志文件字符串流
	stringstream strstrLog;

	//锁定互斥量
	WaitForSingleObject(pDownloadThread->m_hDownloadThreadMutex,INFINITE);

	//写入文件
	if ((pDownloadTask->m_llDownloadedPos+nmemb) <= pDownloadTask->m_llEndPos){
		fseek (pDownloadThread->m_pLocalFile, pDownloadTask->m_llDownloadedPos, SEEK_SET); 
		nWritten = fwrite (ptr, size, nmemb, pDownloadThread->m_pLocalFile);
		pDownloadTask->m_llDownloadedPos = pDownloadTask->m_llDownloadedPos + nmemb;
	} 
	else {
		fseek (pDownloadThread->m_pLocalFile, pDownloadTask->m_llDownloadedPos, SEEK_SET); 
		nWritten = fwrite (ptr, 1, pDownloadTask->m_llEndPos-pDownloadTask->m_llDownloadedPos+1, pDownloadThread->m_pLocalFile);
		pDownloadTask->m_llDownloadedPos = pDownloadTask->m_llEndPos;
	}

	//测试
	//LOG(INFO)<<"当前任务："<<pDownloadTask->m_nTaskId<<"已下载到:"<<pDownloadTask->m_llDownloadedPos<<"本次收到:"<<nmemb<<"本次写入"<<nWritten;
	//fputs(strstrLog.str().c_str(),pDownloadThread->m_pLogFile);

	//解锁互斥量
	ReleaseMutex(pDownloadThread->m_hDownloadThreadMutex);

	if (nWritten != nmemb)
	{
		//LOG(INFO) << " Thread: " << pDownloadTask->m_nTaskId << " Write Error. Task id：" << pDownloadTask->m_nTaskId << " Download to:" << pDownloadTask->m_llDownloadedPos << " Receive:" << nmemb << " Write:" << nWritten << " Task EndPos:" << pDownloadTask->m_llEndPos;
		//fputs(strstrLog.str().c_str(),pDownloadThread->m_pLogFile);
	}

	return nWritten;
}

//curl错误码转换
int CLibcurlTool::figureError(CURLcode err){
	switch (err) 
	{
	case CURLE_UNSUPPORTED_PROTOCOL:
		LOG(INFO) << " ProtoControl Unsupport!";
		break;
	case CURLE_FAILED_INIT :		
		LOG(INFO) << " Init Failed!";
		break;
	case CURLE_URL_MALFORMAT :
		LOG(INFO) << " Curl RemotePath Error";
		break;
	case CURLE_NOT_BUILT_IN :
		LOG(INFO) << " libcurl InitFailed，Please Try again!";
		break;
	case CURLE_COULDNT_RESOLVE_PROXY :
		LOG(INFO) << " Cannt resovlve proxy!";
		break;
	case CURLE_COULDNT_RESOLVE_HOST :
		LOG(INFO) << " cannt resovlve server!";
		break;
	case CURLE_COULDNT_CONNECT :
		LOG(INFO) << " Curl Connect Server Error";
		break;
	case CURLE_FTP_WEIRD_SERVER_REPLY :
		LOG(INFO) << " FTP服务器非法回复";
		break;
	case CURLE_REMOTE_ACCESS_DENIED :
		LOG(INFO) << " Acess Denied!";
		break;
	case CURLE_FTP_ACCEPT_FAILED :
		LOG(INFO) << " FTP连接失败";
		break;
	case CURLE_FTP_WEIRD_PASS_REPLY :
		LOG(INFO) << " FTP服务器未知回复";
		break;
	case CURLE_FTP_ACCEPT_TIMEOUT :
		LOG(INFO) << " FTP连接超时";
		break;
	case CURLE_FTP_WEIRD_PASV_REPLY :
		LOG(INFO) << " Curl unknow error!";
		break;
	case CURLE_FTP_WEIRD_227_FORMAT:
		LOG(INFO) << " Curl unknow error!";
		break;
	case CURLE_FTP_CANT_GET_HOST :
		LOG(INFO) << " 内部故障";
		break;
	case CURLE_HTTP2 :
		LOG(INFO) << " HTTP2错误";
		break;
	case CURLE_FTP_COULDNT_SET_TYPE :
		LOG(INFO) << " FTP获取传输类型失败";
		break;
	case CURLE_PARTIAL_FILE :
		LOG(INFO) << " 服务器返回文件大小非法";
		break;
	case CURLE_HTTP_RETURNED_ERROR :
		LOG(INFO) << " HTTP请求错误";
		break;
	case CURLE_WRITE_ERROR :
		LOG(INFO) << " 接收数据错误";
		break;
	case CURLE_UPLOAD_FAILED :
		LOG(INFO) << " 上传数据错误";
		break;
	case CURLE_READ_ERROR :
	    LOG(INFO) << " 本地文件读取错误";
		break;
	case CURLE_OUT_OF_MEMORY :
		LOG(INFO) << " 内存不足";
		break;
	case CURLE_OPERATION_TIMEDOUT :
		LOG(INFO) << " Curl Time out";
		break;
	case CURLE_FTP_PORT_FAILED :
		LOG(INFO) << " FTP端口失败";
		break;
	case CURLE_RANGE_ERROR :
	    LOG(INFO) << " 请求的数据范围错误";
		break;
	default:
		LOG(INFO) << " Curl Else Error";
		break;
	}
	return 1;
}


//http获取内容
CURLcode CLibcurlTool::httpGetContent(const char * strUrl , long long llStartPos , long long llEndPos , char * pResponseData){
	return httpGet(strUrl,llStartPos,llEndPos,0,pResponseData);
}

//http获取header（一般获取前500字节的数据）
CURLcode CLibcurlTool::httpGetHeader(const char * strUrl , char * pResponseData){
	return httpGet(strUrl,0,2000,1,pResponseData);
}

//http下载文件
CURLcode CLibcurlTool::httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDownloadThread * pThread){
	CURLcode err;

	//格式化下载文件范围字符串
	string strRange;
	stringstream strstrRange;
	strstrRange<<llStartPos<<"-"<<llEndPos;
	strRange = strstrRange.str();
	//LOG(INFO) << " Request Range: " << strRange.c_str();

	//打开日志文件
	string strLogFilePath = "c:\\log";
	//strLogFilePath.push_back('0'+pDivide->m_nDivideNO);
	strLogFilePath = strLogFilePath + ".log";
	//pDivide->m_pLogFile = fopen(strLogFilePath.c_str(),"w");

	//curl设置url
	curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl); 

	//curl设置不发送其他信号
	curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1);

	//curl设置下载文件范围
	curl_easy_setopt(m_pCurl, CURLOPT_RANGE, strRange.c_str());

    //curl设置重定向
	//curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

	//curl设置写数据回调函数（在数据返回时调用）
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, downloadFunc);

	//curl设置写数据buff（数据返回时最终的存储位置）
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)pThread);

	//curl设置超时
	curl_easy_setopt(m_pCurl, CURLOPT_TIMEOUT, 10);

	//curl设置连接超时
	curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 3);

	//curl设置支持https
    err = curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);   //不验证证书和HOST
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);


    if (err != CURLE_OK){
		figureError(err);
		return err;
	}



	//curl interface执行
	err = curl_easy_perform(m_pCurl);

	if (err != CURLE_OK )
	{
		figureError(err);
	}
	return err;
}

//httpget
CURLcode CLibcurlTool::httpGet(const char * strUrl , long long llStartPos , long long llEndPos , int nHeaderFlag , char * pResponseData) {

	CURLcode objCurlcode;

	//格式化下载文件范围字符串
	string strRange;
	stringstream strstrRange;
	strstrRange<<llStartPos<<"-"<<llEndPos;
	strRange = strstrRange.str();

	//创建字符串流
	stringstream strstrResponseData;

	//curl设置url
	curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl); 

	//curl设置不发送其他信号
	curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1);

	//curl设置追踪重定向
	//curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

	//curl设置是否获取http头
	if (nHeaderFlag == 1)
	{
		curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1);
		curl_easy_setopt(m_pCurl, CURLOPT_HEADER, 1);
	}
	//curl_easy_setopt(m_pCurl, CURLOPT_HEADER, 1);

	//curl设置下载文件范围
	//curl_easy_setopt(m_pCurl, CURLOPT_RANGE, strRange.c_str());

	//curl设置写数据回调函数（在数据返回时调用）
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION , writeData);

	//curl设置写数据buff（数据返回时最终的存储位置）
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA , (void *)&strstrResponseData);

	//curl设置支持https
	objCurlcode = curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, false);

	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);   //不验证证书和HOST
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

	if (objCurlcode != CURLE_OK) {
		figureError(objCurlcode);
		return objCurlcode;
	}

	//curl interface执行
	objCurlcode = curl_easy_perform(m_pCurl);

	//curl 错误处理

	strncpy(pResponseData,strstrResponseData.str().c_str(),2000);

	if (objCurlcode != CURLE_OK )
	{
		figureError(objCurlcode);
	}

	return objCurlcode;
}

	//http下载文件
	/*CURLcode httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDivide * pDivide){
		CURLcode err;

		//格式化下载文件范围字符串
		string strRange;
		stringstream strstrRange;
		strstrRange<<llStartPos<<"-"<<llEndPos;
		strRange = strstrRange.str();

		//打开日志文件
		string strLogFilePath = "c:\\log";
		strLogFilePath.push_back('0'+pDivide->m_nDivideNO);
		strLogFilePath = strLogFilePath + ".log";
		pDivide->m_pLogFile = fopen(strLogFilePath.c_str(),"w");

		//curl设置url
		curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl); 

		//curl设置不发送其他信号
		curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1);

		//curl设置下载文件范围
		curl_easy_setopt(m_pCurl, CURLOPT_RANGE, strRange.c_str());

		//curl设置写数据回调函数（在数据返回时调用）
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, downloadData);

		//curl设置写数据buff（数据返回时最终的存储位置）
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)pDivide);

		//curl设置下载是显示进度
		curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0L);

		//curl设置进度回调函数
		curl_easy_setopt(m_pCurl, CURLOPT_XFERINFOFUNCTION, getProgress);

		//curl设置进度回调函数传参
		curl_easy_setopt(m_pCurl, CURLOPT_XFERINFODATA, (void *)pDivide);

		//curl interface执行
		err = curl_easy_perform(m_pCurl);

		if (err != CURLE_OK )
		{
			figureError(err);
		}
		else{
			stringstream strstrLog;
			strstrLog<<"线程："<<pDivide->m_nDivideNO<<"成功结束！";
			fputs(strstrLog.str().c_str(),pDivide->m_pLogFile);
		}

		return err;

	}*/

//curl下载回调函数
/*size_t downloadData(void *ptr, size_t size, size_t nmemb, void * pDivide){

	CDivide * p = (CDivide  *)pDivide;

	//写入文件大小
	size_t nWritten;

	//日志文件字符串流
	stringstream strstrLog;

	//锁定互斥量
	WaitForSingleObject(p->m_hDownloadThreadMutex,INFINITE);

	//测试
	//strstrLog<<"下载回调函数输出,收到数据:"<<nmemb<<"字节";

    //写入文件
	if (p->m_llDownloadToPos+nmemb <= p->m_llEndPos){
		fseek (p->m_pLocalFile, p->m_llDownloadToPos, SEEK_SET); 
		if (size != 1)
		{
			//LOG(INFO)<<"size!=1";
		}
		nWritten = fwrite (ptr, size, nmemb, p->m_pLocalFile);
		if (nWritten != nmemb)
		{
			//LOG(INFO)<<"写错误";
		}
		p->m_llDownloadToPos = p->m_llDownloadToPos + nmemb;
	} 
	else {
		fseek (p->m_pLocalFile, p->m_llDownloadToPos, SEEK_SET); 
		nWritten = fwrite (ptr, 1, p->m_llEndPos-p->m_llDownloadToPos+1, p->m_pLocalFile);
		p->m_llDownloadToPos = p->m_llEndPos;
	}
	strstrLog<<"线程:"<<p->m_nDivideNO<<"下载到:"<<p->m_llDownloadToPos<<"写入:"<<nWritten<<"收到:"<<nmemb;

	fputs(strstrLog.str().c_str(),p->m_pLogFile);
	
	//解锁互斥量
	ReleaseMutex(p->m_hDownloadThreadMutex);

	//测试
	////LOG(INFO)<<"线程："<<p->m_nDivideNO<<"下载回调函数输出，已下载:"<<p->m_llDownloadToPos<<"总共需下载："<< p->m_llEndPos-p->m_llStartPos+1;

	return nWritten;

} */


//curl下载进度回调函数
int getProgress(void *ptr, curl_off_t  dlTotal, curl_off_t  dlNow, curl_off_t  ulTotal, curl_off_t  ulNow)  
{
	////LOG(INFO)<<"进度回调函数信息下载总数："<<dlTotal<<", 已下载数:"<<dlNow;
	return 0;
}
