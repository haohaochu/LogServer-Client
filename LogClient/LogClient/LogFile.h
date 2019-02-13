
// LogFile.h : ��@�ɮ����O�C
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�� PCH �]�t���ɮ׫e���]�t 'stdafx.h'"
#endif

#include "resource.h"		// �D�n�Ÿ�
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
	// �ɮץD�n�ݩ�
	LOGFILEINFO				m_info;
	
	// �ɮװ򥻸��
	char					m_chFilepath[256];
	char					m_chFilename[256];
	char					m_chVer[16];

	// �ɮױ���
	CLogFileList*			m_master;
//	SRWLOCK					m_lock;
	OVERLAPPED				m_overlapped;
	
	// �ɮ׬���
	DWORD					m_dwLasttime;
//	UINT					m_uFlow;
//	UINT					m_uFlowMax;
	UINT					m_uAge;
	BOOL					m_bProc;
	BOOL					m_bDone;
	UINT					m_uStatus;
} LOGFILE;


// class CDFFileRecord
// - �ɮ׬���
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
// - �ɮ׺޲z��
class CLogFileList : public CMyThread
{
public:
	CLogFileList(AFX_THREADPROC pfnThreadProc, LPVOID param, CString& path);
	~CLogFileList(void);

protected:
	virtual void			Go(void);
	
	// �ۤv�Ϊ�,������
	UINT					m_uCount;
	UINT					m_uRefreshAge;		// UI��ܤ���������ɮת���
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

	// UI�Ϊ�,�n��
	BOOL					IsUpdate()			{return m_bBeRefresh;}
	void					ResetUpdate()		{m_bBeRefresh = FALSE;}
	UINT					GetFileCount()		{return m_uFileCount;}
	UINT64					GetFileSize()		{return m_uFileSize;}
	
	CString					m_strFilePath;		// �R�A���,����
	UINT					m_uAge;				// ��W���ɮת���,Interlock
	UINT					m_uGrowAge;			// ��W������ɮת���	,Interlock

	// �ɮײM��,�n��
	SYSTEMTIME						m_tmEventTime;
//	SRWLOCK							m_lockFileList;			// ���ɮ׸�T��s
//	SRWLOCK							m_lockRefreshFileList;	// ���ɮ׷s�W�R��
	UINT							m_nSortCount;
	CMapStringToPtr					m_mapMtkFile;
	//CArray<LOGFILE*, LOGFILE*&>	m_arrMtkFile;
	
	LOGFILE*						m_arrMtkFile[100];
	SRWLOCK							m_lockFileList;
	UINT							m_uFileCount;
	int								m_uFileTail;
	UINT64							m_uFileSize;
};




