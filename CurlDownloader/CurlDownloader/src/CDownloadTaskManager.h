#pragma  once

#define KILOBYTES (1024)
#define MEGABYTES (1024*KILOBYTES)
#define GIGABYTES (1024*MEGABYTES)
#define TERABYTES (1024*GIGABYTES)


#include <CTaskManager.h>
#include <CDownloadTask.h>

using namespace std;

enum errorcode {
	//没有错误
	noerror = 1,
	//与远程文件不一致
	inconsisdent = 2,
	//下载划分信息错误
	dividerror = 3,
	//已下载文件错误
	downloadederror = 4,
	//文件操作错误
	filerror = 5,
	//远程文件错误
	remotefilerror = 6,
	//本地下载路径错误
	localfilerror = 7,
};

class CDownloadTaskManager : public CTaskManager {
public:
	//下载任务信息文件地址
	string m_strTaskInfoFilePath;
	//下载远程地址url
	string m_strRemotePath;
	//下载本地地址
	string m_strLocalPath;
	//下载文件大小，字节数
	long long m_llContentLength;
	//下载时间
	long long m_llDownloadTime;

	CDownloadTaskManager(const char * strRemotePath,const char * strLocalPath,const char * strTaskInfoFilePath);
	//生成下载任务--新建的下载任务
	errorcode generateDownloadTask();	
	//从文件中加载下载任务--已存在的任务
	int loadTaskFromFile();
	//写入下载任务文件
	int writeToFile();
	//载入下载任务
	errorcode loadDownloadTask();	
	//下载任务信息文件是否存在
	int isTaskInfoFileExisted(); 
	//获取下载文件的大小
	long long getContentLength();
	//转换字节数到带单位的字符串
	string convertLLContentLengthToString(long long llContentLength);
	//带单位字符串转换成大约值字符串
	string convertToAboutContentLength(string strContentLength);
	//字节数转换成大约值字符串
	string convertToAboutContentLength(long long llContentLength);
	//校验下载任务信息
	errorcode checkTaskInfo();
	//任务是否完成
	int isTasksFinished();
	//获取已下载总数
	long long getTotalDownloadedLength();
	//获取下载信息--下载进度，下载速度，剩余下载时间等。
	int getDownloadInfo();
	//清除下载任务
	int clearDownloadTask();
	//刷新下载时间
	long long freshDownloadTime(int nTime);

};