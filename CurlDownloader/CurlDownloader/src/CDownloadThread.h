#pragma  once

#include <CThread.h>
#include <CDownloadTask.h>

class CDownloadThread : public CThread {
public:
	FILE * m_pLocalFile;

	CDownloadThread(HANDLE hThreadHandle,FILE * pLocalFile, HANDLE hPauseEvent);

	int taskBusiness();

};