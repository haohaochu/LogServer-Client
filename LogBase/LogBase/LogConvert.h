
// LogConvert.h
//

#pragma once
#include "mythread.h"
#include "LogBase.h"
#include <curl/curl.h>

typedef struct CPacket
{
	char* content;
	size_t contentlen;
} CPacket;

#pragma pack(1)
typedef struct LogHeader 
{
	int			len;
	char		type;
	__time64_t	time;
	WORD		index;
	WORD		version;
	char		userid[1];
} CLogHeader;

typedef struct LogBody
{
	int			len;
	unsigned short		route;
	char		data[1];
} CLogBody;

typedef struct LogBodyEx
{
	int			len;
	__time64_t	time;
	SOCKADDR_IN addr;
	UINT64		mask[3];
//	char		data[1];
} CLogBodyEx;

typedef struct LogApnsGateway
{
	char		msgid[24];
	char		appkey[64];
	char		appsword[24];
	char		catid[3];
	char		pushid[256];
	char		login;
	char		msg[256];		// big5
	char		stk[20];
	char		counts[5];
	char		ctime[15];
	char		action[128];
	char		delimiter;		// 0:fail ,1:success
} CLogApnsGateway;
#pragma pack()

typedef struct LogContent
{

} CLogContent;

typedef struct LogData
{
	CLogHeader header;
	CLogContent content;
} CLogData;

typedef struct FileInfo
{
	LPVOID		master;
	LPVOID		filemaster;
	UINT		fileindex;
} CFileInfo;

class CLogConvert : public CMyThread
{
public:
	CLogConvert(AFX_THREADPROC pfnThreadProc, LPVOID param);
	~CLogConvert(void);

	static size_t RecvFunc(void *ptr, size_t size, size_t nmemb, void *stream);
	CPacket*			m_packet;
	
	static CString ConvertString(char*, int);
	static CString ConvertUTF8(char *);
	static CString MakeStockTerms(CString&);

//	ZSTD_DCtx*			m_zstd;
	ZSTD_DDict*			m_ptDict;
//	CURL*				m_curl;
//	CURLcode			m_curlres;
	
	DWORD static WINAPI m_fnFileProc(PVOID lpParameter);
	DWORD static WINAPI m_fnFileProcAG(PVOID lpParameter);

	SRWLOCK				m_lockUserid;
	CMapStringToPtr		m_mapUserid;

protected:
	virtual void		Go(void);
	
	HANDLE				m_hWait;
	UINT				m_uIndex;
};
