
// LogNet.cpp : �����\�����O��@�C
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
, m_NetSendVolumn(0)				// �C��ǰe�q
, m_NetSendMaxVolumn(0)				// �C��̤j�ǰe�q
, m_NetSendTotalVolumn(0)			// �`�ǰe�q
, m_NetProcVolumn(0)				// �C��B�z�q(���Y�e)
, m_NetProcMaxVolumn(0)				// �C��̤j�B�z�q(���Y�e)
, m_NetProcTotalVolumn(0)			// �`�B�z�q(���Y�e)
, m_strStatus("��l�s�u")
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
		
		m_strStatus = "�s�u����";
		LogSend("�s�u����!���s�s�u...", ID_SYSLOG_SEVERITY_WARNING);
		Sleep(10000); // 10��
	}

	m_strStatus = "�s�u���\";

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
		sprintf_s(chLog, "WSACreateEvent����(Send)![%d]", WSAGetLastError());
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
		sprintf_s(chLog, "WSACreateEvent����(Recv)![%d]", WSAGetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
		closesocket(m_socket);

		EndThread();
		return;
	}

	WSAOVERLAPPED RecvOverlapped;
	memset(&RecvOverlapped, NULL, sizeof(WSAOVERLAPPED));
	RecvOverlapped.hEvent = RecvEvent;

	// �אּ�����X�e�X�Ҧ�
	BOOL bOptVal = TRUE;
	int	bOptLen = sizeof(BOOL);
	int err = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOptVal, bOptLen);
    if (err==SOCKET_ERROR) 
	{
		sprintf_s(chLog, "setsockopt���~![%d]", GetLastError());
		LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
        closesocket(m_socket);
		
		EndThread();
		return;
    } 

	// ������
	DWORD dwTimeout = GetTickCount();
	DWORD dwTime;
	while (!IsStop())
	{
		dwTime = GetTickCount();

		// Timeout
		//if ((dwTime-dwTimeout) > 100000) // 100��
		//{
		//	sprintf_s(chLog, "[�D��:%s]�S������^��!��[%u]", m_NetServerIP, dwTime-dwTimeout);
		//	LogSend(chLog, ID_SYSLOG_SEVERITY_INFORMATIONAL);
		//	break;
		//}
		
		UpdateLogList();
		
		// �ʥ]�M���b
		memset(m_packet, NULL, sizeof(m_packet));
		
		// ��l��
		UINT total = 0;
		UINT remain = 0;
		WORK* work = NULL;
		
		for (POSITION pos = m_listLogFile.GetHeadPosition(); pos != NULL; )
		{
			if (total+sizeof(LOGFILEINFO)+sizeof(DWORD) > MAX_PACKET_SIZE)
			{
				break; // ���o
			}

			remain = MAX_PACKET_SIZE-total/*�ثe�ʥ]�j�p*/-sizeof(LOGFILEINFO)/*�̤p���ʥ]*/-sizeof(DWORD)/*������*/;
			dwTime = GetTickCount();
			
			work = (WORK*)m_listLogFile.GetNext(pos);
			if (work == NULL)
			{
				LogSend("�_�Ǫ��Ʊ� NULL���ɮ�!", ID_SYSLOG_SEVERITY_ERROR);
				continue;
			}

			//if (work->m_file->m_info.m_attribute)
			//{
			//	sprintf_s(chLog, "�ɮפw����![%s]", work->m_file->m_chFilename);
			//	LogSend(chLog, ID_SYSLOG_SEVERITY_WARNING);

			//	work->m_done = TRUE;
			//	work->m_file->m_bDone = TRUE;
			//	continue;
			//}

			if (work->m_file->m_info.m_handle == NULL)
			{
				sprintf_s(chLog, "�ɮפw����![%s]", work->m_file->m_chFilename);
				LogSend(chLog, ID_SYSLOG_SEVERITY_WARNING);

				work->m_done = TRUE;
				work->m_file->m_bDone = TRUE;
				continue;
			}

			if (bReset || work->m_heartbeat == 0)
			{
				// ���s���ɮ�
				DWORD filenamelen = strlen(work->m_file->m_chFilename);
				if (remain < filenamelen) break; // ���o

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
				// ���٨S�ǧ������
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
									sprintf_s(chLog, "�_�Ǫ��Ʊ� �SŪ����![%d][%s]", retlen, work->m_file->m_chFilename);
									LogSend(chLog, ID_SYSLOG_SEVERITY_WARNING);
								}
							}
						}
						else
						{
							// ERROR
							sprintf_s(chLog, "�_�Ǫ�Ū�ɦ^�ǭ�![%d][%s]", dw, work->m_file->m_chFilename);
							LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
							continue;
						}
					}
					else
					{
						// ERROR
						sprintf_s(chLog, "�ɮ�Ū������![%d][%s]", GetLastError(), work->m_file->m_chFilename);
						LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
						continue;
					}
				}

				ResetEvent(/*work->m_overlapped.hEvent*/work->m_file->m_overlapped.hEvent);

				// CheckReadFile
				if (retlen <= 0)
				{
					sprintf_s(chLog, "Readfile���`!?![%s][%d]", work->m_file->m_chFilename, retlen);
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
				// �S����ƭn��
				BOOL bIsReadonly = work->m_file->m_info.m_attribute;
				
				// �ɮ׶��m�W�L10�� �Ϊ� �ɮ��ݩ�ReadOnly => �n�eHB
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

				// �ɮ��ݩ�ReadOnly �ӥB �ɮפw���ưe���� => �Ӱh���F
				if (bIsReadonly && (/*(UINT64)work->m_overlapped.Offset*/(UINT64)work->m_file->m_overlapped.Offset == work->m_file->m_info.m_size)) 
				{
					sprintf_s(chLog, "�ǿ駹��![�ɮ�:%s][�j�p:%d]", work->m_file->m_chFilename, work->m_file->m_info.m_size);
					LogSend(chLog, ID_SYSLOG_SEVERITY_INFORMATIONAL);
					
					work->m_done = TRUE;
				}
			}
		}

		if (bReset)
			bReset = FALSE;

		if (total <= 0)
		{
			// �S�s���ɮ� + �S��ƥi�H�� + �SHeartBeat�ݭn�� => �B�@100ms
			Sleep(100);
		}
		else
		{
			// ������F��ݭn�ǰe
			DWORD sendlen = 0;
			m_sendbuf[0] = 0;

			m_packet->m_pktlen = total;

			if ((m_packet->m_pktlen>256) && m_zstd && theApp.m_ptDict) // ���Y
			{
				UINT nOutLen = 0;
				if (!ZSTD_isError(nOutLen=(int)ZSTD_compress_usingCDict(m_zstd, (char*)m_sendbuf+sizeof(int), MAX_PACKET_SIZE, (const char*)m_packet, m_packet->m_pktlen+sizeof(int), theApp.m_ptDict)))
				{	
					// ���Y���\
					*(int*)m_sendbuf = nOutLen | ZSTD_COMPRESS;	// len(OR)���Y�X��

					SendDataBuf.len = nOutLen+4;
					SendDataBuf.buf = (char*)m_sendbuf;
				}
				else
				{
					// ���Y����
					sprintf_s(chLog, "�_�Ǫ��Ʊ� ���Y����![%s][%d/%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
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
									sprintf_s(chLog, "�_�Ǫ��Ʊ� �S�g�J���![%d]", sendlen);
									LogSend(chLog, ID_SYSLOG_SEVERITY_DEBUG);
								}
							}
						}
					}
					else
					{
						sprintf_s(chLog, "WSASend����![%d]", WSAGetLastError());
						LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
						break;
					}
				}

				WSAResetEvent(SendOverlapped.hEvent);

				if (SendDataBuf.len != sendlen)
				{
					sprintf_s(chLog, "�_�Ǫ��Ʊ� �e�X���j�p���@��![com][%d][%d]", nOutLen+4, sendlen);
					LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
				}
			}
			else // �����Y
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
									sprintf_s(chLog, "�_�Ǫ��Ʊ� �S�g�J���![%d]", sendlen);
									LogSend(chLog, ID_SYSLOG_SEVERITY_DEBUG);
								}
							}
					}
					else
					{
						sprintf_s(chLog, "WSASend����![%d]", WSAGetLastError());
						LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
						break;
					}
				}

				WSAResetEvent(SendOverlapped.hEvent);

				//TRACE("[%I64d]%d\n", tra++, sendlen);

				if (SendDataBuf.len != sendlen)
				{
					sprintf_s(chLog, "�_�Ǫ��Ʊ� �e�X���j�p���@��![%d][%d]", total+4, sendlen);
					LogSend(chLog, ID_SYSLOG_SEVERITY_ERROR);
				}
			}

			// Check WSASend
			if (sendlen == 0)
			{
				LogSend("����ƭn�ǰe�o�e�X0Byte!(�_�u!?)", ID_SYSLOG_SEVERITY_DEBUG);
			}

			// ��s�s�u��T(For UI)
			AcquireSRWLockExclusive(&m_NetServerStatusLock);

			// �C���m�y�q�p��
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
		LogSend("�۰ʭ��s�s�u", ID_SYSLOG_SEVERITY_WARNING);
		closesocket(m_socket);
		m_strStatus = "���s�s�u";
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



