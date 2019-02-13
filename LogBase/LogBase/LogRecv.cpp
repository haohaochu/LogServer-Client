
// LogRecv.cpp
//

#include "stdafx.h"
#include "LogRecv.h"
#include "LogBaseDlg.h"
#include "LogBase.h"
#include "syslog\syslogclient.h"

CLogRecver::CLogRecver(AFX_THREADPROC pfnThreadProc, LPVOID param, SOCKET socket, char* host, UINT port)
: CMyThread(pfnThreadProc, param)
{

}

CLogRecver::~CLogRecver(void)
{

}

/*virtual*/ void CLogRecver::Go(void)
{
	// Recv
}


CLogRecv::CLogRecv(AFX_THREADPROC pfnThreadProc, LPVOID param)
: CMyThread(pfnThreadProc, param)
, m_socket(NULL)
, m_iocp(NULL)
{
	m_mapNameToLogFile.RemoveAll();
	m_mapHandleToLogFile.RemoveAll();

	InitializeSRWLock(&m_lock);
	m_debug = FALSE;
}

CLogRecv::~CLogRecv(void)
{

}

/*virtual*/ void CLogRecv::Go(void)
{
	char buf[1024]={0};
	CLogBaseDlg* dlg = (CLogBaseDlg*)GetParentsHandle();

	memset(&m_overlapped_accept, NULL, sizeof(WSAOVERLAPPED));
	memset(&m_overlapped_send, NULL, sizeof(WSAOVERLAPPED));
	memset(&m_overlapped_recv, NULL, sizeof(WSAOVERLAPPED));

	//m_overlapped_accept.hEvent = CreateEvent(NULL, TRUE, FALSE, _T("logbase_net_accept"));
	//m_overlapped_send.hEvent = CreateEvent(NULL, TRUE, FALSE, _T("logbase_net_send"));
	//m_overlapped_recv.hEvent = CreateEvent(NULL, TRUE, FALSE, _T("logbase_net_recv"));

	WORD wVersionRequested;
	WSADATA wsaData;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);	
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		UINT err = GetLastError();
		return;
	}

	while (!IsStop())
	{
		if (m_socket == NULL)
		{
			if (!CreateIocp() || !CreateSocket())
				continue;
		}
		
		DWORD dwBytes = 0;

		// 載入並設定AcceptEx
		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &m_pfAcceptEx, sizeof(m_pfAcceptEx), &dwBytes, NULL, NULL);
		
		// 載入並設定GetAcceptExSockaddrs 
		GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
		WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidGetAcceptExSockaddrs, sizeof(GuidGetAcceptExSockaddrs), &m_pfGetAcceptExSockaddrs, sizeof(m_pfGetAcceptExSockaddrs), &dwBytes, NULL, NULL);

		// Accept時接收Client資訊長度
		CAcceptBuf* pAcceptBuf = new CAcceptBuf();
		memset(pAcceptBuf, NULL, sizeof(CAcceptBuf));
		pAcceptBuf->base.opType = OP_ACCEPT;
		pAcceptBuf->base.ptMaster = this;
		pAcceptBuf->uDatalen = 0;

		// 連結到Iocp
		if (!AssociateIocp((HANDLE)m_socket, pAcceptBuf))
		{	
			int err = WSAGetLastError();
			sprintf_s(buf, "[CLogRecv::Go]連結ListenerSocket到Iocp失敗！[%d]", err);
			LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			closesocket(m_socket);
			m_socket = NULL;
			continue;
		}			
		
		pAcceptBuf->base.socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (pAcceptBuf->base.socket == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			sprintf_s(buf, "[CLogRecv::Go]建立AcceptSocket失敗！[%d]", err);
			LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			closesocket(pAcceptBuf->base.socket);
			pAcceptBuf->base.socket = NULL;
			continue;
		}
		
		if (!m_pfAcceptEx(m_socket, pAcceptBuf->base.socket, pAcceptBuf->chData, pAcceptBuf->uDatalen, ACCEPT_ADDRESS_SIZE, ACCEPT_ADDRESS_SIZE, NULL, &m_overlapped_accept))
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				sprintf_s(buf, "[CLogRecv::Go]呼叫AcceptEx失敗！[%d]", err);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				break;
			}
		}

		//if (!RegisterWaitForSingleObject(&m_hWaitEvent_accept, m_overlapped_accept.hEvent, fnAcceptCompletion, pAcceptBuf, INFINITE, WT_EXECUTEDEFAULT/*WT_EXECUTEINWAITTHREAD*/))
		//{
		//	//LogSend("[AcceptEx]RegisterWaitForSingleObject Failure!!!", ID_SYSLOG_SEVERITY_ERROR);
		//	return;
		//}
		
		// 4個工作緒
		QueueUserWorkItem(&CLogRecv::m_fnIocpProc, this, WT_EXECUTELONGFUNCTION);
		QueueUserWorkItem(&CLogRecv::m_fnIocpProc, this, WT_EXECUTELONGFUNCTION);
		QueueUserWorkItem(&CLogRecv::m_fnIocpProc, this, WT_EXECUTELONGFUNCTION);
		QueueUserWorkItem(&CLogRecv::m_fnIocpProc, this, WT_EXECUTELONGFUNCTION);

		while (!IsStop())
		{
			Sleep(1000000);
		}
	}
}

BOOL CLogRecv::CreateIocp()
{
	CloseIocp();

	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (m_iocp == NULL)
	{	
		int err = WSAGetLastError();
		return FALSE;
	}

	return TRUE;
}

void CLogRecv::CloseIocp(void)
{
	if (m_iocp)
		CloseHandle(m_iocp);

	m_iocp = NULL;
}

BOOL CLogRecv::AssociateIocp(HANDLE hSocket, PVOID pKey)
{
	if (m_iocp == NULL)
		return FALSE;

	if (CreateIoCompletionPort(hSocket, m_iocp, (ULONG_PTR)pKey, 0) != m_iocp)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CLogRecv::CreateSocket()
{
	DeleteSocket();

	// 建立一個新的Socket
	if ((m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
	//	log->DFLogSend(CString("Socket Error(Invalid Socket)"), DF_SEVERITY_DISASTER);
		UINT err = GetLastError();
		closesocket(m_socket);
		m_socket = NULL;
		return FALSE;
	}

	// 綁定伺服器資訊
	struct sockaddr_in server_addr;
	
	memset(&server_addr, NULL, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(theApp.m_uServerPORT); // SET
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(m_socket, (SOCKADDR*)&server_addr, sizeof(struct sockaddr_in)) < 0)
	{
//		strLog.Format("Bind Error (%d)", GetLastError());
//		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		DeleteSocket();
		m_socket = NULL;
		return FALSE;
	}

	// LISTEN
	if (listen(m_socket, 5) < 0)
	{
//		strLog.Format("Listen Error (%d)", GetLastError());
//		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		DeleteSocket();
		m_socket = NULL;
		return FALSE;
	}

	return TRUE;
}

void CLogRecv::DeleteSocket()
{
	if (m_socket)
		closesocket(m_socket);
	
	m_socket = NULL;
}

DWORD /*WINAPI */CLogRecv::m_fnIocpProc(PVOID lpParameter)
{
	char buf[1024] = {0};
	CLogRecv* master = (CLogRecv*)lpParameter;

	while (1)
	{
		// 主要處理事項(Start)		
		DWORD dwBytesTransfered = 0;
		WSAOVERLAPPED* pOL = NULL;
		CBuf* pBuf = NULL;

		BOOL bReturn = GetQueuedCompletionStatus(master->m_iocp, &dwBytesTransfered, (PULONG_PTR)&pBuf, &pOL, INFINITE);

		if (!pBuf)
		{
			// ERROR
			sprintf_s(buf, "IOCP錯誤[ERR][資料有誤]");
			LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
			continue;
		}

		UINT uRet = 0;
		switch (pBuf->opType)
		{
		case OP_ACCEPT:
			
			// ACCEPT=>新增一個連線
			uRet = QueueUserWorkItem(&CLogRecv::m_fnAcceptProc, pBuf, WT_EXECUTEDEFAULT);
			break;

		case OP_RECV:

			// RECV=>
			uRet = QueueUserWorkItem(&CLogRecv::m_fnRecvProc, pBuf, WT_EXECUTEDEFAULT);
			break;

		case OP_SEND:

			// SEND=>
			uRet = QueueUserWorkItem(&CLogRecv::m_fnSendProc, pBuf, WT_EXECUTEDEFAULT);
			break;

		default:
			break;
		}

		if (!uRet)
		{
			sprintf_s(buf, "IOCP錯誤[ERR][%d:%d]", pBuf->opType, uRet);
			LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
		}
	}
}

DWORD /*WINAPI */CLogRecv::m_fnAcceptProc(PVOID lpParameter)
{
	char buf[1024] = {0};
	CAcceptBuf* ptr = (CAcceptBuf*)lpParameter;
	CLogRecv* master = (CLogRecv*)ptr->base.ptMaster;
	DWORD addrlen = sizeof(sockaddr_in)+16;
//	ResetEvent(master->m_overlapped_accept.hEvent);

	struct sockaddr* Local_addr;
	struct sockaddr* Remote_addr;
	int Local_addrlen;
	int Remote_addrlen;
	master->m_pfGetAcceptExSockaddrs(ptr->chData, ptr->uDatalen, addrlen, addrlen, &Local_addr, &Local_addrlen, &Remote_addr, &Remote_addrlen);

	// 產生新的連線
	CRecvBuf* pnClient = new CRecvBuf();
	memset(pnClient, NULL, sizeof(CRecvBuf));
	pnClient->base.socket = ptr->base.socket;
	pnClient->base.opType = OP_RECV;
	pnClient->base.ptMaster = master;
	pnClient->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, _T("logbase_net_recv"));
	pnClient->uDatalen = 0;

	// 取得SOCKET
	CString strRemoteIP(inet_ntoa(((struct sockaddr_in*)Remote_addr)->sin_addr));
	memcpy(pnClient->chRemoteIP, strRemoteIP, strRemoteIP.GetLength());
	pnClient->uRemotePORT = ((struct sockaddr_in*)Remote_addr)->sin_port;

	// 連結到Iocp
	if (!master->AssociateIocp((HANDLE)pnClient->base.socket, pnClient))
	{	
		closesocket(pnClient->base.socket);
		pnClient->base.socket = NULL;
		
		sprintf_s(buf, "接收連線[ERR][AssociateIocp錯誤]");
		LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
		
		return 1;
	}		

	// 觸發Recv
	DWORD dwFlags = 0;
	WSABUF wsaBuf = {4, (char*)&pnClient->uDatalen};
	if (WSARecv(pnClient->base.socket, &wsaBuf, 1, NULL, &dwFlags, &pnClient->overlapped, NULL) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			sprintf_s(buf, "接收連線[ERR][WSARecv錯誤:%d]", err);
			LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
			return 1;
		}
	}

	// 產生下一個Accept
	ptr->base.socket = NULL;
	memset(ptr->chData, NULL, ptr->uDatalen);
	ptr->base.socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (ptr->base.socket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		sprintf_s(buf, "接收連線[ERR][WSASocket錯誤:%d]", err);
		LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
		return 1;
	}

//	ResetEvent(master->m_overlapped_accept.hEvent);
	if (!master->m_pfAcceptEx(master->m_socket, ptr->base.socket, ptr->chData, 0, ACCEPT_ADDRESS_SIZE, ACCEPT_ADDRESS_SIZE, NULL, &master->m_overlapped_accept))
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			sprintf_s(buf, "接收連線[ERR][AcceptEx錯誤:%d]", err);
			LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
			return 1;
		}
	}

	return 0;
}

DWORD /*WINAPI */CLogRecv::m_fnSendProc(PVOID lpParameter)
{
	CSendBuf* ptr = (CSendBuf*)lpParameter;
	CLogRecv* master = (CLogRecv*)ptr->base.ptMaster;
//	ResetEvent(master->m_overlapped_send.hEvent);

	return 0;
}

DWORD /*WINAPI */CLogRecv::m_fnRecvProc(PVOID lpParameter)
{
	char buf[1024] = {0};
	CRecvBuf* ptr = (CRecvBuf*)lpParameter;
	CLogRecv* master = (CLogRecv*)ptr->base.ptMaster;

	// 檢查有沒有壓縮
	BOOL bCompress = FALSE;
//	BOOL bCompress = ((ptr->uDatalen & ZSTD_COMPRESS) == ZSTD_COMPRESS);
//	ptr->uDatalen &= ~ZSTD_COMPRESS;

	// 把資料收完
	UINT nRecv = 0;
	while (nRecv < ptr->uDatalen)
		nRecv += recv(ptr->base.socket, ptr->chData+nRecv, ptr->uDatalen-nRecv, 0);
	
	// 確認是否收好收滿
	if ((nRecv<=0) || (nRecv != ptr->uDatalen))
	{
		// ERROR LOG
		sprintf_s(buf, "接收檔案[ERR][連線中斷:%s]", ptr->chRemoteIP);
		LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);

		delete ptr;
		ptr = NULL;

		return 1;
	}

	// 處理資料
	//if (bCompress) 
	//{
	//	// 準備工作


	//	// 需解壓縮
	//	UINT nOutLen = 0;
	//	memset(m_depacket, NULL, sizeof(PACKET));
	//	if (!ZSTD_isError(nOutLen=(int)ZSTD_decompress_usingDDict(m_zstd, (char*)m_depacket, sizeof(PACKET), (const char*)m_packet->m_pkt, m_packet->m_pktlen, theApp.m_ptDict/*, theApp.m_uDictSize*/)))
	//	{
	//		// 解壓縮成功
	//		memset(m_packet, NULL, sizeof(PACKET));
	//				
	//		//m_packet->m_pktlen = nOutLen-sizeof(int);	// len(OR)壓縮旗標
	//		memcpy(m_packet, m_depacket, nOutLen);

	//		if (m_packet->m_pktlen != nOutLen-sizeof(int))
	//		{
	//			strLog.Format("奇怪的事情 解壓縮長度不符合![%d][%d]", m_packet->m_pktlen, nOutLen-sizeof(int));
	//			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
	//		}
	//	}
	//	else
	//	{
	//		strLog.Format("解壓縮失敗![%s][%d]", ZSTD_getErrorName(nOutLen), m_packet->m_pktlen);
	//		log->DFLogSend(strLog, DF_SEVERITY_HIGH);
	//		continue;
	//	}
	//}

	// UI介面資料

	// 收到資料
	UINT idx = 0;
	UINT total = ptr->uDatalen;
	BOOL bDisconnect = FALSE;
	DWORD tick = GetTickCount();

	int infolen = sizeof(CLogInfo);
	int wordlen = sizeof(DWORD);

	while (!bDisconnect && (idx < total))
	{
		CLogInfo* info = (CLogInfo*)&ptr->chData[idx];
		DWORD datalen = 0;
		memcpy(&datalen, &ptr->chData[idx+infolen], wordlen);
		char* data = &ptr->chData[idx+infolen+wordlen];

		//if (info==NULL || info->handle==NULL)
		//{
		//	sprintf_s(buf, "接收檔案[ERR][錯誤]");
		//	LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
		//	idx += (infolen + wordlen + datalen);
		//	continue;
		//}
		CString strKey;
		strKey.Format("%s.%d", ptr->chRemoteIP, info->handle);

		switch ((DWORD)info->attribute)
		{
		case PACKET_WRITE:
			{
				CLog* log = NULL;
				if (!master->m_mapHandleToLogFile.Lookup(strKey, (void*&)log))
				{
					sprintf_s(buf, "接收檔案[ERR][%s][找不到檔案:%d]", ptr->chRemoteIP, info->handle);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);

					idx += (infolen + wordlen + datalen);
					continue;
				}
					
				if (log == NULL)
				{
					sprintf_s(buf, "接收檔案[ERR][%s][檔案已刪除:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					break;
				}

				if (log->info.handle == NULL)
				{
					sprintf_s(buf, "接收檔案[ERR][%s][檔案錯誤:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					break;
				}

				if (log->info.attribute == TRUE)
				{
					sprintf_s(buf, "接收檔案[ERR][%s][檔案唯讀:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
							
					idx += (infolen + wordlen + datalen);
					continue;
				}

				DWORD ret = 0;
				if (datalen > 0)
				{
					if (!WriteFile(log->info.handle, data, datalen, &ret, &log->overlapped))
					{
						if (GetLastError() == ERROR_IO_PENDING)
						{
							DWORD dw = WaitForSingleObject(log->overlapped.hEvent, INFINITE);

							if (dw == WAIT_OBJECT_0)
							{
								if (GetOverlappedResult(log->info.handle, &log->overlapped, &ret, TRUE) != 0)
								{
									if (ret <= 0)
									{
										sprintf_s(buf, "寫入檔案[ERR][%s][寫入長度為0:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
										LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
									}
								}
							}
							else
							{
								sprintf_s(buf, "寫入檔案[ERR][%s][?????:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
								LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
							}
						}
						else
						{
							sprintf_s(buf, "寫入檔案[ERR][%s][CODE(%d):%d:%s]", ptr->chRemoteIP, GetLastError(), info->handle, log->chFilename);
							LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
									
							bDisconnect = TRUE;

							idx += (infolen + wordlen + datalen);
							continue;
						}
					}

					ResetEvent(log->overlapped.hEvent);
				}
				else 
				{
					//if (log->overlapped.Offset != info->size)
					//{
					//	sprintf_s(buf, "接收檔案[ERR][檔案不同步:%s][%I64d:%I64d]", log->chFilename, log->overlapped.Offset, info->size);
					//	LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					//	
					//	idx += (infolen + wordlen + datalen);
					//	continue;
					//}
				}

				AcquireSRWLockExclusive(&log->lock);
					
				//[2018/02/09] 每層0-40秒延遲,造成四~五層架構中,HeartBeat延遲過高,發生警示 => 改為收到資料(或HeartBeat),即刻押上當前時間,收到ReadOnly仍維持押上Server檔案時間(與Server端同步)
				//mtk->m_info.m_lastwrite = info->m_lastwrite;
				SYSTEMTIME stime;
				FILETIME ftime;
				GetSystemTime(&stime);
				SystemTimeToFileTime(&stime, &ftime);
					
				//if (CompareFileTime(&(log->info.lastwrite), &(info->lastwrite))== -1)
				//{
					SetFileTime(log->info.handle, NULL, NULL, &ftime);
					log->info.lastwrite = ftime;
				//}
				log->info.size += ret;

				// FOR UI
				//DWORD dwtick = GetTickCount();
				//if ((dwtick-log->lasttime) >= 1000) 
				//{
				//	mtk->m_flow = ret;
				//	mtk->m_lasttime = dwtick;
				//}
				//else 
				//{
				//	mtk->m_flow += ret;
				//}

				//if (mtk->m_flow > mtk->m_maxflow) 
				//	mtk->m_maxflow = mtk->m_flow;

				ReleaseSRWLockExclusive(&log->lock);

				log->overlapped.Offset += ret;
				
				break;
			}
		case PACKET_READONLY:
			{
				CLog* log = NULL;
				if (!master->m_mapHandleToLogFile.Lookup(strKey, (void*&)log))
				{
					sprintf_s(buf, "接收檔案R[ERR][%s][找不到檔案:%d]", ptr->chRemoteIP, info->handle);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);

					idx += (infolen + wordlen + datalen);
					continue;
				}
					
				if (log == NULL)
				{
					sprintf_s(buf, "接收檔案R[ERR][%s][檔案已刪除:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					break;
				}

				if (log->info.handle == NULL)
				{
					sprintf_s(buf, "接收檔案R[ERR][%s][檔案錯誤:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					break;
				}

				if (log->info.attribute == TRUE)
				{
					sprintf_s(buf, "接收檔案R[ERR][%s][檔案唯讀:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);

					idx += (infolen + wordlen + datalen);
					continue;
				}

				DWORD ret = 0;
				if (datalen > 0)	
				{
					if (!WriteFile(log->info.handle, data, datalen, &ret, &log->overlapped))
					{
						if (GetLastError() == ERROR_IO_PENDING)
						{
							DWORD dw = WaitForSingleObject(log->overlapped.hEvent, INFINITE);

							if (dw == WAIT_OBJECT_0)
							{
								if (GetOverlappedResult(log->info.handle, &log->overlapped, &ret, TRUE) != 0)
								{
									if (ret <= 0)
									{
										sprintf_s(buf, "寫入檔案R[ERR][%s][寫入長度為0:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
										LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
									}
								}
							}
							else
							{
								sprintf_s(buf, "寫入檔案R[ERR][%s][?????:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
								LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
							}
						}
						else
						{
							sprintf_s(buf, "寫入檔案R[ERR][%s][CODE(%d):%d:%s]", ptr->chRemoteIP, GetLastError(), info->handle, log->chFilename);
							LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
									
							bDisconnect = TRUE;

							idx += (infolen + wordlen + datalen);
							continue;
						}
					}

					ResetEvent(log->overlapped.hEvent);
				}
				else
				{
					//if (log->overlapped.Offset != info->size)
					//{
					//	sprintf_s(buf, "接收檔案R[ERR][檔案不同步:%s][%I64d:%I64d]", log->chFilename, log->overlapped.Offset, info->size);
					//	LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					//	
					//	idx += (infolen + wordlen + datalen);
					//	continue;
					//}
				}

				SetFileTime(log->info.handle, NULL, NULL, &info->lastwrite);

				AcquireSRWLockExclusive(&log->lock);
				
				log->info.lastwrite = info->lastwrite;
				log->info.size += ret;
				//log->ended = TRUE;
				//log->dwLastwrite = tick;
				//mtk->m_info.m_attribute = TRUE;

				// FOR UI
				//DWORD dwtick = GetTickCount();
				//if ((dwtick-mtk->m_lasttime) >= 1000) 
				//{
				//	mtk->m_flow = ret;
				//	mtk->m_lasttime = dwtick;
				//}
				//else 
				//{
				//	mtk->m_flow += ret;
				//}

				//if (mtk->m_flow > mtk->m_maxflow) 
				//	mtk->m_maxflow = mtk->m_flow;

				ReleaseSRWLockExclusive(&log->lock);

				log->overlapped.Offset += ret;
				
				//if (log->overlapped.Offset == info->size) // 接收完畢
				//{
				//CString strFile("");
				//strFile.Format("%s%s", log->chFilepath, log->chFilename);
				//SetFileAttributes(strFile, FILE_ATTRIBUTE_READONLY);							
				//master->m_mapHandleToLogFile.RemoveKey((UINT&)info->handle);
				//			
				//AcquireSRWLockExclusive(&log->lock);
				//CloseHandle(log->info.handle);
				//log->info.handle = NULL;
				//log->info.attribute = TRUE;
				////log->master = NULL;
				//ReleaseSRWLockExclusive(&log->lock);

				//sprintf_s(buf, "接收檔案R[END][檔案結束:%s]", log->chFilename);
				//LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
				//}
				break;
			}
		case PACKET_NEWFILE: // 新的檔案
			{
				char chFilename[256]={0};
				memcpy(chFilename, data, datalen); 
	
				char chMtkVer[9] = {0};
				char* chMtkName = NULL;			
				memcpy(chMtkVer, chFilename, 8);
				chMtkName = chFilename+9;

				AcquireSRWLockExclusive(&master->m_lock);
				CLog* log = NULL;

				CString strnameKey;
				strnameKey.Format("%s.%s", ptr->chRemoteIP, chFilename);
				if (!master->m_mapNameToLogFile.Lookup(strnameKey, (void*&)log))
				{
					// 全新的檔案, 所以要新增一個
					log = new CLog();
					memset(log, NULL, sizeof(CLog));
					memcpy(log->chFilename, data, datalen); 
					memcpy(log->chFilepath, "c:\\quote\\log\\", strlen("c:\\quote\\log\\")); 
					memcpy(log->chMtkname, chMtkName, strlen(chMtkName));
					log->uMtkver = atoi(chMtkVer);
					//log->master = this;
					//log->filter = file->fnCheckFileFilter(mtk->m_chFilename);
					//log->ended = FALSE;
					//log->age = file->m_uAge;
					InitializeSRWLock(&log->lock);

					//if (log->m_filter)
					//{
					//	delete mtk;
					//	mtk = NULL;
					//			
					//	// 取消訂閱
					//	REQUEST* req = new REQUEST;
					//	memset(req, NULL, sizeof(REQUEST));

					//	req->m_id = 0;
					//	req->m_handle = info->m_handle;
					//	req->m_offset = 0;

					//	m_vecSyncList.push_back(req);
					//	//m_vecSyncList.push_back((UINT)info->m_handle);
					//	idx += (infolen + wordlen + datalen);
					//	ReleaseSRWLockExclusive(&file->m_lockFileList);
					//	continue;
					//}

					//master->m_arrMtkFile.Add(mtk);
					master->m_mapNameToLogFile.SetAt(strnameKey, (void*&)log);
					//InterlockedIncrement(&log->uGrowAge);
				}
				else
				{
					// 已有紀錄的檔案
					AcquireSRWLockExclusive(&log->lock);						
					//BOOL bMaster = (log->master == NULL) ? FALSE : TRUE;
					//if (!bMaster)
					//	mtk->m_master = this;
					//		
					//BOOL bFilter = mtk->m_filter;
					//if (bFilter)
					//	mtk->m_master = NULL;
							
					BOOL bReadonly = log->info.attribute;
					ReleaseSRWLockExclusive(&log->lock);

					//if (bFilter)
					//{
					//	// 取消訂閱
					//	REQUEST* req = new REQUEST;
					//	memset(req, NULL, sizeof(REQUEST));

					//	req->m_id = 0;
					//	req->m_handle = info->m_handle;
					//	req->m_offset = 0;

					//	m_vecSyncList.push_back(req);
					//	//m_vecSyncList.push_back((UINT)info->m_handle);
					//	idx += (infolen + wordlen + datalen);
					//	ReleaseSRWLockExclusive(&file->m_lockFileList);
					//	continue;
					//}

					//if (bMaster && (m_NetServerID != mtk->m_master->m_NetServerID))
					//{
					//	strLog.Format("已有其他資料源提供![主機:%d][其他主機:%d][%s]", m_NetServerID, mtk->m_master->m_NetServerID, mtk->m_chFilename);
					//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);

					//	// 取消訂閱
					//	REQUEST* req = new REQUEST;
					//	memset(req, NULL, sizeof(REQUEST));

					//	req->m_id = 0;
					//	req->m_handle = info->m_handle;
					//	req->m_offset = 0;

					//	m_vecSyncList.push_back(req);
					//	//m_vecSyncList.push_back((UINT)info->m_handle);
					//	idx += (infolen + wordlen + datalen);
					//	ReleaseSRWLockExclusive(&file->m_lockFileList);
					//	continue;
					//}

					if (bReadonly)
					{
						// 取消訂閱
						//REQUEST* req = new REQUEST;
						//memset(req, NULL, sizeof(REQUEST));

						//req->m_id = 0;
						//req->m_handle = info->m_handle;
						//req->m_offset = 0;

						//m_vecSyncList.push_back(req);
						//m_vecSyncList.push_back((UINT)info->m_handle);
						idx += (infolen + wordlen + datalen);
						ReleaseSRWLockExclusive(&master->m_lock);
						continue;
					}
				}
				ReleaseSRWLockExclusive(&master->m_lock);

				log->overlapped.Offset = 0;
				log->overlapped.OffsetHigh = 0;
				if (log->overlapped.hEvent == NULL) // 只做一次
				{
					log->overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
					if (log->overlapped.hEvent == NULL)
					{
						log->info.handle = NULL;
						//log->master = NULL;

						//strLog.Format("可怕的事情!檔案OVERLAPPED.EVENT建立失敗![%s]", mtk->m_chFilename);
						//log->DFLogSend(strLog, DF_SEVERITY_HIGH);
							
						// 取消訂閱
						//REQUEST* req = new REQUEST;
						//memset(req, NULL, sizeof(REQUEST));

						//req->m_id = 0;
						//req->m_handle = info->m_handle;
						//req->m_offset = 0;

						//m_vecSyncList.push_back(req);
						//m_vecSyncList.push_back((UINT)info->m_handle);
						idx += (infolen + wordlen + datalen);
						continue;	
					}
				}
						
				CString strFilename("");
				strFilename.Format("%s%s.%s", log->chFilepath, ptr->chRemoteIP, log->chFilename);
				if (log->info.handle == NULL) // 只做一次
				{
					if ((log->info.handle = CreateFile(strFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
					{
						UINT err = GetLastError();
						if (err = ERROR_ACCESS_DENIED)
						{
							sprintf_s(buf, "開啟檔案[ERR][%s][%d:%s][CODE:%d]", ptr->chRemoteIP, info->handle, log->chFilename, err);
							LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
						}
						else
						{
							sprintf_s(buf, "開啟檔案[ERR][%s][%d:%s][CODE:%d]", ptr->chRemoteIP, info->handle, log->chFilename, err);
							LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
						}

						// 不管是不是唯讀,只要打不開,這個檔案就不訂了
						log->info.handle = NULL;
							
						//AcquireSRWLockExclusive(&log->lock);
						//log->master = NULL;
						//ReleaseSRWLockExclusive(&log->lock);

						// 取消訂閱
						//REQUEST* req = new REQUEST;
						//memset(req, NULL, sizeof(REQUEST));

						//req->m_id = 0;
						//req->m_handle = info->m_handle;
						//req->m_offset = 0;

						//m_vecSyncList.push_back(req);
						//m_vecSyncList.push_back((UINT)info->m_handle);
						idx += (infolen + wordlen + datalen);
						continue;
					}
				}

				// 通過這裡的檔案就一定要收

				if (log->info.handle) // 若檔案原本就存在,取得檔案資訊,若是新的也可以順便初始化:)
				{
					// 檔案資訊結構
					BY_HANDLE_FILE_INFORMATION info;
					memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
					if (GetFileInformationByHandle(log->info.handle, &info) == 0)
					{
						//strLog.Format("GetFileInformationByHandle失敗![%s]", mtk->m_chFilename);
						//log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
						idx += (infolen + wordlen + datalen);
						continue;
					}
		
					AcquireSRWLockExclusive(&log->lock);
						
					// 檔案屬性(唯讀?)
					log->info.attribute = (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? TRUE : FALSE;

					// 檔案最後寫入時間
					log->info.lastwrite = info.ftLastWriteTime;

					// 檔案大小
					LARGE_INTEGER size;
					size.QuadPart = 0;
					size.HighPart = info.nFileSizeHigh;
					size.LowPart = info.nFileSizeLow;
					log->info.size = size.QuadPart;
					memcpy(&log->overlapped.Offset, &log->info.size, sizeof(UINT64));

					ReleaseSRWLockExclusive(&log->lock);

					// ***必須更新OVERLAPPED,才能續寫
					memcpy(&log->overlapped.Offset, &size.QuadPart, sizeof(LARGE_INTEGER));
				}

				// 加入工作列
				master->m_mapHandleToLogFile.SetAt(strKey, log);		
				//strLog.Format("新的檔案![主機:%d][%s]", m_NetServerID, strFilename);
				//log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
				
				break;
			}
		case PACKET_DELETEFILE:
			{
				CLog* log = NULL;
				if (!master->m_mapHandleToLogFile.Lookup(strKey, (void*&)log))
				{
					sprintf_s(buf, "接收檔案E[ERR][%s][找不到檔案:%d]", ptr->chRemoteIP, info->handle);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);

					idx += (infolen + wordlen + datalen);
					continue;
				}
					
				if (log == NULL)
				{
					sprintf_s(buf, "接收檔案E[ERR][%s][檔案已刪除:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					break;
				}

				if (log->info.handle == NULL)
				{
					sprintf_s(buf, "接收檔案E[ERR][%s][檔案錯誤:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
					break;
				}

				if (log->info.attribute == TRUE)
				{
					sprintf_s(buf, "接收檔案E[ERR][%s][檔案唯讀:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
					LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);

					idx += (infolen + wordlen + datalen);
					continue;
				}

				CString strFile("");
				strFile.Format("%s%s.%s", log->chFilepath, ptr->chRemoteIP, log->chFilename);
				SetFileAttributes(strFile, FILE_ATTRIBUTE_READONLY);							
				master->m_mapHandleToLogFile.RemoveKey(strKey);
							
				AcquireSRWLockExclusive(&log->lock);
				CloseHandle(log->info.handle);
				log->info.handle = NULL;
				log->info.attribute = TRUE;
				
				SetFileTime(log->info.handle, NULL, NULL, &info->lastwrite);
				log->info.lastwrite = info->lastwrite;
				log->info.size = info->size;
				ReleaseSRWLockExclusive(&log->lock);

				sprintf_s(buf, "接收檔案R[END][%s][檔案結束:%d:%s]", ptr->chRemoteIP, info->handle, log->chFilename);
				LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
				break;
			}
		}

		idx += (infolen + wordlen + datalen);
	}

	// 產生下一個WSARecv
	DWORD dwFlags = 0;
	ptr->uDatalen = 0;
	WSABUF wsaBuf = {4, (char*)&ptr->uDatalen};
	if (WSARecv(ptr->base.socket, &wsaBuf, 1, NULL, &dwFlags, &ptr->overlapped, NULL) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			sprintf_s(buf, "接收檔案[ERR][%s][連線錯誤:%d]", ptr->chRemoteIP, err);
			LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
			return 1;
		}
	}

	return 0;
}




