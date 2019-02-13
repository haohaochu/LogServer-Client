
// LogFile.h : 實作檔案類別。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include "tools\mythread.h"
#include "syslog\syslogclient.h"

// file
typedef struct CLogFileInfo
{
	HANDLE					m_handle;
	BOOL					m_attribute;
	UINT64					m_size;
	FILETIME				m_lastwrite;
} LOGFILEINFO;


class CLogFileList;
class CLogSend;
typedef struct CLogFile
{
	// 檔案主要屬性
	LOGFILEINFO				m_info;
	
	// 檔案基本資料
	char					m_chFilepath[256];
	char					m_chFilename[256];
	char					m_chVer[16];

	// 檔案控制
	CLogFileList*			m_master;
//	SRWLOCK					m_lock;
	OVERLAPPED				m_overlapped;
	
	// 檔案紀錄
	DWORD					m_dwLasttime;
//	UINT					m_uFlow;
//	UINT					m_uFlowMax;
	UINT					m_uAge;
	BOOL					m_bProc;
	BOOL					m_bDone;
	UINT					m_uStatus;
} LOGFILE;


// class CDFFileRecord
// - 檔案紀錄
class CLogFileRecord : public CXTPReportRecord
{
public:
	CLogFileRecord(CLogFileList* master);
	virtual ~CLogFileRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
//	virtual void	DoPropExchange(CXTPPropExchange* pPX);

	CLogFileList*	m_FileMaster;
};


// class CDFFileMaster
// - 檔案管理員
class CLogFileList : public CMyThread
{
public:
	CLogFileList(AFX_THREADPROC pfnThreadProc, LPVOID param, CString& path);
	~CLogFileList(void);

protected:
	virtual void			Go(void);
	
	// 自己用的,不用鎖
	UINT					m_uCount;
	UINT					m_uRefreshAge;		// UI顯示介面的更動檔案版本
	UINT					m_nSortIndex;
	UINT					m_nSortType;
	CString					m_strFilter;
	HANDLE					m_hRehreshEvent;
	HANDLE					m_hWaitRehreshEvent;
	BOOL					m_bBeRefresh;

public:
	static void CALLBACK	fnDoRefresh(PVOID lpParameter, BOOLEAN bTimer);
	static void CALLBACK	fnDoUpdate(PVOID lpParameter, BOOLEAN bTimer);

	//
	void					AddLog(LOGFILE*);

	// UI用的,要鎖
	BOOL					IsUpdate()			{return m_bBeRefresh;}
	void					ResetUpdate()		{m_bBeRefresh = FALSE;}
	UINT					GetFileCount()		{return m_uFileCount;}
	UINT64					GetFileSize()		{return m_uFileSize;}
	
	CString					m_strFilePath;		// 靜態資料,不鎖
	UINT					m_uAge;				// 手上的檔案版本,Interlock
	UINT					m_uGrowAge;			// 手上的更動檔案版本	,Interlock

	// 檔案清單,要鎖
	SYSTEMTIME						m_tmEventTime;
//	SRWLOCK							m_lockFileList;			// 鎖檔案資訊更新
//	SRWLOCK							m_lockRefreshFileList;	// 鎖檔案新增刪減
	UINT							m_nSortCount;
	CMapStringToPtr					m_mapMtkFile;
	//CArray<LOGFILE*, LOGFILE*&>	m_arrMtkFile;
	
	LOGFILE*						m_arrMtkFile[100];
	SRWLOCK							m_lockFileList;
	UINT							m_uFileCount;
	int								m_uFileTail;
	UINT64							m_uFileSize;
};




