
// LogAnalysis.h
//

#pragma once
#include "mythread.h"
#include "LogBase.h"
#include <curl/curl.h>
#include "JsonParser.h"


class CLogAnalysis : public CMyThread
{
public:
	CLogAnalysis(AFX_THREADPROC pfnThreadProc, LPVOID param);
	~CLogAnalysis(void);

	static size_t RecvFunc(void *ptr, size_t size, size_t nmemb, void *stream);

protected:
	virtual void Go(void);
	HANDLE m_hStopTimerQueue;
	HANDLE m_hTimerQueue;
	HANDLE m_hTimer[100];

public:
	VOID static CALLBACK m_fnAnalysisApiA(PVOID lpParameter, BOOL bTimer);
	VOID static CALLBACK m_fnAnalysisApiB(PVOID lpParameter, BOOL bTimer);
	VOID static CALLBACK m_fnAnalysisUserA(PVOID lpParameter, BOOL bTimer);

};

