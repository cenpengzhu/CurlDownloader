#include "stdafx.h"
#include "CDownloadTaskManager.h"
#include "CLibcurlTool.h"



using namespace tinyxml2;
using namespace std;

CDownloadTaskManager::CDownloadTaskManager(const char * strRemotePath,const char * strLocalPath,const char * strTaskInfoFilePath){
	m_strRemotePath = strRemotePath;
	m_strLocalPath = strLocalPath;
	m_strTaskInfoFilePath = strTaskInfoFilePath;
	m_llDownloadTime = 0;
	m_llContentLength = 0;
}

//生成下载任务--新建的下载任务
errorcode CDownloadTaskManager::generateDownloadTask(){
	long long llContentLength;
	llContentLength = getContentLength(); 
	if (llContentLength == 0)
	{
		LOG(INFO) << " [Error]: " << "Get ContentLength Fail";
		return remotefilerror;
	}
	m_llContentLength = llContentLength;

	CDownloadTask * pDownloadTaskTemp;
	int nHowManyM = (int)(llContentLength / MEGABYTES);

	//一个任务分1M,不足1M的，分一个任务来做
	if (nHowManyM == 0 && llContentLength != 0)
	{
		m_nTasksCount = 1;
	}
	else if (nHowManyM != 0)
	{
		m_nTasksCount = nHowManyM;
	}
	else{
		return remotefilerror;
	}

	for (int i = 1;i <= m_nTasksCount; i++ )
	{
		pDownloadTaskTemp = new CDownloadTask;

		pDownloadTaskTemp->m_llStartPos = MEGABYTES * (i - 1);
		pDownloadTaskTemp->m_llDownloadedPos = pDownloadTaskTemp->m_llStartPos;
		if (i == m_nTasksCount)
		{
			pDownloadTaskTemp->m_llEndPos = llContentLength ;
		}
		else{
			pDownloadTaskTemp->m_llEndPos = MEGABYTES * i - 1;
		}
		pDownloadTaskTemp->m_nTaskStatus = TASK_TODO;
		pDownloadTaskTemp->m_strRemotePath = m_strRemotePath;
		pDownloadTaskTemp->m_strLocalPath = m_strLocalPath;
		pDownloadTaskTemp->m_nTaskId = i;

		pushOneTask(pDownloadTaskTemp);
	}
	return noerror;
}

//从文件中加载下载任务--已存在的任务
int CDownloadTaskManager::loadTaskFromFile(){
	tinyxml2::XMLDocument xmlTaskInfo;
	if (isTaskInfoFileExisted()) {
		xmlTaskInfo.LoadFile(m_strTaskInfoFilePath.c_str());
	}
	XMLNode * pTaskInfoNode = xmlTaskInfo.RootElement();
	//遍历XML节点，根据节点的value和text，来初始化TaskInfo

	//使用map来匹配TaskInfo节点的子节点
	map<string,int> mapDownTaskInfoValues;
	mapDownTaskInfoValues["TasksCount"] = 1;
	mapDownTaskInfoValues["ContentLength"] = 2;
	mapDownTaskInfoValues["DownloadTime"] = 3;
	mapDownTaskInfoValues["Task"] = 4;

	//使用map来匹配Task节点的子节点
	map<string,int> mapTaskValues;
	mapTaskValues["StartPos"] = 1;
	mapTaskValues["EndPos"] = 2;
	mapTaskValues["DownloadedPos"] = 3;
	mapTaskValues["RemotePath"] = 4;
	mapTaskValues["LocalPath"] = 5;
	mapTaskValues["TaskStatus"] = 6;
	mapTaskValues["TaskId"] = 7;

	//两个用于强制类型转换的指针
	XMLElement * pTaskInfoElementTemp;
	XMLElement * pTaskElementTemp;

	//开始遍历
	for (XMLNode * pIterTaskInfoNode = pTaskInfoNode->FirstChild();pIterTaskInfoNode != NULL;pIterTaskInfoNode = pIterTaskInfoNode->NextSibling())
	{
		pTaskInfoElementTemp = (XMLElement * )pIterTaskInfoNode;
		switch (mapDownTaskInfoValues[pIterTaskInfoNode->Value()])
		{
		case 1:
			pTaskInfoElementTemp->QueryIntText(&m_nTasksCount);
			break;
		case 2:
			pTaskInfoElementTemp->QueryInt64Text(&m_llContentLength);
			break;
		case 3:
			pTaskInfoElementTemp->QueryInt64Text(&m_llDownloadTime);
			break;
		case 4:
			CDownloadTask * pTempDownloadTask = new CDownloadTask();
			for (XMLNode * pIterTaskNode = pIterTaskInfoNode->FirstChild();pIterTaskNode != NULL;pIterTaskNode = pIterTaskNode->NextSibling())
			{
				pTaskElementTemp = (XMLElement *)pIterTaskNode;
				switch (mapTaskValues[pIterTaskNode->Value()])
				{
				case 1:
					pTaskElementTemp->QueryInt64Text(&pTempDownloadTask->m_llStartPos);
					break;
				case 2:
					pTaskElementTemp->QueryInt64Text(&pTempDownloadTask->m_llEndPos);
					break;
				case 3:
					pTaskElementTemp->QueryInt64Text(&pTempDownloadTask->m_llDownloadedPos);
					break;
				case 4:
					pTempDownloadTask->m_strRemotePath = pTaskElementTemp->GetText();
					break;
				case 5:
					pTempDownloadTask->m_strLocalPath = pTaskElementTemp->GetText();
					break;
				case 6:
					pTaskElementTemp->QueryIntText(&pTempDownloadTask->m_nTaskStatus);
					break;
				case 7:
					pTaskElementTemp->QueryIntText(&pTempDownloadTask->m_nTaskId);
					break;

				}
			}
			pushOneTask(pTempDownloadTask);
		}
	}
	return 1;
}

//写入下载任务文件
int CDownloadTaskManager::writeToFile(){
	tinyxml2::XMLDocument xmlTaskInfo;
	//两个xml节点指针，pXMLNode指向Doc中已存在的父节点，pXMLNodeNew指向new出来的节点。
	XMLNode * pXMLNode;
	XMLNode * pXMLNodeNew;

	XMLElement * pXMLElementNew;

	//转换XMLDocument

	//新增根节点TaskInfo
	pXMLNode = &xmlTaskInfo;
	pXMLNodeNew = xmlTaskInfo.NewElement("TaskInfo");
	pXMLNode ->InsertFirstChild(pXMLNodeNew);

	pXMLNode = pXMLNode->FirstChild();

	//添加TasksCount节点
	pXMLNodeNew = xmlTaskInfo.NewElement("TasksCount");
	pXMLElementNew = (XMLElement * )pXMLNodeNew;
	pXMLElementNew->SetText(m_nTasksCount);
	pXMLNode -> LinkEndChild(pXMLNodeNew);

	//添加ContentLength节点
	pXMLNodeNew = xmlTaskInfo.NewElement("ContentLength");
	pXMLElementNew = (XMLElement * )pXMLNodeNew;
	pXMLElementNew->SetText((int64_t)m_llContentLength);
	pXMLNode -> LinkEndChild(pXMLNodeNew);

	//添加DownloadTime节点
	pXMLNodeNew = xmlTaskInfo.NewElement("DownloadTime");
	pXMLElementNew = (XMLElement * )pXMLNodeNew;
	pXMLElementNew->SetText((int64_t)m_llDownloadTime);
	pXMLNode -> LinkEndChild(pXMLNodeNew);

	//依次添加每一个Task节点
	for (vector<CTask *>::const_iterator iterPTask = m_vecPTasks.begin();iterPTask != m_vecPTasks.end();iterPTask++)
	{
		pXMLNodeNew = xmlTaskInfo.NewElement("Task");
		pXMLNode->LinkEndChild(pXMLNodeNew);
		CDownloadTask * pDownloadTask = (CDownloadTask *)*iterPTask;
		//父节点指向Divide节点
		pXMLNode = pXMLNodeNew;

		//添加StartPos节点
		pXMLNodeNew = xmlTaskInfo.NewElement("StartPos");
		pXMLElementNew = (XMLElement * )pXMLNodeNew;
		pXMLElementNew->SetText(pDownloadTask->m_llStartPos);
		pXMLNode->LinkEndChild(pXMLNodeNew);

		//添加EndPos节点
		pXMLNodeNew = xmlTaskInfo.NewElement("EndPos");
		pXMLElementNew = (XMLElement * )pXMLNodeNew;
		pXMLElementNew->SetText(pDownloadTask->m_llEndPos);
		pXMLNode->LinkEndChild(pXMLNodeNew);

		//添加DownloadedPos节点
		pXMLNodeNew = xmlTaskInfo.NewElement("DownloadedPos");
		pXMLElementNew = (XMLElement * )pXMLNodeNew;
		pXMLElementNew->SetText(pDownloadTask->m_llDownloadedPos);
		pXMLNode->LinkEndChild(pXMLNodeNew);

		//添加RemotePath节点
		pXMLNodeNew = xmlTaskInfo.NewElement("RemotePath");
		pXMLElementNew = (XMLElement * )pXMLNodeNew;
		pXMLElementNew->SetText(pDownloadTask->m_strRemotePath.c_str());
		pXMLNode->LinkEndChild(pXMLNodeNew);

		//添加LocalPath节点
		pXMLNodeNew = xmlTaskInfo.NewElement("LocalPath");
		pXMLElementNew = (XMLElement * )pXMLNodeNew;
		pXMLElementNew->SetText(pDownloadTask->m_strLocalPath.c_str());
		pXMLNode->LinkEndChild(pXMLNodeNew);

		//添加TaskStatus节点
		pXMLNodeNew = xmlTaskInfo.NewElement("TaskStatus");
		pXMLElementNew = (XMLElement * )pXMLNodeNew;
		pXMLElementNew->SetText(pDownloadTask->m_nTaskStatus);
		pXMLNode->LinkEndChild(pXMLNodeNew);

		//添加TaskId节点
		pXMLNodeNew = xmlTaskInfo.NewElement("TaskId");
		pXMLElementNew = (XMLElement * )pXMLNodeNew;
		pXMLElementNew->SetText(pDownloadTask->m_nTaskId);
		pXMLNode->LinkEndChild(pXMLNodeNew);

		//父节点指针回退，指向DownloadedInfo节点
		pXMLNode = pXMLNode->Parent();
	}
	//xmlTaskInfo.Print();
	xmlTaskInfo.SaveFile(m_strTaskInfoFilePath.c_str());
	return 1;
}

//载入下载任务
errorcode CDownloadTaskManager::loadDownloadTask(){
	//如果下载任务文件存在，则从文件加载。如果不存在，重新初始化。
	if (isTaskInfoFileExisted())
	{
		//下载信息文件存在
		loadTaskFromFile();
		if (checkTaskInfo() == noerror)
		{
			LOG(INFO) << " Load tasks from file success!";
			return noerror;
		}
		else{
			LOG(INFO) << " taskinfo file error";
			clearDownloadTask();
			return generateDownloadTask();
		}
	}
	//不存在，重新初始化一个。
	else{
		return generateDownloadTask();
	}
}

//下载任务信息文件是否存在
int CDownloadTaskManager::isTaskInfoFileExisted() {
	fstream fsDownloadedInfoFile;
	fsDownloadedInfoFile.open(m_strTaskInfoFilePath.c_str(),ios::in);

	if (!fsDownloadedInfoFile) {
		return 0;
	}
	else {
		return 1;
	}
}

//获取下载文件的大小
long long CDownloadTaskManager::getContentLength(){
	char ResponseData[2000];

	//文件大小
	long long llContentLength = 0;
	string strHttpHeader;

	CLibcurlTool * pCurlForContentLength = new CLibcurlTool();
	while (true){ 
		pCurlForContentLength->httpGetHeader(m_strRemotePath.c_str(), ResponseData); 
		strHttpHeader = ResponseData;
		//http头有Location字段，说明有重定向。
		if (strHttpHeader.find("Location:") != string::npos)
		{
			string strRedirect = strHttpHeader.substr(strHttpHeader.find("Location:"));
			strRedirect = strRedirect.substr(strRedirect.find_first_of(":") + 2, strRedirect.find("\r\n") - strRedirect.find_first_of(":") - 2);
			m_strRemotePath = strRedirect;
		}
		else
		{
			break;
		}
	}

    strHttpHeader = ResponseData;
	stringstream strstrContentLength;

	//返回的结果没有Content-Range行，可能是没有获得数据，也可能是没有获得Content-Range行
	if (strHttpHeader.find("Content-Range:") == string::npos && strHttpHeader.find("Content-Length:") == string::npos  )
	{
		//错误处理
		return 0;
	}
	else if (strHttpHeader.find("Content-Range:") != string::npos)
	{
		string strContentRange = strHttpHeader.substr(strHttpHeader.find("Content-Range:"),strHttpHeader.find("Server:")-strHttpHeader.find("Content-Range"));
		strstrContentLength<<strContentRange.substr(strContentRange.find("/")+1);
	}
	else if (strHttpHeader.find("Content-Length:") != string::npos)
	{
		string strContentLength = strHttpHeader.substr(strHttpHeader.find("Content-Length:"));
		strstrContentLength<<strContentLength.substr(strContentLength.find(":")+1,strContentLength.find("\n"));

	}


	//获取文件大小数值
	strstrContentLength>>llContentLength;
	//m_llContentLength = llContentLength;

	LOG(INFO) << " Get Contentlength = " <<llContentLength;
	return llContentLength;
}

//转换字节数到带单位的字符串
string CDownloadTaskManager::convertLLContentLengthToString(long long llContentLength){
	long long llTemp;
	string strContentLength;
	stringstream strstrContentLength;
	llTemp = llContentLength;
	unsigned int nHowManyG = 0;
	unsigned int nHowManyM = 0;
	unsigned int nHowManyK = 0;
	unsigned int nHowManyB = 0;
	if (llTemp > GIGABYTES)
	{
		nHowManyG = (unsigned int)(llTemp / GIGABYTES);
		strstrContentLength<<nHowManyG<<"G";
		llTemp = llTemp % GIGABYTES;
	}
	if (llTemp > MEGABYTES)
	{
		nHowManyM = (unsigned int)(llTemp / MEGABYTES);
		strstrContentLength<<nHowManyM<<"M";
		llTemp = llTemp % MEGABYTES;
	}
	if (llTemp > KILOBYTES)
	{
		nHowManyK = (unsigned int)(llTemp / KILOBYTES);
		strstrContentLength<<nHowManyK<<"K";
		llTemp = llTemp % KILOBYTES;
	}
	if (llTemp > 0)
	{
		nHowManyB = (unsigned int)llTemp ;
		strstrContentLength<<nHowManyB<<"B";
	}
	strContentLength = strstrContentLength.str();
	return strContentLength;
}

//带单位字符串转换成大约值字符串
string CDownloadTaskManager::convertToAboutContentLength(string strContentLength){
	stringstream strstrAboutContentLength;
	string strAboutContentLength;

	int nInteger;
	double dDecimal;
	int temp;

	//转换成只有一个单位的大约值
	if (strContentLength.find('G')!= string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('G')).c_str());
		if (strContentLength.find('M')!= string::npos)
		{
			dDecimal = atoi(strContentLength.substr(strContentLength.find('G')+1,strContentLength.find('M')).c_str())/1024.0;
		}
		temp = int(dDecimal * 100);
		dDecimal = temp / 100.0;
		strstrAboutContentLength<<nInteger+dDecimal<<"G";
		strAboutContentLength = strstrAboutContentLength.str();
	}
	else if (strContentLength.find('M')!= string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('M')).c_str());
		if (strContentLength.find('K')!= string::npos)
		{
			dDecimal = atoi(strContentLength.substr(strContentLength.find('M')+1,strContentLength.find('K')).c_str())/1024.0;
		}
		temp = (int)(dDecimal * 100);
		dDecimal = temp / 100.0;
		strstrAboutContentLength<<nInteger+dDecimal<<"M";
		strAboutContentLength = strstrAboutContentLength.str();
	}
	else if (strContentLength.find('K')!= string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('K')).c_str());
		if (strContentLength.find('B')!= string::npos)
		{
			dDecimal = atoi(strContentLength.substr(strContentLength.find('K')+1,strContentLength.find('B')).c_str())/1024.0;
		}
		temp = (int)(dDecimal * 100);
		dDecimal = temp / 100.0;
		strstrAboutContentLength<<nInteger+dDecimal<<"K";
		strAboutContentLength = strstrAboutContentLength.str();
	}
	else if (strContentLength.find('B') != string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('B')).c_str());
		strstrAboutContentLength<<nInteger+dDecimal<<"B";
		strAboutContentLength = strstrAboutContentLength.str();	
	}
	else 
	{
		strContentLength = "0B";
		strAboutContentLength = "0.00B";	
	}
	return strAboutContentLength;
}


//带单位字符串转换成大约值字符串
string CDownloadTaskManager::convertToAboutContentLength(long long llContentLength){
	if (llContentLength == 0)
	{
		return "0.00B";
	}

	string strContentLength = convertLLContentLengthToString(llContentLength);
	//LOG(INFO)<<strContentLength.c_str();
	stringstream strstrAboutContentLength;
	string strAboutContentLength;

	int nInteger = 0;
	double dDecimal = 0;
	int temp = 0;

	//转换成只有一个单位的大约值
	if (strContentLength.find('G')!= string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('G')).c_str());
		if (strContentLength.find('M')!= string::npos)
		{
			dDecimal = atoi(strContentLength.substr(strContentLength.find('G')+1,strContentLength.find('M')).c_str())/1024.0;
		}
		temp = (int)(dDecimal * 100);
		dDecimal = temp / 100.0;
		strstrAboutContentLength<<nInteger+dDecimal<<"G";
		strAboutContentLength = strstrAboutContentLength.str();
	}
	else if (strContentLength.find('M')!= string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('M')).c_str());
		if (strContentLength.find('K')!= string::npos)
		{
			dDecimal = atoi(strContentLength.substr(strContentLength.find('M')+1,strContentLength.find('K')).c_str())/1024.0;
		}
		temp = (int)(dDecimal * 100);
		dDecimal = temp / 100.0;
		strstrAboutContentLength<<nInteger+dDecimal<<"M";
		strAboutContentLength = strstrAboutContentLength.str();
	}
	else if (strContentLength.find('K')!= string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('K')).c_str());
		if (strContentLength.find('B')!= string::npos)
		{
			dDecimal = atoi(strContentLength.substr(strContentLength.find('K')+1,strContentLength.find('B')).c_str())/1024.0;
		}
		temp = (int)(dDecimal * 100);
		dDecimal = temp / 100.0;
		strstrAboutContentLength<<nInteger+dDecimal<<"K";
		strAboutContentLength = strstrAboutContentLength.str();
	}
	else if (strContentLength.find('B') != string::npos)
	{
		nInteger = atoi(strContentLength.substr(0,strContentLength.find('B')).c_str());
		strstrAboutContentLength<<nInteger+dDecimal<<"B";
		strAboutContentLength = strstrAboutContentLength.str();	
	}
	else 
	{
		strContentLength = "0B";
		strAboutContentLength = "0.00B";	
	}
	return strAboutContentLength;
}

//校验下载任务信息
errorcode CDownloadTaskManager::checkTaskInfo(){
	//校验ContentLength是否与远程文件一致
	if (m_llContentLength != getContentLength())
	{
		//下载信息ContentLength与远程文件不一致
		return inconsisdent;
	}
	//校验任务是否合法--每个划分首尾相连，最后一个划分的结束位置等于contentlength
	long long llTemp = 0;
	for (vector<CTask *>::const_iterator iterPTasks = m_vecPTasks.begin();iterPTasks != m_vecPTasks.end();iterPTasks++)
	{
		CDownloadTask *p = (CDownloadTask *)*iterPTasks;
		if (p->m_llStartPos != ((p->m_nTaskId - 1) * MEGABYTES))
		{
			//划分开始位置错误
			return dividerror;
		}
		if (llTemp != p->m_llStartPos)
		{
			//划分结束位置错误
			return dividerror;
		}
		llTemp = p->m_llEndPos + 1;
	}
	if (llTemp != (m_llContentLength+1))
	{
		//最后一个划分的结束位置错误
		return dividerror;
	}
	//校验已下载文件信息是否一致
	return noerror;
}

//任务是否完成
int CDownloadTaskManager::isTasksFinished(){
	CDownloadTask * p;
	for (vector<CTask *>::const_iterator iterPDownloadTask = m_vecPTasks.begin();iterPDownloadTask != m_vecPTasks.end();iterPDownloadTask++)
	{
		p = (CDownloadTask *)*iterPDownloadTask;
		if (p->m_nTaskStatus != TASK_COMPLETE)
		{
			return 0;
		}
	}
	return 1;
}

//获取已下载总数
long long CDownloadTaskManager::getTotalDownloadedLength(){
	long long llTotalDownloadedLength = 0;
	CDownloadTask * p;
	for (vector<CTask *>::const_iterator iterPDownloadTask = m_vecPTasks.begin();iterPDownloadTask != m_vecPTasks.end();iterPDownloadTask++)
	{
		p = (CDownloadTask *)*iterPDownloadTask;
		llTotalDownloadedLength = llTotalDownloadedLength + (p->m_llDownloadedPos - p->m_llStartPos + 1);
	}
	return llTotalDownloadedLength;
}



int  CDownloadTaskManager::clearDownloadTask(){
	//基类成员
	clearTask();

	//子类成员
	m_llDownloadTime = 0;
	m_llContentLength = 0;

	return 1;
}

long long CDownloadTaskManager::freshDownloadTime(int nTime){
	m_llDownloadTime = m_llDownloadTime + nTime;
	return m_llDownloadTime;
}