// LogClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LogClient.h"
#include "LogClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


//

// CUpdateEliseDlg dialog

CGrid1Record::CGrid1Record()
{
}

CGrid1Record::CGrid1Record(CLogClientDlg* master, CString& name, CString& value)
{
	AddItem(new CXTPReportRecordItemText(name));
	AddItem(new CXTPReportRecordItemText(value));
}

CGrid1Record::CGrid1Record(CLogClientDlg* master, CString& name, UINT value)
{
	AddItem(new CXTPReportRecordItemText(name));
	
	CString str("");
	str.Format("%d", value);
	AddItem(new CXTPReportRecordItemText(str));
}

/*virtual */CGrid1Record::~CGrid1Record(void)
{
}

/*virtual */void CGrid1Record::CreateItems(void)
{
}

/*virtual */void CGrid1Record::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportRecord::GetItemMetrics(pDrawArgs, pItemMetrics);
}

BOOL CGrid1Record::UpdateRecord(int idx, CString& data)
{
	if (idx < this->GetItemCount()) 
	{
		((CXTPReportRecordItemText*)GetItem(idx))->SetValue(data);
		return TRUE;
	}

	return FALSE;
}

BOOL CGrid1Record::UpdateRecord(int idx, UINT data)
{
	if (idx < this->GetItemCount()) 
	{
		CString strData("");
		strData.Format("%d", data);

		((CXTPReportRecordItemText*)GetItem(idx))->SetValue(strData);
		return TRUE;
	}

	return FALSE;
}

//

CGrid2Record::CGrid2Record(CLogClientDlg* master)
: m_master(master)
{
	CreateItems();
}

CGrid2Record::~CGrid2Record(void)
{
}

/*virtual */void CGrid2Record::CreateItems()
{
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
}

/*virtual */void CGrid2Record::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CString strColumn = pDrawArgs->pColumn->GetCaption();
	int nIndexCol = pDrawArgs->pColumn->GetItemIndex();
	int nIndexRow = pDrawArgs->pRow->GetIndex();
	int nCount = pDrawArgs->pControl->GetRows()->GetCount();

	if (m_master)
	{
		// 鎖新增刪除
		//AcquireSRWLockExclusive(&m_master->m_ptLogFile->m_lockRefreshFileList);
		AcquireSRWLockExclusive(&m_master->m_ptLogFile->m_lockFileList);

		//UINT idx = (m_master->m_FileCount-nIndexRow-1)%100;
		UINT idx = (m_master->m_ptLogFile->m_uFileTail-nIndexRow)%100;
		if (idx < 0)
		{
			pItemMetrics->strText.Format("%s", "NULL");
			ReleaseSRWLockExclusive(&m_master->m_ptLogFile->m_lockFileList);
			return;
		}

		LOGFILE* log = m_master->m_ptLogFile->m_arrMtkFile[idx];

		if (log == NULL)
		{
			pItemMetrics->strText.Format("%s", "NULL");
			//ReleaseSRWLockExclusive(&m_master->m_ptLogFile->m_lockRefreshFileList);
			ReleaseSRWLockExclusive(&m_master->m_ptLogFile->m_lockFileList);
			return;
		}
		
		switch (nIndexCol)
		{
		case 0: // 編號
			{
				pItemMetrics->strText.Format("%d", m_master->m_FileCount-nIndexRow-1);
				break;
			}
		case 1: // 檔案
			{
				pItemMetrics->strText.Format("%s", log->m_chFilename);
				break;
			}
		case 2: // 狀態
			{
				pItemMetrics->strText.Format("%s", log->m_bProc? (log->m_bDone? "完成" : "傳送") : "等待");
				break;
			}
		case 3: // 流量
			{
				pItemMetrics->strText.Format("%I64dKB/%I64dKB", (UINT64)log->m_overlapped.Offset/1024, (UINT64)log->m_info.m_size/1024);
				break;
			}
		default:
			{
				break;
			}
		}

		//ReleaseSRWLockExclusive(&m_master->m_ptLogFile->m_lockRefreshFileList);
		ReleaseSRWLockExclusive(&m_master->m_ptLogFile->m_lockFileList);
	}

	return;


}

// CLogClientDlg dialog

CLogClientDlg::CLogClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLogClientDlg::IDD, pParent)
, m_FileCount(0)
, m_FileSize(0)
, m_Setting(FALSE)
, m_Status(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CLogClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLogClientDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CLogClientDlg message handlers

BOOL CLogClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);         // Set big icon
	SetIcon(m_hIcon, FALSE);        // Set small icon

	// Setting
	LoadSysLogClientDll();
	theApp.GetIni();
	
	// User Interface
	CRect rect;
	CBitmap bmp;
	GetClientRect(&rect);
	
	m_Font.CreateFont(
		16,							// nHeight(Min 8)
		0,							// nWidth(min 4)
		0,							// nEscapement
		0,							// nOrientation
		FW_BOLD,					// nWeight
		FALSE,						// bItalic
		FALSE,						// bUnderline
		0,							// cStrikeOut
		ANSI_CHARSET,				// nCharSet
		OUT_DEFAULT_PRECIS,			// nOutPrecision
		CLIP_DEFAULT_PRECIS,		// nClipPrecision
		DEFAULT_QUALITY,			// nQuality
		DEFAULT_PITCH | FF_SWISS,	// nPitchAndFamily
		//"Consolas");				// lpszFacename
		//"Courier New");			// lpszFacename
		//"Arial");					// lpszFacename
		"微軟正黑體");				// lpszFacename

	m_ToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arToolBarImg[] = {IDB_BITMAP3, IDB_BITMAP4, IDB_BITMAP5};
	UINT arToolBarBtn[] = {IDC_TOOLBARBTN01, IDC_TOOLBARBTN02, IDC_TOOLBARBTN03};
	for (int i=0; i<3; i++)
	{
		bmp.LoadBitmap(arToolBarImg[i]);
		m_ToolBarImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}

	m_ToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_ToolBar.SetButtons(arToolBarBtn, 3);
	m_ToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_ToolBar.GetToolBarCtrl().SetImageList(&m_ToolBarImageList);
	m_ToolBar.MoveWindow(0, 0, rect.Width(), 40, 1);
	
	m_ToolBar.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_ToolBar.SetButtonText(0, _T("開始"));
	
	m_ToolBar.SetButtonStyle(1, TBBS_AUTOSIZE);
	m_ToolBar.SetButtonText(1, _T("停止"));

	m_ToolBar.SetButtonStyle(2, TBBS_AUTOSIZE | TBSTYLE_CHECK);
	m_ToolBar.SetButtonText(2, _T("設定"));

	// Grid
	m_GridImageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);
	UINT arGridImg[] = {IDB_BITMAP1, IDB_BITMAP2};
	for (int i=0; i<2; i++)
	{
		bmp.LoadBitmap(arGridImg[i]);
		m_GridImageList.Add(&bmp, RGB(0, 0, 0));
		bmp.DeleteObject();
	}

	m_Grid1.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 40, 180, rect.Height()), this, IDC_GRID1, NULL);
	m_Grid1.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	//m_Grid1.GetPaintManager()->SetGridStyle(FALSE, xtpReportGridNoLines);
	m_Grid1.GetReportHeader()->AllowColumnRemove(FALSE);
	
	m_Grid1.SetImageList(&m_GridImageList);
	m_Grid1.SetFont(&m_Font);
	m_Grid1.ShowHeader(FALSE);
	
	m_Grid1.AddColumn(new CXTPReportColumn(0, _T("項目"), 70, 1, -1, 0));
	m_Grid1.AddColumn(new CXTPReportColumn(1, _T("內容"), 100, 1, -1, 0));

	m_Grid1.GetColumns()->GetAt(0)->SetAlignment(DT_CENTER);
	m_Grid1.GetColumns()->GetAt(1)->SetAlignment(DT_CENTER);
	
	CString strTarget;
	strTarget.Format("%s:%i", theApp.m_chServerIP, theApp.m_uServerPORT);
	m_Grid1.AddRecord(new CGrid1Record(this, CString("程式狀態"), m_Status));
	m_Grid1.AddRecord(new CGrid1Record(this, CString("連線狀態"), m_Status));
	m_Grid1.AddRecord(new CGrid1Record(this, CString("目的位置"), strTarget));
	m_Grid1.AddRecord(new CGrid1Record(this, CString("檔案數量"), m_FileCount));
	m_Grid1.AddRecord(new CGrid1Record(this, CString("檔案大小"), m_FileSize));
	
	m_Grid1.GetColumns()->GetAt(0)->SetEditable(0);
	
	m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetEditable(0);
	m_Grid1.GetRecords()->GetAt(1)->GetItem(1)->SetEditable(0);
	m_Grid1.GetRecords()->GetAt(2)->GetItem(1)->SetEditable(1);
	m_Grid1.GetRecords()->GetAt(3)->GetItem(1)->SetEditable(0);
	m_Grid1.GetRecords()->GetAt(4)->GetItem(1)->SetEditable(0);

	switch (m_Status)
	{
	case 0:
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(2);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("初始設定...");
			break;
		}
	case 1: // 正常
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(0);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("正常");
			break;
		}
	case 2: // 停止
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(1);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("停止");
			break;
		}
	case 3: // 更新設定...
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(2);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("更新設定...");
			break;
		}
	default:
		{
			break;
		}
	}

	((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(1)->GetItem(1)))->SetValue("初始連線");
	m_Grid1.Populate();
	
	m_Grid2.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(180, 40, rect.Width(), rect.Height()), this, IDC_GRID2, NULL);
	m_Grid2.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_Grid2.GetPaintManager()->SetGridStyle(FALSE, xtpReportGridNoLines);
	m_Grid2.GetReportHeader()->AllowColumnRemove(FALSE);
	
	m_Grid2.SetImageList(&m_GridImageList);
	m_Grid2.SetFont(&m_Font);
	m_Grid2.ShowHeader(FALSE);

	m_Grid2.AddColumn(new CXTPReportColumn(0, _T("編號"), 30, 1, -1, 0));
	m_Grid2.AddColumn(new CXTPReportColumn(1, _T("檔案"), 150, 1, -1, 0));
	m_Grid2.AddColumn(new CXTPReportColumn(2, _T("狀態"), 40, 1, -1, 0));
	m_Grid2.AddColumn(new CXTPReportColumn(3, _T("流量"), 60, 1, -1, 0));
	
	m_Grid2.GetColumns()->GetAt(0)->SetAlignment(DT_LEFT);
	m_Grid2.GetColumns()->GetAt(1)->SetAlignment(DT_LEFT);
	m_Grid2.GetColumns()->GetAt(2)->SetAlignment(DT_LEFT);
	m_Grid2.GetColumns()->GetAt(3)->SetAlignment(DT_LEFT);

	// Start
	BeginApp();
	SetTimer(IDC_TIMER, 1000, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CLogClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLogClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLogClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLogClientDlg::BeginApp()
{
	// 0:
	InitializeSRWLock(&m_lockCreateApp);

	// 1: 監控檔案物件
	m_ptLogFile = new CLogFileList(CLogFileList::ThreadFunc, this, CString(theApp.m_chFileDir));
	//AcquireSRWLockExclusive(&m_lockCreateApp); 
	VERIFY(m_ptLogFile->CreateThread());
	
	// 2: 傳送檔案物件
	m_ptLogSend = new CLogSend(CLogSend::ThreadFunc, this, theApp.m_chServerIP, theApp.m_uServerPORT);
	//AcquireSRWLockExclusive(&m_lockCreateApp); 
	VERIFY(m_ptLogSend->CreateThread());

	m_Status = 1;
}

void CLogClientDlg::EndApp()
{
	 
}

void CLogClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	m_FileCount = m_ptLogFile->GetFileCount();
	m_FileSize = m_ptLogFile->GetFileSize();

	// 更新GRID2
	if (m_ptLogFile->IsUpdate())
	{
		if ((m_Grid2.GetRecords()->GetCount() <= 100) && (m_Grid2.GetRecords()->GetCount() != m_FileCount))
		{
			m_Grid2.SetVirtualMode(new CGrid2Record(this), m_FileCount);
			m_Grid2.Populate();
		}
		m_ptLogFile->ResetUpdate();
	}

	m_Grid2.RedrawControl();

	// 更新GRID1
	switch (m_Status)
	{
	case 0:
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(2);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("初始設定...");
			break;
		}
	case 1: // 正常
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(0);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("正常");
			break;
		}
	case 2: // 停止
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(1);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("停止");
			break;
		}
	case 3: // 更新設定...
		{
			m_Grid1.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(2);
			((CXTPReportRecordItemText*)(m_Grid1.GetRecords()->GetAt(0)->GetItem(1)))->SetValue("更新設定...");
			break;
		}
	default:
		{
			break;
		}
	}
	((CGrid1Record*)m_Grid1.GetRecords()->GetAt(1))->UpdateRecord(1, m_ptLogSend->m_strStatus);
	((CGrid1Record*)m_Grid1.GetRecords()->GetAt(3))->UpdateRecord(1, m_FileCount);
	((CGrid1Record*)m_Grid1.GetRecords()->GetAt(4))->UpdateRecord(1, m_FileSize);
	m_Grid1.RedrawControl();
	
	return;
}

