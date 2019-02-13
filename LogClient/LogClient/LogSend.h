
// LogNet.h : �w�q�����\�����O�C
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�� PCH �]�t���ɮ׫e���]�t 'stdafx.h'"
#endif

#include "resource.h"		// �D�n�Ÿ�
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
const unsigned int	NO_COMPRESS		= 0x00000000;	// �����Y
const unsigned int	ZSTD_COMPRESS	= 0x80000000;	// Zstd���Y	

// structure WORK,PACKET
typedef struct CLogFile LOGFILE;


// 1���u�@
typedef struct CLogNetWork
{
	LOGFILE*		m_file;
//	OVERLAPPED		m_overlapped;
	DWORD			m_heartbeat;
	BOOL			m_done;
} WORK;

// 1�ӫʥ]
typedef struct CLogNetPacket
{
	UINT			m_pktlen;
	char			m_pkt[MAX_PACKET_SIZE];
} PACKET;

// 1�ӻݨD
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
// - Net�޲z��
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

	// note:�ۤv�Ϊ�,������
	CPtrList		m_listLogFile;

public:
	// note:�R�A���,������
	char			m_NetServerIP[16];
	UINT			m_NetServerPort;
	
	// note:��UI��ܪ���T,�n��
	SRWLOCK			m_NetServerStatusLock;
	DWORD			m_NetServerLastTick;
	UINT			m_NetServerFileCount;
	UINT64			m_NetSendVolumn;				// �C��ǰe�q
	UINT64			m_NetSendMaxVolumn;				// �C��̤j�ǰe�q
	UINT64			m_NetSendTotalVolumn;			// �`�ǰe�q
	UINT64			m_NetProcVolumn;				// �C��B�z�q(���Y�e)
	UINT64			m_NetProcMaxVolumn;				// �C��̤j�B�z�q(���Y�e)
	UINT64			m_NetProcTotalVolumn;			// �`�B�z�q(���Y�e)
	CString			m_strStatus;

	void			ForceStopMaster();
};


