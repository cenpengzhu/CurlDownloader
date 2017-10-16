#pragma  once
#include <CTask.h>
#include <string>

class CDownloadTask : public CTask {
public:
	//下载地址URL
	string m_strRemotePath;
	//本地地址
	string m_strLocalPath;
	//开始位置
	long long m_llStartPos;
	//结束位置
	long long m_llEndPos;
	//已下载位置
	long long m_llDownloadedPos;
};