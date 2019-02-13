
// LogNet.h : 定義網路功能類別。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include "tools\mythread.h"
#include "syslog\syslogclient.h"

// definition
#define MAX_BUFFER_SIZE 4*1024*1024
#define MAX_PACKET_SIZE MAX_BUFFER_SIZE-4
#define MAX_CONTENT_SIZE MAX_PACKET_SIZE-4096

#define PACKET_WRITE 0
#define PACKET_READONLY 1
#define PACKET_NEWFILE 2
#define PACKET_DELETEFILE 3

// thread variable
//extern __declspec(thread) ZSTD_CCtx* m_zstd;

// compress type
const unsigned int	NO_COMPRESS		= 0x00000000;	// 不壓縮
const unsigned int	ZSTD_COMPRESS	= 0x80000000;	// Zstd壓縮	

// structure WORK,PACKET
typedef struct CLogFile LOGFILE;


// 1項工作
typedef struct CLogNetWork
{
	LOGFILE*		m_file;
//	OVERLAPPED		m_overlapped;
	DWORD			m_heartbeat;
	BOOL			m_done;
} WORK;

// 1個封包
typedef struct CLogNetPacket
{
	UINT			m_pktlen;
	char			m_pkt[MAX_PACKET_SIZE];
} PACKET;

// 1個需求
typedef struct CLogNetHeader
{
	HANDLE			m_handle;		// 4byte
	UINT			m_attribute;	// 4byte
	UINT64			m_offset;		// 8byte
	FILETIME		m_lastwrite;	// 8byte
	UINT			m_datalen;		// 4byte
	char			m_dummy[4];		// 4byte
} HEADER;


// class CDFNetMaster
// - Net管理員
class CLogSend : public CMyThread
{
public:
	CLogSend(AFX_THREADPROC pfnThreadProc, LPVOID param, char* host, UINT port);
	~CLogSend(void);

protected:
	virtual void	Go(void);
	SOCKET			m_socket;

	PACKET*			m_packet;
	char*			m_sendbuf;
	char*			m_recvbuf;

	bool			CreateSocket();
	void			DeleteSocket();
	void			UpdateLogList();

	// note:自己用的,不用鎖
	CPtrList		m_listLogFile;

public:
	// note:靜態資料,不用鎖
	char			m_NetServerIP[16];
	UINT			m_NetServerPort;
	
	// note:給UI顯示的資訊,要鎖
	SRWLOCK			m_NetServerStatusLock;
	DWORD			m_NetServerLastTick;
	UINT			m_NetServerFileCount;
	UINT64			m_NetSendVolumn;				// 每秒傳送量
	UINT64			m_NetSendMaxVolumn;				// 每秒最大傳送量
	UINT64			m_NetSendTotalVolumn;			// 總傳送量
	UINT64			m_NetProcVolumn;				// 每秒處理量(壓縮前)
	UINT64			m_NetProcMaxVolumn;				// 每秒最大處理量(壓縮前)
	UINT64			m_NetProcTotalVolumn;			// 總處理量(壓縮前)
	CString			m_strStatus;

	void			ForceStopMaster();
};


