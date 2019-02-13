
// LogInput.cpp
//

#include "stdafx.h"
#include "LogInput.h"
#include "LogBase.h"
#include "LogBaseDlg.h"
#include "syslog\syslogclient.h"

// class CLogInput implementation
//
CLogInput::CLogInput(AFX_THREADPROC pfnThreadProc, LPVOID param, UINT date, UINT64 offset)
: CMyThread(pfnThreadProc, param)
, m_uDate(date)
, m_uOffset(offset)
{

}

CLogInput::~CLogInput(void)
{

}

/*virtual*/ void CLogInput::Go(void)
{
	CLogBaseDlg* parent = (CLogBaseDlg*)GetParentsHandle();
	LogSend("CLogInput GO", ID_SYSLOG_SEVERITY_INFORMATIONAL);

	CString strFilePath("c:\\quote\\log\\");
	CString strFileFormat(strFilePath+CString("*.log"));

	CFileFind finder;
	BOOL bKeepWork = finder.FindFile(strFileFormat);

	CString strFileName;
	CString strFileDate;

	// 第一次先全部取出
//	UINT i = 0;
//	while (bKeepWork)
//	{
//		bKeepWork = finder.FindNextFile();
//		strFileName = finder.GetFileName();
		
//		parent->AddLogFile(strFileName);
//		memcpy(m_arrLog[i].chFilename, strFileName.GetBuffer(0), strFileName.GetLength());
//		m_mapLog[strFileName] = &m_arrLog[i];
		
//		i++;
//	}

	// 然後排序一次
	//qsort(m_arrLog, i, sizeof(CLogNode), &CLogInput::LogSort);
//	m_uLoghead = 0;
//	m_uLogtail = i;

	// 有的話直接開工
//	HANDLE hEvent = CreateEvent(0, TRUE, FALSE, _T("logbase_working_event"));
	
//	if (m_uLogtail > 0)
//		SetEvent(hEvent);

	// 休息1分鐘
//	Sleep(60000);

	// 開始迴圈
	while (!IsStop())
	{
		BOOL bNewFile = FALSE;
		bKeepWork = finder.FindFile(strFileFormat);

		while (bKeepWork)
		{
			bKeepWork = finder.FindNextFile();
			strFileName = finder.GetFileName();

			parent->AddLogFile(strFileName);
		}

		Sleep(60000);
	}

	EndThread();
	return;
}

int CLogInput::LogSort(const void* a, const void* b)
{
	char* cha = ((CLogNode*)a)->chFilename;
	char* chb = ((CLogNode*)b)->chFilename;
	
	for (int i=0; i<20; i++)
	{
		if (cha[i]==0 && chb[i]==0)
			break;

		if (cha[i]-chb[i]==0)
			continue;

		return cha[i]-chb[i];
	}
	
	return 0;
}