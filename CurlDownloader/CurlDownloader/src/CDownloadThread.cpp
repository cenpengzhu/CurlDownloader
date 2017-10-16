#include "stdafx.h"
#include "CDownloadThread.h"
#include "CLibcurlTool.h"



CDownloadThread::CDownloadThread(HANDLE hThreadHandle,FILE * pLocalFile, HANDLE hPauseEvent):CThread(hThreadHandle, hPauseEvent){
	m_pLocalFile = pLocalFile;
}

int CDownloadThread::taskBusiness(){
	CLibcurlTool objLibcurlTool;
	CDownloadTask * pTask = (CDownloadTask *)m_pThreadTask;
	CURLcode err;
	if (pTask->m_llDownloadedPos == pTask->m_llEndPos)
	{
		pTask->m_nTaskStatus = TASK_COMPLETE;
		return 1;
	}
	err = objLibcurlTool.httpDownloadContent(pTask->m_strRemotePath.c_str(),pTask->m_llDownloadedPos,pTask->m_llEndPos,this);
	pTask->m_nTaskStatus = TASK_DONE;

	return 1;
}

