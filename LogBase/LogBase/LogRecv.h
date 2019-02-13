
// LogRecv.h 
//

#pragma once
#include "mythread.h"
#include "LogBase.h"

// definition
#define MAX_BUFFER_SIZE 4*1024*1024
#define MAX_PACKET_SIZE MAX_BUFFER_SIZE-4
#define MAX_CONTENT_SIZE MAX_PACKET_SIZE-4096

#define	ACCEPT_ADDRESS_SIZE sizeof(sockaddr_in)+16
#define	ACCEPT_BUFFER_SIZE 1000-(ACCEPT_ADDRESS_SIZE*2)

#define PACKET_WRITE		0
#define PACKET_READONLY		1
#define PACKET_NEWFILE		2
#define PACKET_DELETEFILE	3

typedef enum SocketType
{
	OP_UNKNOWN = 0,
	OP_ACCEPT = 1,
	OP_SEND = 2,
	OP_RECV = 3
} CSocketType;

// --- For Network Data Transfer
typedef struct Buf
{
	SOCKET			socket;
	CSocketType		opType;
	LPVOID			ptMaster;
} CBuf;

typedef struct AcceptBuf
{
	CBuf			base;
	UINT			uDatalen;
	char			chData[1000];
} CAcceptBuf;

typedef struct RecvBuf
{
	CBuf			base;
	char			chRemoteIP[20];
	UINT			uRemotePORT;
	WSAOVERLAPPED	overlapped;
	HANDLE			hWaitEventv;
	UINT			uDatalen;
	char			chData[MAX_PACKET_SIZE];
} CRecvBuf;

typedef struct SendBuf
{
	CBuf			base;
	char			chRemoteIP[20];
	UINT			uRemotePORT;
	WSAOVERLAPPED	overlapped;
	HANDLE			hWaitEvent;
	DWORD			dwDatalen;
	char			chData[MAX_PACKET_SIZE];
} CSendBuf;
// --- End

// --- For File Management
typedef struct LogInfo
{
	HANDLE			handle;
	BOOL			attribute;
	UINT64			size;
	FILETIME		lastwrite;
} CLogInfo;

typedef struct Log
{
	CLogInfo		info;
	char			chFilepath[256];
	char			chFilename[256];
	char			chMtkname[256];
	UINT			uMtkver;

	OVERLAPPED		overlapped;
	SRWLOCK			lock;
} CLog;
// --- End


class CLogRecver : public CMyThread
{
public:
	CLogRecver(AFX_THREADPROC pfnThreadProc, LPVOID param, SOCKET socket, char* host, UINT port);
	~CLogRecver(void);

protected:
	virtual void Go(void);
};


class CLogRecv : public CMyThread
{
public:
	CLogRecv(AFX_THREADPROC pfnThreadProc, LPVOID param);
	~CLogRecv(void);

protected:
	virtual void Go(void);
	SOCKET m_socket;
	HANDLE m_iocp;
	
	WSAOVERLAPPED m_overlapped_accept;
	WSAOVERLAPPED m_overlapped_send;
	WSAOVERLAPPED m_overlapped_recv;

	HANDLE m_hWaitEvent_accept;
	HANDLE m_hWaitEvent_send;
	HANDLE m_hWaitEvent_recv;

	DWORD static WINAPI m_fnIocpProc(PVOID lpParameter);
	DWORD static WINAPI m_fnAcceptProc(PVOID lpParameter);
	DWORD static WINAPI m_fnSendProc(PVOID lpParameter);
	DWORD static WINAPI m_fnRecvProc(PVOID lpParameter);

	LPFN_ACCEPTEX m_pfAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS m_pfGetAcceptExSockaddrs;

	BOOL CreateIocp(void);
	void CloseIocp(void);
	BOOL AssociateIocp(HANDLE hSocket, PVOID pKey);

	BOOL CreateSocket(void);
	void DeleteSocket(void);

	CMapStringToPtr m_mapNameToLogFile;
	CMapStringToPtr m_mapHandleToLogFile;
	SRWLOCK m_lock;
	BOOL m_debug;

public:
};