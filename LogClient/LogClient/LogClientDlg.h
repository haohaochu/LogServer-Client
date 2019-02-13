// LogClientDlg.h : header file
//

#pragma once
#include "LogFile.h"
#include "LogSend.h"

class CLogClientDlg;
class CGrid1Record : public CXTPReportRecord
{
public:
	CGrid1Record(void);
	CGrid1Record(CLogClientDlg* master, CString& name, CString& value);
	CGrid1Record(CLogClientDlg* master, CString& name, UINT value);
	virtual ~CGrid1Record(void);

	virtual void CreateItems();
	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	BOOL UpdateRecord(int idx, CString& data);
	BOOL UpdateRecord(int idx, UINT data);
	CLogClientDlg* m_master;
};

class CGrid2Record : public CXTPReportRecord
{
public:
	CGrid2Record(CLogClientDlg* master);
	virtual ~CGrid2Record(void);

	virtual void CreateItems();
	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	CLogClientDlg* m_master;
};


// CLogClientDlg dialog
class CLogClientDlg : public CDialog
{
// Construction
public:
	CLogClientDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_LOGCLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	void BeginApp();
	void EndApp();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	SRWLOCK				m_lockCreateApp;
//	SRWLOCK				m_lockFileList;
	CLogFileList*		m_ptLogFile;
	CLogSend*			m_ptLogSend;

protected:
	CFont				m_Font;
	CImageList			m_ToolBarImageList;
	CToolBar			m_ToolBar;

	CImageList			m_GridImageList;
	CXTPReportControl	m_Grid1;
	CXTPReportControl	m_Grid2;

public:
	UINT				m_FileCount;
	UINT64				m_FileSize;

	BOOL				m_Setting;
	UINT				m_Status;
};
