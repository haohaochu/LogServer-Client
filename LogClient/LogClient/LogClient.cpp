// LogClient.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LogClient.h"
#include "LogClientDlg.h"

#pragma comment(lib, "Version.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLogClientApp

BEGIN_MESSAGE_MAP(CLogClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CLogClientApp construction

CLogClientApp::CLogClientApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CLogClientApp object

CLogClientApp theApp;


// CLogClientApp initialization

BOOL CLogClientApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CLogClientDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CLogClientApp::GetIni()
{
	char szIniPath[261]={0}, szFolder[260]={0}, szAppName[256]={0}, szDrive[3]={0};
	CString	strbuf;
	CString strExecutePath;
	
	// then try current working dir followed by app folder
	GetModuleFileName(NULL, szIniPath, sizeof(szIniPath)-1);
	_splitpath_s(szIniPath, szDrive, 3, szFolder, 260, szAppName, 256, NULL, 0);
	
	// chNowDirectory
	GetCurrentDirectory(sizeof(szIniPath)-1, szIniPath);
	strExecutePath.Format("%s\\", szIniPath);

	// SYSTEM
	_makepath_s(szIniPath, NULL, szIniPath, szAppName, ".ini");
	free((void*)m_pszProfileName);
	m_pszProfileName = _strdup(szIniPath);

	// 監控目錄
	memset(m_chFileDir, NULL, 256);
	GetPrivateProfileString("SYSTEM", "PATH", "d:\\quote\\log", m_chFileDir, 256, szIniPath);	
	if(m_chFileDir[strlen(m_chFileDir)-1] != '\\')
		strcat_s(m_chFileDir, "\\");

	// 程式標題
	m_strTitle.Format("%s-%s%s", szAppName, szDrive, szFolder);

	// SERVER資訊
	memset(m_chServerIP, NULL, 16);
	GetPrivateProfileString("SYSTEM", "SERVERIP", "0.0.0.0", m_chServerIP, 16, szIniPath);
	m_uServerPORT = GetPrivateProfileInt("SYSTEM", "SERVERPORT", 0, szIniPath);

	// 壓縮設定
	m_bCompress = GetPrivateProfileInt("SYSTEM", "COMPRESS", 0, szIniPath) == 0 ? FALSE : TRUE;

	// 字典名稱
	memset(m_chDict, NULL, 256);
	GetPrivateProfileString("SYSTEM", "DICT", "zstd170125.dic", m_chDict, 256, szIniPath);

	CString strDict("");
	strDict.Format("%sdic\\%s", strExecutePath, m_chDict);
	HANDLE hDict = NULL;
	if (!m_bCompress)
	{
		m_ptDict = NULL;
	}
	else if ((hDict = CreateFile(strDict, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	{
		LogSend("字典檔開檔失敗", ID_SYSLOG_SEVERITY_ERROR);
		m_ptDict = NULL;
	}
	else
	{
		DWORD len = 256*1024;
		DWORD ret = 0;
		char* buf = new char[len];
		memset(buf, NULL, len);
		if (!ReadFile(hDict, buf, len, &ret, NULL))
		{
			LogSend("字典檔讀取失敗", ID_SYSLOG_SEVERITY_ERROR);
			m_ptDict = NULL;
		}
		else
		{
			if ((m_ptDict = ZSTD_createCDict(buf, ret, 1)) == NULL)
			{
				LogSend("字典檔建立失敗", ID_SYSLOG_SEVERITY_ERROR);
				m_ptDict = NULL;
				m_uDictSize = 0;
			}
			else
				m_uDictSize = ret;
		}

		delete buf;
	}

	GetVersion();
}

void CLogClientApp::SetInt()
{
	WriteProfileString("SYSTEM", "PATH", m_chFileDir);
	WriteProfileString("SYSTEM", "SERVERIP", m_chServerIP);
	WriteProfileInt("SYSTEM", "SERVERPORT", m_uServerPORT);

	WriteProfileInt("SYSTEM", "COMPRESS", m_bCompress ? 1 : 0);
	WriteProfileString("SYSTEM", "DICT", m_chDict);
}

void CLogClientApp::GetVersion()
{
	char szFileName[128]={0};
	char chLog[128]={0};
	
	// 取程式路徑
	if (GetModuleFileName(NULL, szFileName, sizeof(szFileName)) == 0)
	{
		sprintf_s(chLog, "GetVersion GetModuleFileName() Error(%d)", GetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	DWORD dwInfoSize;
	if ((dwInfoSize=GetFileVersionInfoSize(szFileName,NULL)) == 0)
	{
		sprintf_s(chLog, "GetVersion GetFileVersionInfoSize(%s) Error(%d)", szFileName, GetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		return;
	}
    
	char* pVersionInfo = NULL;

	if ((pVersionInfo=new char[dwInfoSize+1]) == NULL)
	{
		LogSend("[CPats2mtkApp::GetVersion()]New Memory Failure!", ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	ZeroMemory(pVersionInfo,dwInfoSize+1);

	if (!GetFileVersionInfo(szFileName,NULL,dwInfoSize,pVersionInfo))
	{
		delete [] pVersionInfo;
		sprintf_s(chLog, "GetVersion GetFileVersionInfo(%s) Error(%d)", szFileName, GetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	}*lpTranslate;
	
	UINT cbTranslate;
	// Read the list of languages and code pages.

    if (!VerQueryValue(pVersionInfo,TEXT("\\"),(LPVOID*)&lpTranslate,&cbTranslate) || cbTranslate == 0)
	{
		delete [] pVersionInfo;
		sprintf_s(chLog, "GetVersion VerQueryValue(%s) Error", szFileName);
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	m_strVersion.Format("Ver. %d.%d.%d.%d", lpTranslate[2].wCodePage, lpTranslate[2].wLanguage, lpTranslate[3].wCodePage, lpTranslate[3].wLanguage);

	delete [] pVersionInfo;
}
	
