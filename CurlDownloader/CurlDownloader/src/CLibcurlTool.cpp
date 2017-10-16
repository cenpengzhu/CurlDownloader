#include "stdafx.h"
#include "CLibcurlTool.h"



using namespace std;

//curl����ص�����
size_t writeData(void *ptr, size_t size, size_t nmemb, void * pstrstrResponseData){

	stringstream * p = (stringstream *)pstrstrResponseData;
	(*p)<<(char *)ptr;

	//strcpy((char *)pResponseData,(char *)ptr);

	return nmemb;

}  

size_t downloadFunc(void *ptr, size_t size, size_t nmemb, void * pThread){
	CDownloadThread * pDownloadThread = (CDownloadThread  *)pThread;
	CDownloadTask * pDownloadTask = (CDownloadTask *)pDownloadThread->m_pThreadTask;

	//д���ļ���С
	size_t nWritten;

	//��־�ļ��ַ�����
	stringstream strstrLog;

	//����������
	WaitForSingleObject(pDownloadThread->m_hDownloadThreadMutex,INFINITE);

	//д���ļ�
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

	//����
	//LOG(INFO)<<"��ǰ����"<<pDownloadTask->m_nTaskId<<"�����ص�:"<<pDownloadTask->m_llDownloadedPos<<"�����յ�:"<<nmemb<<"����д��"<<nWritten;
	//fputs(strstrLog.str().c_str(),pDownloadThread->m_pLogFile);

	//����������
	ReleaseMutex(pDownloadThread->m_hDownloadThreadMutex);

	if (nWritten != nmemb)
	{
		//LOG(INFO) << " Thread: " << pDownloadTask->m_nTaskId << " Write Error. Task id��" << pDownloadTask->m_nTaskId << " Download to:" << pDownloadTask->m_llDownloadedPos << " Receive:" << nmemb << " Write:" << nWritten << " Task EndPos:" << pDownloadTask->m_llEndPos;
		//fputs(strstrLog.str().c_str(),pDownloadThread->m_pLogFile);
	}

	return nWritten;
}

//curl������ת��
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
		LOG(INFO) << " libcurl InitFailed��Please Try again!";
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
		LOG(INFO) << " FTP�������Ƿ��ظ�";
		break;
	case CURLE_REMOTE_ACCESS_DENIED :
		LOG(INFO) << " Acess Denied!";
		break;
	case CURLE_FTP_ACCEPT_FAILED :
		LOG(INFO) << " FTP����ʧ��";
		break;
	case CURLE_FTP_WEIRD_PASS_REPLY :
		LOG(INFO) << " FTP������δ֪�ظ�";
		break;
	case CURLE_FTP_ACCEPT_TIMEOUT :
		LOG(INFO) << " FTP���ӳ�ʱ";
		break;
	case CURLE_FTP_WEIRD_PASV_REPLY :
		LOG(INFO) << " Curl unknow error!";
		break;
	case CURLE_FTP_WEIRD_227_FORMAT:
		LOG(INFO) << " Curl unknow error!";
		break;
	case CURLE_FTP_CANT_GET_HOST :
		LOG(INFO) << " �ڲ�����";
		break;
	case CURLE_HTTP2 :
		LOG(INFO) << " HTTP2����";
		break;
	case CURLE_FTP_COULDNT_SET_TYPE :
		LOG(INFO) << " FTP��ȡ��������ʧ��";
		break;
	case CURLE_PARTIAL_FILE :
		LOG(INFO) << " �����������ļ���С�Ƿ�";
		break;
	case CURLE_HTTP_RETURNED_ERROR :
		LOG(INFO) << " HTTP�������";
		break;
	case CURLE_WRITE_ERROR :
		LOG(INFO) << " �������ݴ���";
		break;
	case CURLE_UPLOAD_FAILED :
		LOG(INFO) << " �ϴ����ݴ���";
		break;
	case CURLE_READ_ERROR :
	    LOG(INFO) << " �����ļ���ȡ����";
		break;
	case CURLE_OUT_OF_MEMORY :
		LOG(INFO) << " �ڴ治��";
		break;
	case CURLE_OPERATION_TIMEDOUT :
		LOG(INFO) << " Curl Time out";
		break;
	case CURLE_FTP_PORT_FAILED :
		LOG(INFO) << " FTP�˿�ʧ��";
		break;
	case CURLE_RANGE_ERROR :
	    LOG(INFO) << " ��������ݷ�Χ����";
		break;
	default:
		LOG(INFO) << " Curl Else Error";
		break;
	}
	return 1;
}


//http��ȡ����
CURLcode CLibcurlTool::httpGetContent(const char * strUrl , long long llStartPos , long long llEndPos , char * pResponseData){
	return httpGet(strUrl,llStartPos,llEndPos,0,pResponseData);
}

//http��ȡheader��һ���ȡǰ500�ֽڵ����ݣ�
CURLcode CLibcurlTool::httpGetHeader(const char * strUrl , char * pResponseData){
	return httpGet(strUrl,0,2000,1,pResponseData);
}

//http�����ļ�
CURLcode CLibcurlTool::httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDownloadThread * pThread){
	CURLcode err;

	//��ʽ�������ļ���Χ�ַ���
	string strRange;
	stringstream strstrRange;
	strstrRange<<llStartPos<<"-"<<llEndPos;
	strRange = strstrRange.str();
	//LOG(INFO) << " Request Range: " << strRange.c_str();

	//����־�ļ�
	string strLogFilePath = "c:\\log";
	//strLogFilePath.push_back('0'+pDivide->m_nDivideNO);
	strLogFilePath = strLogFilePath + ".log";
	//pDivide->m_pLogFile = fopen(strLogFilePath.c_str(),"w");

	//curl����url
	curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl); 

	//curl���ò����������ź�
	curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1);

	//curl���������ļ���Χ
	curl_easy_setopt(m_pCurl, CURLOPT_RANGE, strRange.c_str());

    //curl�����ض���
	//curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

	//curl����д���ݻص������������ݷ���ʱ���ã�
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, downloadFunc);

	//curl����д����buff�����ݷ���ʱ���յĴ洢λ�ã�
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)pThread);

	//curl���ó�ʱ
	curl_easy_setopt(m_pCurl, CURLOPT_TIMEOUT, 10);

	//curl�������ӳ�ʱ
	curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 3);

	//curl����֧��https
    err = curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);   //����֤֤���HOST
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);


    if (err != CURLE_OK){
		figureError(err);
		return err;
	}



	//curl interfaceִ��
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

	//��ʽ�������ļ���Χ�ַ���
	string strRange;
	stringstream strstrRange;
	strstrRange<<llStartPos<<"-"<<llEndPos;
	strRange = strstrRange.str();

	//�����ַ�����
	stringstream strstrResponseData;

	//curl����url
	curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl); 

	//curl���ò����������ź�
	curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1);

	//curl����׷���ض���
	//curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1);

	//curl�����Ƿ��ȡhttpͷ
	if (nHeaderFlag == 1)
	{
		curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1);
		curl_easy_setopt(m_pCurl, CURLOPT_HEADER, 1);
	}
	//curl_easy_setopt(m_pCurl, CURLOPT_HEADER, 1);

	//curl���������ļ���Χ
	//curl_easy_setopt(m_pCurl, CURLOPT_RANGE, strRange.c_str());

	//curl����д���ݻص������������ݷ���ʱ���ã�
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION , writeData);

	//curl����д����buff�����ݷ���ʱ���յĴ洢λ�ã�
	curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA , (void *)&strstrResponseData);

	//curl����֧��https
	objCurlcode = curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, false);

	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);   //����֤֤���HOST
	curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

	if (objCurlcode != CURLE_OK) {
		figureError(objCurlcode);
		return objCurlcode;
	}

	//curl interfaceִ��
	objCurlcode = curl_easy_perform(m_pCurl);

	//curl ������

	strncpy(pResponseData,strstrResponseData.str().c_str(),2000);

	if (objCurlcode != CURLE_OK )
	{
		figureError(objCurlcode);
	}

	return objCurlcode;
}

	//http�����ļ�
	/*CURLcode httpDownloadContent(const char * strUrl , long long llStartPos , long long llEndPos , CDivide * pDivide){
		CURLcode err;

		//��ʽ�������ļ���Χ�ַ���
		string strRange;
		stringstream strstrRange;
		strstrRange<<llStartPos<<"-"<<llEndPos;
		strRange = strstrRange.str();

		//����־�ļ�
		string strLogFilePath = "c:\\log";
		strLogFilePath.push_back('0'+pDivide->m_nDivideNO);
		strLogFilePath = strLogFilePath + ".log";
		pDivide->m_pLogFile = fopen(strLogFilePath.c_str(),"w");

		//curl����url
		curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl); 

		//curl���ò����������ź�
		curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1);

		//curl���������ļ���Χ
		curl_easy_setopt(m_pCurl, CURLOPT_RANGE, strRange.c_str());

		//curl����д���ݻص������������ݷ���ʱ���ã�
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, downloadData);

		//curl����д����buff�����ݷ���ʱ���յĴ洢λ�ã�
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)pDivide);

		//curl������������ʾ����
		curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0L);

		//curl���ý��Ȼص�����
		curl_easy_setopt(m_pCurl, CURLOPT_XFERINFOFUNCTION, getProgress);

		//curl���ý��Ȼص���������
		curl_easy_setopt(m_pCurl, CURLOPT_XFERINFODATA, (void *)pDivide);

		//curl interfaceִ��
		err = curl_easy_perform(m_pCurl);

		if (err != CURLE_OK )
		{
			figureError(err);
		}
		else{
			stringstream strstrLog;
			strstrLog<<"�̣߳�"<<pDivide->m_nDivideNO<<"�ɹ�������";
			fputs(strstrLog.str().c_str(),pDivide->m_pLogFile);
		}

		return err;

	}*/

//curl���ػص�����
/*size_t downloadData(void *ptr, size_t size, size_t nmemb, void * pDivide){

	CDivide * p = (CDivide  *)pDivide;

	//д���ļ���С
	size_t nWritten;

	//��־�ļ��ַ�����
	stringstream strstrLog;

	//����������
	WaitForSingleObject(p->m_hDownloadThreadMutex,INFINITE);

	//����
	//strstrLog<<"���ػص��������,�յ�����:"<<nmemb<<"�ֽ�";

    //д���ļ�
	if (p->m_llDownloadToPos+nmemb <= p->m_llEndPos){
		fseek (p->m_pLocalFile, p->m_llDownloadToPos, SEEK_SET); 
		if (size != 1)
		{
			//LOG(INFO)<<"size!=1";
		}
		nWritten = fwrite (ptr, size, nmemb, p->m_pLocalFile);
		if (nWritten != nmemb)
		{
			//LOG(INFO)<<"д����";
		}
		p->m_llDownloadToPos = p->m_llDownloadToPos + nmemb;
	} 
	else {
		fseek (p->m_pLocalFile, p->m_llDownloadToPos, SEEK_SET); 
		nWritten = fwrite (ptr, 1, p->m_llEndPos-p->m_llDownloadToPos+1, p->m_pLocalFile);
		p->m_llDownloadToPos = p->m_llEndPos;
	}
	strstrLog<<"�߳�:"<<p->m_nDivideNO<<"���ص�:"<<p->m_llDownloadToPos<<"д��:"<<nWritten<<"�յ�:"<<nmemb;

	fputs(strstrLog.str().c_str(),p->m_pLogFile);
	
	//����������
	ReleaseMutex(p->m_hDownloadThreadMutex);

	//����
	////LOG(INFO)<<"�̣߳�"<<p->m_nDivideNO<<"���ػص����������������:"<<p->m_llDownloadToPos<<"�ܹ������أ�"<< p->m_llEndPos-p->m_llStartPos+1;

	return nWritten;

} */


//curl���ؽ��Ȼص�����
int getProgress(void *ptr, curl_off_t  dlTotal, curl_off_t  dlNow, curl_off_t  ulTotal, curl_off_t  ulNow)  
{
	////LOG(INFO)<<"���Ȼص�������Ϣ����������"<<dlTotal<<", ��������:"<<dlNow;
	return 0;
}
