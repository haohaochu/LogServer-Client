// LogClient.h : main header file for the PROJECT_NAME application
//

#pragma once
#pragma comment(lib, ".\\lib\\libzstd.lib")
//#pragma warning (disable : 4005)	// It's a bug in VS2010. intsafe.h and stdint.h both define INT8_MIN...
#include "zstd.h"

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "syslog\syslogclient.h"

// CLogClientApp:
// See LogClient.cpp for the implementation of this class
//

class CLogClientApp : public CWinApp
{
public:
	CLogClientApp();

// Overrides
	public:
	virtual BOOL InitInstance();

public:
	void GetIni();
	void SetInt();
	void GetVersion();
	
	CString				m_strTitle;
	CString				m_strVersion;
	
	char				m_chFileDir[256];
	char				m_chServerIP[16];
	UINT				m_uServerPORT;
	BOOL				m_bCompress;

	char				m_chDict[256];
	ZSTD_CDict*			m_ptDict;
	UINT				m_uDictSize;
// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CLogClientApp theApp;
