
// LogFile.cpp : �w�q�ɮ����O�C
//

#include "stdafx.h"
#include "LogFile.h"
#include "LogClient.h"
#include "LogClientDlg.h"

// Static member initialize
//


// CLogFileRecord class implementation
// 
CLogFileRecord::CLogFileRecord(CLogFileList* master)
{
}

/*virtual */CLogFileRecord::~CLogFileRecord(void)
{
}

/*virtual */void CLogFileRecord::CreateItems()
{
}

/*virtual */void CLogFileRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
}


// class CLogFileList implementation
// 
CLogFileList::CLogFileList(AFX_THREADPROC pfnThreadProc, LPVOID param, CString& path)
: CMyThread(pfnThreadProc, param)
, m_strFilePath(path)
, m_uAge(0)
, m_uGrowAge(0)
, m_bBeRefresh(FALSE)
, m_uFileCount(0)
, m_uFileTail(-1)
, m_uFileSize(0)
{
	InitializeSRWLock(&m_lockFileList);
//	InitializeSRWLock(&m_lockRefreshFileList);

	memset(&m_arrMtkFile, NULL, sizeof(LOGFILE*)*100);
}

CLogFileList::~CLogFileList(void)
{
}

/*virtual */void CLogFileList::Go(void)
{
	CString strLog;

	CString	strRefreshLogFileEvent;
	strRefreshLogFileEvent.Format("%srefreshlogfile", m_strFilePath);
	strRefreshLogFileEvent.MakeLower();
	strRefreshLogFileEvent.Replace("\\", "_");
	strRefreshLogFileEvent.Replace(":", "_");

	m_hRehreshEvent = CreateEvent(NULL, TRUE, FALSE, strRefreshLogFileEvent);
	
	// ���w�t�@��
	fnDoRefresh(this, FALSE);
	if (!RegisterWaitForSingleObject(&m_hWaitRehreshEvent, m_hRehreshEvent, fnDoRefresh, this, 5000, WT_EXECUTEDEFAULT))
	{
	//	strLog.Format("[%s]RegisterWaitForSingleObject Failure!!!", strRefreshEvent);
	//	log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		EndThread();
		return;
	}

	// ������
	while (!IsStop())
	{
		//if (m_bBeRefresh)
		//{
		//	AcquireSRWLockExclusive(&m_lockRefreshFileList);

		//	// RedrawControl
		//	for (int i=0; i<m_arrMtkFile.GetCount(); i++)
		//	{
		//		TRACE(m_arrMtkFile[i]->m_chFilename);
		//		TRACE("\n");
		//	}
		//	//m_bBeRefresh = FALSE;

		//	ReleaseSRWLockExclusive(&m_lockRefreshFileList);
		//}

		Sleep(1000);
	}

	EndThread();
	return;
}

void CLogFileList::AddLog(LOGFILE* log)
{
//	AcquireSRWLockExclusive(&m_lockFileList);
	
	m_uFileCount = m_uFileCount>=100 ? 100 : m_uFileCount+1;
	m_uFileTail = (m_uFileTail+1)%100;

	UINT size = 100;
	while (1)
	{
		if (size == 0)
		{
			LogSend("�ɮפӦh!", ID_SYSLOG_SEVERITY_ERROR);
			break;
		}

		if (!m_arrMtkFile[m_uFileTail] || (m_arrMtkFile[m_uFileTail] && m_arrMtkFile[m_uFileTail]->m_bDone==TRUE && m_arrMtkFile[m_uFileTail]->m_info.m_handle==NULL))
		{
			m_arrMtkFile[m_uFileTail] = log;
			break;
		}

		m_uFileTail = (m_uFileTail+1)%100;
		size--;
	}

//	ReleaseSRWLockExclusive(&m_lockFileList);
}

void CLogFileList::fnDoRefresh(PVOID lpParameter, BOOLEAN bTimer)
{
	CFileFind finder;
	CString strFind;
	char chLog[256] = {0};

	CLogFileList* me = (CLogFileList*)lpParameter;
	if (me->IsStop())
		return;

	if (!TryAcquireSRWLockExclusive(&me->m_lockFileList))
		return;					

	// �̫��ɮײM���s�ɶ�
	GetLocalTime(&me->m_tmEventTime);

	// ���o��Ӹ�Ƶ��c��m
	CMapStringToPtr* map = &me->m_mapMtkFile;
	LOGFILE** ary = me->m_arrMtkFile;

	// �s�@�N
	InterlockedIncrement((LONG*)&me->m_uAge);

	// ���o��Өt�Ϊ����m 
	//CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)ptr->GetParentsHandle();
	//CDFLogMaster* log = dlg->m_ptDFLog;

	strFind.Format("%s*.log", me->m_strFilePath);
	if (!finder.FindFile(strFind))
	{
		LogSend("FindFile ERROR!", ID_SYSLOG_SEVERITY_ERROR);

		ReleaseSRWLockExclusive(&me->m_lockFileList);
		return;
	}
	
	DWORD dwTick = GetTickCount();
	BOOL KeepWork = TRUE;
	while (KeepWork)
	{
		KeepWork = finder.FindNextFile();
		CString strFileName = finder.GetFileName();
		CString strFullFileName;
		strFullFileName.Format("%s%s", me->m_strFilePath, strFileName);

		LOGFILE* log;
		BOOL bFindFile = map->Lookup(strFileName, (void*&)log);
		BOOL bReadonlyFile = GetFileAttributes(strFullFileName) & FILE_ATTRIBUTE_READONLY;

		// �S������+�ɮװ�Ū=>�������L
		if (!bFindFile && bReadonlyFile)
			continue;

		// �S���������ɮ�=>���ͬ���
		if(!bFindFile) 
		{
			// �s���ɮ�
			log = new LOGFILE();
			memset(log, NULL, sizeof(LOGFILE));
		
			// ��l��
			memcpy(log->m_chFilepath, me->m_strFilePath, me->m_strFilePath.GetLength());	// ���|�W��
			memcpy(log->m_chFilename, strFileName, strFileName.GetLength());				// �ɮצW��
			memcpy(log->m_chVer, strFileName.Left(12), 12);									// �ɮפ��													

			log->m_master = me;																// �ɮ׺޲z��
//			InitializeSRWLock(&log->m_lock);												// SRWLOCK
			memset(&log->m_overlapped, NULL, sizeof(OVERLAPPED));							// OVERLAPPED
			
			log->m_dwLasttime = 0;															//
			log->m_uAge = me->m_uAge;														// 
			log->m_bProc = FALSE;
			log->m_bDone = FALSE;															//

			log->m_info.m_handle = NULL;													//
			log->m_info.m_attribute = FALSE;												//
			log->m_info.m_size = 0;															//

			// �s�W�ɮ׬���
//			me->AddLog(log);
			//ary->Add(log);
//			map->SetAt(strFileName, (void*&)log);

			// ���O
//			me->m_bBeRefresh = TRUE;
		}

		if (!bReadonlyFile && (log->m_info.m_handle==NULL))
		{
			if ((log->m_info.m_handle = CreateFile(strFullFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			{
				// ��Ū�ɮ׸��L
				sprintf_s(chLog, "[%s][%d]CreateFile ERROR!!!", strFileName, GetLastError());
				LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);

				log->m_info.m_handle = NULL;
				continue;
			}
			else
			{
				sprintf_s(chLog, "[%s][%d]CreateFile OK!!!", strFileName, GetLastError());
				LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
			}
		}

		// ������+���}�ɮ�=>��s���A
		if (log->m_info.m_handle)
		{
			BY_HANDLE_FILE_INFORMATION info;
			memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
			if (GetFileInformationByHandle(log->m_info.m_handle, &info) == 0)
			{
				sprintf_s(chLog, "[%s][%d]GetFileInformationByHandle ERROR!!!", strFileName, GetLastError());
				LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
				
				CloseHandle(log->m_info.m_handle);
				log->m_info.m_handle = NULL;
//				ReleaseSRWLockExclusive(&log->m_lock);	
				continue;
			}
		
			// �ɮ��ݩ�
			log->m_info.m_attribute = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;									
			if (!log->m_info.m_attribute) log->m_uAge = me->m_uAge;
			// �ɮפj�p
			LARGE_INTEGER size;
			size.QuadPart = 0;
			size.HighPart = info.nFileSizeHigh;
			size.LowPart = info.nFileSizeLow;
			log->m_info.m_size = size.QuadPart;		
			// �ɮ׳̫�g�J�ɶ�
			log->m_info.m_lastwrite = info.ftLastWriteTime;	
		}		
		
		if (!bFindFile && !bReadonlyFile)
		{
			// �s�W�ɮ׬���
			me->AddLog(log);
			map->SetAt(strFileName, (void*&)log);

			// ���O
			me->m_bBeRefresh = TRUE;
		}		
	}
	finder.Close();

	// �ɮװh������
	BOOL bGrowUp = FALSE;
	for (int i=0; i<100; i++)
	{
		LOGFILE* log = me->m_arrMtkFile[i];

		if (log && log->m_info.m_attribute && ((log->m_uAge+1) < me->m_uAge))
		{
			//ary->RemoveAt(i);
			map->RemoveKey(log->m_chFilename);	

			if (log->m_info.m_handle)
			{
				sprintf_s(chLog, "�����ɮ�![%s]", log->m_chFilename);
				LogSend(chLog, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				CloseHandle(log->m_info.m_handle);
				log->m_info.m_handle = NULL;
			}
//			delete log;
//			log = NULL;

//			me->m_bBeRefresh = TRUE;
//			bGrowUp = TRUE;													// �ɮ׼W��ݭn
		}
	}
	
	ReleaseSRWLockExclusive(&me->m_lockFileList);							// ����(�ɮײM��)

	if (bGrowUp)
	{
		InterlockedIncrement((LONG*)&me->m_uGrowAge);
		me->m_bBeRefresh = TRUE;
	}

	return;
}





