
// LogInput.h
//

#pragma once
#include "mythread.h"


typedef struct CLogNode
{
	char chFilename[256];
	BOOL bProc;
	BOOL bDone;
	BOOL bReadonly;
	UINT64 uFilesize;
	UINT uFileDataCount;
	UINT uFileErrorCount;
	OVERLAPPED overlapped;
}CLogNode;


class CLogInput : public CMyThread
{
public:
	CLogInput(AFX_THREADPROC pfnThreadProc, LPVOID param, UINT date, UINT64 offset);
	~CLogInput(void);

protected:
	virtual void	Go(void);
	static int		LogSort(const void*, const void*);

public:
	UINT		m_uDate;
	UINT64		m_uOffset;

//	UINT		m_uLoghead;
//	UINT		m_uLogtail;
//	CLogNode	m_arrLog[100];
//	CMapStringToPtr m_mapLog;
};