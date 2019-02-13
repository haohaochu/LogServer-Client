// LogBaseDlg.h : header file
//

#pragma once
#include "LogInput.h"
#include "LogConvert.h"
#include "LogRecv.h"
#include "LogAnalysis.h"

class CLogBaseDlg;
class CGrid1Record : public CXTPReportRecord
{
public:
	CGrid1Record(void);
	CGrid1Record(CLogBaseDlg* master, CString& name, CString& value);
	CGrid1Record(CLogBaseDlg* master, CString& name, UINT value);
	virtual ~CGrid1Record(void);

	virtual void CreateItems();
	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	BOOL UpdateRecord(int idx, CString& data);
	BOOL UpdateRecord(int idx, UINT data);
	CLogBaseDlg* m_master;
};

class CGrid2Record : public CXTPReportRecord
{
public:
	CGrid2Record(CLogBaseDlg* master);
	virtual ~CGrid2Record(void);

	virtual void CreateItems();
	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	CLogBaseDlg* m_master;
};

// CLogBaseDlg dialog
class CLogBaseDlg : public CDialog
{
// Construction
public:
	CLogBaseDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_LOGBASE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CLogInput*			m_ptLogInput;		// 監控檔案
	CLogConvert*		m_ptLogConvert;		// 轉換檔案
//	CLogOutput*			m_ptLogOutput;		// 輸出檔案
	CLogRecv*			m_ptLogRecv;		// 接收檔案
	CLogAnalysis*		m_ptLogAnalysis;	// 分析資料庫

public:
	BOOL				m_bUpdate;
	UINT				m_uLoghead;
	UINT				m_uLogtail;
	UINT				m_uStatus;
	UINT				m_uConnectCount;
	UINT				m_uDataCount;
	CLogNode			m_arrLog[100];
	CMapStringToPtr		m_mapLog;
	SRWLOCK				m_lockLog;

	HANDLE				m_hConvert;

	void				AddLogFile(CString&);
	void				DelLogFile(CString&);

// Implementation
protected:
	HICON				m_hIcon;

	void				BeginApp();
	void				EndApp();

	CFont				m_Font;
	CImageList			m_ToolBarImageList;
	CToolBar			m_ToolBar;

	CImageList			m_GridImageList;
	CXTPReportControl	m_Grid1;
	CXTPReportControl	m_Grid2;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
