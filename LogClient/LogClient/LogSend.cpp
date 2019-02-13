
// LogNet.cpp : 網路功能類別實作。
//

#include "stdafx.h"
#include "LogSend.h"
#include "LogClient.h"
#include "LogClientDlg.h"
#include "zstd.h"

__declspec(thread) ZSTD_CCtx* m_zstd;

// CLogSend class implementation
// 
CLogSend::CLogSend(AFX_THREADPROC pfnThreadProc, LPVOID param, char* host, UINT port)
: CMyThread(pfnThreadProc, param)
, m_socket(NULL)
, m_packet(NULL)
, m_sendbuf(NULL)
, m_recvbuf(NULL)
, m_NetServerLastTick(0)
, m_NetServerFileCount(0)
, m_NetSendVolumn(0)				// 每秒傳送量
, m_NetSendMaxVolumn(0)				// 每秒最大傳送量
, m_NetSendTotalVolumn(0)			// 總傳送量
, m_NetProcVolumn(0)				// 每秒處理量(壓縮前)
, m_NetProcMaxVolumn(0)				// 每秒最大處理量(壓縮前)
, m_NetProcTotalVolumn(0)			// 總處理量(壓縮前)
, m_strStatus("初始連線")
{
	m_listLogFile.RemoveAll();

	memset(m_NetServerIP, NULL, 16);
	memcpy(m_NetServerIP, host, strlen(host));
	m_NetServerPort = port;
	
	InitializeSRWLock(&m_NetServerStatusLock);

	AcquireSRWLockExclusive(&m_NetServerStatusLock);
	m_NetServerLastTick = GetTickCount();
	ReleaseSRWLockExclusive(&m_NetServerStatusLock);
}

CLogSend::~CLogSend(void)
{
	m_listLogFile.RemoveAll();
}

/*virtual */void CLogSend::Go(void)
{
	char chLog[256] = {0};

//	m_zstd = ZSTD_createCCtx();
	BOOL bReset = FALSE;

START:
	while (1)
	{
		if (IsStop())
			break;

		if (CreateSocket())
			break;
		
		m_strStatus = "連線失敗";
		LogSend("連線失敗!重新連線...", ID_SYSLOG_SEVERITY_WARNING);
		Sleep(10000); // 10秒
	}

	m_strStatus = "連線成功";

	// For Send
	m_packet = new PACKET();
	memset(m_packet, NULL, sizeof(PACKET));

	// For Impress Send
	m_sendbuf = new char[sizeof(PACKET)];
	memset(m_sendbuf, NULL, sizeof(PACKET));

	// For Recv
	m_recvbuf = new char[20000];
	memset(m_recvbuf, NULL, 20000);

	// Initial WSA Buffer For Send/Recv
	WSABUF SendDataBuf;
	memset(&SendDataBuf, NULL, sizeof(WSABUF));

	WSABUF RecvDataBuf;
	memset(&RecvDataBuf, NULL, sizeof(WSABUF));

	// Initial WSA Overlapped For Send
	WSAEVENT SendEvent = WSACreateEvent();
	if (SendEvent == WSA_INVALID_EVENT)
	{
		sprintf_s(chLog, "WSACreateEvent失敗(Send)![%d]", WSAGetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		closesocket(m_socket);
		
		EndThread();
		return;
	}

	WSAOVERLAPPED SendOverlapped;
	memset(&SendOverlapped, NULL, sizeof(WSAOVERLAPPED));
	SendOverlapped.hEvent = SendEvent;

	// Initial WSA Overlapped For Recv
	WSAEVENT RecvEvent = WSACreateEvent();
	if (RecvEvent == WSA_INVALID_EVENT)
	{
		sprintf_s(chLog, "WSACreateEvent失敗(Recv)![%d]", WSAGetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		closesocket(m_socket);

		EndThread();
		return;
	}

	WSAOVERLAPPED RecvOverlapped;
	memset(&RecvOverlapped, NULL, sizeof(WSAOVERLAPPED));
	RecvOverlapped.hEvent = RecvEvent;

	// 改為不集合送出模式
	BOOL bOptVal = TRUE;
	int	bOptLen = sizeof(BOOL);
	int err = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOptVal, bOptLen);
    if (err==SOCKET_ERROR) 
	{
		sprintf_s(chLog, "setsockopt錯誤![%d]", GetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
        closesocket(m_socket);
		
		EndThread();
		return;
    } 

	// 正式來
	DWORD dwTimeout = GetTickCount();
	DWORD dwTime;
	while (!IsStop())
	{
		dwTime = GetTickCount();

		// Timeout
		//if ((dwTime-dwTimeout) > 100000) // 100秒
		//{
		//	sprintf_s(chLog, "[主機:%s]沒有任何回應!踢掉[%u]", m_NetServerIP, dwTime-dwTimeout);
		//	LogSend(chLog, ID_SYSLOG_SEVERITY_INFORMATIONAL);
		//	break;
		//}
		
		UpdateLogList();
		
		// 封包清乾淨
		memset(m_packet, NULL, sizeof(m_packet));
		
		// 初始值
		UINT total = 0;
		UINT remain = 0;
		WORK* work = NULL;
		
		for (POSITION pos = m_listLogFile.GetHeadPosition(); pos != NULL; )
		{
			if (total+sizeof(LOGFILEINFO)+sizeof(DWORD) > MAX_PACKET_SIZE)
			{
				break; // 滿囉
			}

			remain = MAX_PACKET_SIZE-total/*目前封包大小*/-sizeof(LOGFILEINFO)/*最小單位封包*/-sizeof(DWORD)/*單位長度*/;
			dwTime = GetTickCount();
			
			work = (WORK*)m_listLogFile.GetNext(pos);
			if (work == NULL)
			{
				LogSend("奇怪的事情 NULL的檔案!", ID_SYSLOG_SEVERITY_ERROR);
				continue;
			}

			//if (work->m_file->m_info.m_attribute)
			//{
			//	sprintf_s(chLog, "檔案已結束![%s]", work->m_file->m_chFilename);
			//	LogSend(chLog, ID_SYSLOG_SEVERITY_WARNING);

			//	work->m_done = TRUE;
			//	work->m_file->m_bDone = TRUE;
			//	continue;
			//}

			if (work->m_file->m_info.m_handle == NULL)
			{
				sprintf_s(chLog, "檔案已關閉![%s]", work->m_file->m_chFilename);
				LogSend(chLog, ID_SYSLOG_SEVERITY_WARNING);

				work->m_done = TRUE;
				work->m_file->m_bDone = TRUE;
				continue;
			}

			if (bReset || work->m_heartbeat == 0)
			{
				// 全新的檔案
				DWORD filenamelen = strlen(work->m_file->m_chFilename);
				if (remain < filenamelen) break; // 滿囉

				*(HANDLE*)&m_packet->m_pkt[total] = work->m_file->m_info.m_handle;
				*(DWORD*)&m_packet->m_pkt[total+4] = 2;
				*(UINT64*)&m_packet->m_pkt[total+8] = /*(UINT64)work->m_overlapped.Offset*/(UINT64)work->m_file->m_overlapped.Offset;
				*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_file->m_info.m_lastwrite;
				*(UINT*)&m_packet->m_pkt[total+24] = filenamelen;
				memcpy(&m_packet->m_pkt[total+28], work->m_file->m_chFilename, filenamelen);
				
				total += (28+filenamelen);
				work->m_heartbeat = dwTime;
			}
			else if (/*(UINT64)work->m_overlapped.Offset*/(UINT64)work->m_file->m_overlapped.Offset < work->m_file->m_info.m_size)
			{
				// 有還沒傳完的資料
				DWORD retlen = 0;
				char *buf = &m_packet->m_pkt[total+28];

				if (!ReadFile(work->m_file->m_info.m_handle, buf, remain, &retlen, /*&work->m_overlapped*/&work->m_file->m_overlapped))
				{
					if (GetLastError() == ERROR_IO_PENDING)
					{
						DWORD dw = WaitForSingleObject(/*work->m_overlapped.hEvent*/work->m_file->m_overlapped.hEvent, INFINITE);
						if(dw == WAIT_OBJECT_0)
						{
							if (GetOverlappedResult(work->m_file->m_info.m_handle, /*&work->m_overlapped*/&work->m_file->m_overlapped, &retlen, TRUE) != 0)   
							{
								if (retlen <= 0) 
								{
									sprintf_s(chLog, "奇怪的事情 沒讀到資料![%d][%s]", retlen, work->m_file->m_chFilename);
									LogSend(chLog, ID_SYSLOG_SEVERITY_WARNING);
								}
							}
						}
						else
						{
							// ERROR
							sprintf_s(chLog, "奇怪的讀檔回傳值![%d][%s]", dw, work->m_file->m_chFilename);
							LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
							continue;
						}
					}
					else
					{
						// ERROR
						sprintf_s(chLog, "檔案讀取失敗![%d][%s]", GetLastError(), work->m_file->m_chFilename);
						LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
						continue;
					}
				}

				ResetEvent(/*work->m_overlapped.hEvent*/work->m_file->m_overlapped.hEvent);

				// CheckReadFile
				if (retlen <= 0)
				{
					sprintf_s(chLog, "Readfile異常!?![%s][%d]", work->m_file->m_chFilename, retlen);
					LogSend(chLog, ID_SYSLOG_SEVERITY_WARNING);
				}

				*(HANDLE*)&m_packet->m_pkt[total] = work->m_file->m_info.m_handle;
				*(DWORD*)&m_packet->m_pkt[total+4] = 0;
				*(UINT64*)&m_packet->m_pkt[total+8] = /*(UINT64)work->m_overlapped.Offset*/(UINT64)work->m_file->m_overlapped.Offset;
				*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_file->m_info.m_lastwrite;
				*(UINT*)&m_packet->m_pkt[total+24] = retlen;
				
				total += (28+retlen);
				/*work->m_overlapped.Offset*/work->m_file->m_overlapped.Offset += retlen;
				work->m_heartbeat = dwTime;
			}
			else
			{
				// 沒有資料要傳
				BOOL bIsReadonly = work->m_file->m_info.m_attribute;
				
				// 檔案閒置超過10秒 或者 檔案屬性ReadOnly => 要送HB
				if (bIsReadonly || dwTime-work->m_heartbeat >= 10000) 
				{
					*(HANDLE*)&m_packet->m_pkt[total] = work->m_file->m_info.m_handle;
					*(DWORD*)&m_packet->m_pkt[total+4] = bIsReadonly ? 3 : work->m_file->m_info.m_attribute;
					*(UINT64*)&m_packet->m_pkt[total+8] = /*(UINT64)work->m_overlapped.Offset*/(UINT64)work->m_file->m_overlapped.Offset;
					*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_file->m_info.m_lastwrite;
					*(UINT*)&m_packet->m_pkt[total+24] = 0;

					total += 28;
					work->m_heartbeat = dwTime;
				}

				// 檔案屬性ReadOnly 而且 檔案已全數送完畢 => 該退場了
				if (bIsReadonly && (/*(UINT64)work->m_overlapped.Offset*/(UINT64)work->m_file->m_overlapped.Offset == work->m_file->m_info.m_size)) 
				{
					sprintf_s(chLog, "傳輸完成![檔案:%s][大小:%d]", work->m_file->m_chFilename, work->m_file->m_info.m_size);
					LogSend(chLog, ID_SYSLOG_SEVERITY_INFORMATIONAL);
					
					work->m_done = TRUE;
				}
			}
		}

		if (bReset)
			bReset = FALSE;

		if (total <= 0)
		{
			// 沒新的檔案 + 沒資料可以傳 + 沒HeartBeat需要傳 => 處罰100ms
			Sleep(100);
		}
		else
		{
			// 有任何東西需要傳送
			DWORD sendlen = 0;
			m_sendbuf[0] = 0;

			m_packet->m_pktlen = total;

			if ((m_packet->m_pktlen>256) && m_zstd && theApp.m_ptDict) // 壓縮
			{
				UINT nOutLen = 0;
				if (!ZSTD_isError(nOutLen=(int)ZSTD_compress_usingCDict(m_zstd, (char*)m_sendbuf+sizeof(int), MAX_PACKET_SIZE, (const char*)m_packet, m_packet->m_pktlen+sizeof(int), theApp.m_ptDict)))
				{	
					// 壓縮成功
					*(int*)m_sendbuf = nOutLen | ZSTD_COMPRESS;	// len(OR)壓縮旗標

					SendDataBuf.len = nOutLen+4;
					SendDataBuf.buf = (char*)m_sendbuf;
				}
				else
				{
					// 壓縮失敗
					sprintf_s(chLog, "奇怪的事情 壓縮失敗![%s][%d/%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
					LogSend(chLog, ID_SYSLOG_SEVERITY_DEBUG);

					SendDataBuf.len = m_packet->m_pktlen+4;
					SendDataBuf.buf = (char*)m_packet;
				}

				DWORD dFlag = 0;
				if (WSASend(m_socket, &SendDataBuf, 1, &sendlen, 0, &SendOverlapped, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSA_IO_PENDING)
					{
						DWORD dw = WaitForSingleObject(SendOverlapped.hEvent, INFINITE);
						if (dw == WAIT_OBJECT_0)
						{
							if (!WSAGetOverlappedResult(m_socket, &SendOverlapped, &sendlen, FALSE, &dFlag))
							{
								if (sendlen <= 0)
								{
									sprintf_s(chLog, "奇怪的事情 沒寫入資料![%d]", sendlen);
									LogSend(chLog, ID_SYSLOG_SEVERITY_DEBUG);
								}
							}
						}
					}
					else
					{
						sprintf_s(chLog, "WSASend失敗![%d]", WSAGetLastError());
						LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
						break;
					}
				}

				WSAResetEvent(SendOverlapped.hEvent);

				if (SendDataBuf.len != sendlen)
				{
					sprintf_s(chLog, "奇怪的事情 送出的大小不一樣![com][%d][%d]", nOutLen+4, sendlen);
					LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
				}
			}
			else // 不壓縮
			{
				SendDataBuf.len = m_packet->m_pktlen+4;
				SendDataBuf.buf = (char*)m_packet;

				DWORD dFlag = 0;
				DWORD n = 0;
				if (WSASend(m_socket, &SendDataBuf, 1, &sendlen, 0, &SendOverlapped, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSA_IO_PENDING)
					{
						DWORD dw = WaitForSingleObject(SendOverlapped.hEvent, INFINITE);
						if (dw == WAIT_OBJECT_0)
							if (!WSAGetOverlappedResult(m_socket, &SendOverlapped, &sendlen, FALSE, &dFlag))
							{
								if (sendlen <= 0)
								{
									sprintf_s(chLog, "奇怪的事情 沒寫入資料![%d]", sendlen);
									LogSend(chLog, ID_SYSLOG_SEVERITY_DEBUG);
								}
							}
					}
					else
					{
						sprintf_s(chLog, "WSASend失敗![%d]", WSAGetLastError());
						LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
						break;
					}
				}

				WSAResetEvent(SendOverlapped.hEvent);

				//TRACE("[%I64d]%d\n", tra++, sendlen);

				if (SendDataBuf.len != sendlen)
				{
					sprintf_s(chLog, "奇怪的事情 送出的大小不一樣![%d][%d]", total+4, sendlen);
					LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
				}
			}

			// Check WSASend
			if (sendlen == 0)
			{
				LogSend("有資料要傳送卻送出0Byte!(斷線!?)", ID_SYSLOG_SEVERITY_DEBUG);
			}

			// 更新連線資訊(For UI)
			AcquireSRWLockExclusive(&m_NetServerStatusLock);

			// 每秒重置流量計算
			if ((GetTickCount()-m_NetServerLastTick) >= 1000)
			{	
				m_NetServerLastTick = GetTickCount();
				m_NetSendVolumn = 0;
				m_NetProcVolumn = 0;
			}

			m_NetSendVolumn += sendlen;
			m_NetSendTotalVolumn += sendlen;
			m_NetProcVolumn += total;
			m_NetProcTotalVolumn += total;

			if (m_NetSendVolumn > m_NetSendMaxVolumn) m_NetSendMaxVolumn = m_NetSendVolumn;
			if (m_NetProcVolumn > m_NetProcMaxVolumn) m_NetProcMaxVolumn = m_NetProcVolumn;

			m_NetServerFileCount = m_listLogFile.GetCount();

			ReleaseSRWLockExclusive(&m_NetServerStatusLock); 
			dwTimeout = dwTime;
		}
	}

	if (!IsStop())
	{
		LogSend("自動重新連線", ID_SYSLOG_SEVERITY_WARNING);
		closesocket(m_socket);
		m_strStatus = "重新連線";
		m_socket = NULL;
		bReset = TRUE;
		goto START;
	}
}

bool CLogSend::CreateSocket()
{
	char chLog[256];

	DeleteSocket();

	if ((m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		LogSend("Socket Error(Invalid Socket)", ID_SYSLOG_SEVERITY_ERROR);
		m_socket = NULL;
		return FALSE;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, NULL, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(m_NetServerPort);
	server_addr.sin_addr.s_addr = inet_addr(m_NetServerIP);

	if (connect(m_socket, (SOCKADDR*)&server_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		sprintf_s(chLog, "[%s:%d]Connect Error(%d)", m_NetServerIP, m_NetServerPort, GetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		closesocket(m_socket);
		m_socket = NULL;
		return FALSE;
	}

	return TRUE;
}

void CLogSend::DeleteSocket()
{
	if (m_socket)
		closesocket(m_socket);
	
	m_socket = NULL;
}

void CLogSend::UpdateLogList()
{
	CLogClientDlg* parent = (CLogClientDlg*)GetParentsHandle();

//	AcquireSRWLockExclusive(&parent->m_ptLogFile->m_lockRefreshFileList);

	for (int i=0; i<100; i++)
	{
		LOGFILE* log = parent->m_ptLogFile->m_arrMtkFile[i];
		
		if (log && !log->m_bProc)
		{
			WORK* work = new WORK();
			memset(work, NULL, sizeof(WORK));

			work->m_file = log;
			work->m_heartbeat = 0;

			memcpy(&work->m_file->m_overlapped.Offset, &log->m_info.m_size, sizeof(UINT64));
			work->m_file->m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			// LOCK?
			log->m_bProc = TRUE;

			m_listLogFile.AddTail(work);
		}
	}

	POSITION pos = m_listLogFile.GetHeadPosition();
	POSITION prepos = NULL;
	while (pos != NULL)
	{
		prepos = pos;
		WORK* work = (WORK*)m_listLogFile.GetNext(pos);
		if (work && work->m_done)
		{
			work->m_file->m_bDone = TRUE;
			m_listLogFile.RemoveAt(prepos);

			delete work;
			work = NULL;
		}
	}

//	ReleaseSRWLockExclusive(&parent->m_ptLogFile->m_lockRefreshFileList);
}



