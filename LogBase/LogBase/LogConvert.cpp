
// LogConvert.cpp
//

#include "stdafx.h"
#include "LogConvert.h"
#include "LogBase.h"
#include "LogBaseDlg.h"
#include <time.h>
#include <curl/curl.h>
#include "base64.h"
#include "syslog\syslogclient.h"

CLogConvert::CLogConvert(AFX_THREADPROC pfnThreadProc, LPVOID param)
: CMyThread(pfnThreadProc, param)
, m_hWait(NULL)
, m_uIndex(0)
{
	
}

CLogConvert::~CLogConvert(void)
{

}

/*virtual*/ void CLogConvert::Go(void)
{
	CLogBaseDlg* parent = (CLogBaseDlg*)GetParentsHandle();
	LogSend("LogConvert GO", ID_SYSLOG_SEVERITY_INFORMATIONAL);
	
	m_hWait = CreateEvent(NULL, TRUE, FALSE, _T("logbase_logconvert_event"));

	char buf[1024]={0};
//	CURL* curl;
//	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
//	char chPostUrl[256] = {0};
//	char chPostBody[1024] = {0};
//	char chPostHeader[] = {"Content-Type: application/json"};

//	struct curl_slist *pPostList = NULL;
//	pPostList = curl_slist_append(pPostList, chPostHeader);

//	ZSTD_DCtx* zstd;
//	m_zstd = ZSTD_createDCtx();

//	ZSTD_DDict*	ptDict;
	CString strDict("zstd170125.dic");
	UINT uDictSize = 0;
	HANDLE hDict = NULL;
	if ((hDict = CreateFile(strDict, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	{
		// Log
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
			// Log
			m_ptDict = NULL;
		}
		else
		{
			if ((m_ptDict = ZSTD_createDDict(buf, ret)) == NULL)
			{
				// Log
				m_ptDict = NULL;
				uDictSize = 0;
			}
			else
				uDictSize = ret;
		}

		delete buf;
	}

	if (m_ptDict == NULL)	
	{
		EndThread();
		return;
	}

	WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{	
		EndThread();
		return;
	}

	DWORD dw = 0;
	while (!IsStop())
	{
		dw = WaitForSingleObject(m_hWait, INFINITE);
		if (dw == WAIT_OBJECT_0)
		{
			if (m_uIndex == parent->m_uLogtail)
			{
				ResetEvent(m_hWait);
				continue;
			}
		}
		else
		{
			break;
		}

		ResetEvent(m_hWait);

		DWORD dwTick = GetTickCount();

		m_uIndex = parent->m_uLoghead;
		for (UINT i=m_uIndex; i!=parent->m_uLogtail; i=(i+1)%100)
		{				
			if (parent->m_arrLog[i].bProc)
				continue;

			parent->m_arrLog[i].bProc = TRUE;

			if (strstr(parent->m_arrLog[i].chFilename, "taco") > 0)
			{
				sprintf_s(buf, "Alert=>新檔案[%s]", parent->m_arrLog[i].chFilename);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				
				CFileInfo* info = new(CFileInfo);
				info->master = this;
				info->filemaster = parent;
				info->fileindex = i;
				QueueUserWorkItem(&CLogConvert::m_fnFileProc, info, WT_EXECUTELONGFUNCTION);
				continue;
			}

			if (strstr(parent->m_arrLog[i].chFilename, "pushgateway") > 0)
			{
				sprintf_s(buf, "Alert=>新檔案[%s]", parent->m_arrLog[i].chFilename);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);

				CFileInfo* info = new(CFileInfo);
				info->master = this;
				info->filemaster = parent;
				info->fileindex = i;
				QueueUserWorkItem(&CLogConvert::m_fnFileProcAG, info, WT_EXECUTELONGFUNCTION);
				continue;
			}

			sprintf_s(buf, "Alert=>新檔案=>檔案名稱有誤[%s]", parent->m_arrLog[i].chFilename);
			LogSend(buf, ID_SYSLOG_SEVERITY_WARNING);
		}
	}

//	delete sendbuf;
//	delete readbuf;

	curl_global_cleanup();
	EndThread();
	return;
}

/*static */size_t CLogConvert::RecvFunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t realsize = size * nmemb;

	//(*(CString*)stream).Append(CString((char*)ptr));
	//strcat((char*)stream, (char*)ptr);
	CPacket* pkt = (CPacket*)stream;

	pkt->content = (char*)realloc(pkt->content, pkt->contentlen+realsize+1);
	if (pkt->content == NULL)
	{
		return 0;
	}

	memcpy(&(pkt->content[pkt->contentlen]), ptr, realsize);
	pkt->contentlen += realsize;
	pkt->content[pkt->contentlen] = 0;

	return realsize;
}

CString CLogConvert::ConvertString(char* src, int len)
{
	int i = 0;
	int idx = 0;
	char* str = new char[len*1.5]();

	char* f = strstr(src, "Last-Modified:");
	if (f)
	{
		int xxx= 0;
	}

	while (idx < len)
	{
		switch (src[idx])
		{
		default:
			str[i++] = src[idx];
			break;
		case '\"':
		case '\\':
		case '\/':
			str[i++] = '\\';
			str[i++] = src[idx];
			//str.AppendChar('\\');
			//str.AppendChar(src[idx]);
			//str.Insert(idx, '\\');
			//idx++;
			break;
		//case 0x0e:
		case 0x01:
			{
				int i=0;
			}
			break;
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
			str[i++] = ' ';
			//str.AppendChar(' ');
			//str.SetAt(idx, ' ');
			break;
		case 0xcc:
			{
				int i=0;
			}
			break;
		}

		idx++;
	}

	CString ret(str);
	delete[] str;

	return ret;
}

CString CLogConvert::ConvertUTF8(char *chAscii/*,char *chUTF8*/)
{
	char chUTF8[512] = {0};

	DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, chAscii, -1, NULL, 0);
	wchar_t *pwText;
	pwText = new wchar_t[dwNum];
	MultiByteToWideChar(CP_ACP, 0, chAscii, -1, pwText, dwNum);
	dwNum = WideCharToMultiByte(CP_UTF8, 0, pwText, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, pwText, -1, chUTF8, dwNum, NULL, NULL);

	delete []pwText;
	return CString(chUTF8);
}

CString CLogConvert::MakeStockTerms(CString& strData)
{
	int head = 0, tail = 0;
	int index = 0;
	
	CString strSrc(strData);
	CString strRet;

	if (strSrc.Find("Push?") >= 0)
	{
		strRet = "\"api\":\"Push\",\"stock\":[";

		if ((strSrc.Find("cmd=r")<0) && (strSrc.Find("cmd=x")<0))
		{
			return CString("\"api\":\"Push\",\"stock\":[]");
		}

		if ((head = strSrc.Find("stks=")) >= 0)
		{
			index = head+5;
			head = index;
			while (1)
			{
				if (strSrc[index]==',')
				{
					strRet.Append("\"");
					strRet.Append(strSrc.Mid(head, index-head));
					strRet.Append("\"");

					if (index+1 > strSrc.GetLength())
						break;

					strRet.Append(",");
					index ++;
					head = index;
				}
				else if (strSrc[index]=='&' || strSrc[index]==';' || strSrc[index]==' ')
				{
					strRet.Append("\"");
					strRet.Append(strSrc.Mid(head, index-head));
					strRet.Append("\"");
					break;
				}
				else if (strSrc[index]=='/')
				{
					strSrc.SetAt(index, '_');
					
					index ++;
				}
				else
				{
					if ((index+1) > strSrc.GetLength())
					{
						if (index!=head)
						{
							strRet.Append("\"");
							strRet.Append(strSrc.Mid(head, index-head));
							strRet.Append("\"");
						}
						
						break;
					}

					index++;
				}	
			}
			
			strRet.Append("]");
			return strRet;
		}
		else
		{
			return CString("\"api\":\"Push\",\"stock\":[]");
		}
	}
	else if (strData.Find("AAtrend") >= 0)
	{
		strRet = "\"api\":\"AAtrend\",\"stock\":[";

		if ((head = strData.Find("stk=")) >= 0)
		{
			index = head+4;
			head = index;
			while (1)
			{
				if (strData[index]==',')
				{
					strRet.Append("\"");
					strRet.Append(strData.Mid(head, index-head));
					strRet.Append("\"");

					if (index+1 > strData.GetLength())
						break;

					strRet.Append(",");
					index ++;
					head = index;
				}
				else if (strData[index]=='&' || strData[index]==';' || strData[index]==' ')
				{
					strRet.Append("\"");
					strRet.Append(strData.Mid(head, index-head));
					strRet.Append("\"");
					break;
				}
				else
				{
					if ((index+1) > strData.GetLength())
					{
						if (index!=head)
						{
							strRet.Append("\"");
							strRet.Append(strData.Mid(head, index-head));
							strRet.Append("\"");
						}
						
						break;
					}

					index++;
				}
			}

			strRet.Append("]");
			return strRet;
		}
		else
		{
			return CString("\"api\":\"AAtrend\",\"stock\":[]");
		}
	}
	
	return CString("\"api\":\"\",\"stock\":[]");
	
	
	//if ((head = strData.Find("stks="))>=0)
	//	head = head+5;
	//else if ((head = strData.Find("stk="))>=0)
	//	head = head+4;
	//else if ((head = strData.Find("sid="))>=0)
	//	head = head+4;
	//else
	//	return CString("[]");

	//CString strRet = ("[");
	//if ((tail = strData.Find("&", head))>0 || (tail = strData.Find(" ", head))>0)
	//{
	//	strRet.Append(strData.Mid(head, tail-head));
	//	strRet.Append("]");
	//	return strRet;
	//}
	//else
	//{
	//	strRet.Append(strData.Right(head));
	//	strRet.Append("]");
	//	return strRet;
	//}
	//
	//return CString("[]");
}

DWORD/* static*/ WINAPI CLogConvert::m_fnFileProc(PVOID lpParameter)
{
	CFileInfo* param = (CFileInfo*)lpParameter;
	CLogBaseDlg* parent = (CLogBaseDlg*)param->filemaster;
	CLogConvert* sender = (CLogConvert*)param->master;
	UINT index = param->fileindex;
	
	int err = 0;
	char buf[1024] = {0};

	ZSTD_DCtx* zstd;
	zstd = ZSTD_createDCtx();

	CURL* curl;
	CURLcode res;
//	curl_global_init(CURL_GLOBAL_ALL);
	char chPostHeader[] = {"Content-Type: application/json"};
	struct curl_slist *pPostList = NULL;
	pPostList = curl_slist_append(pPostList, chPostHeader);

	// Req Output File
	char chFilename[256] = {0};
	sprintf_s(chFilename, "c:\\quote\\log\\%s", parent->m_arrLog[index].chFilename);

	OVERLAPPED ovlp = {0};

	HANDLE hFile = NULL;
	if ((hFile = CreateFile(chFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	{
		// 開檔失敗
		err = GetLastError();
		sprintf_s(buf, "開啟檔案[%d][開啟檔案錯誤]", err);
		LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
		return 0;
	}

	// 4MB Buffer For Bulk Insert
	UINT sendbuflen = 4*1024*1024;

	// 512KB Buffer For Recv (Extension Possible)
	UINT readbuflen = 512;
	char* readbuf = new char[readbuflen];
	memset(readbuf, NULL, readbuflen);

	// 
	//UINT64 uFileLen = 0;
	DWORD dwLen = 0;
	DWORD dwRetlen = 0;
	
	// 
	char chTime[32] = {0};
	char chLoginTime[32] = {0};
	char chLogoutTime[32] = {0};
	char chConnectTime[32] = {0};
	char chUid[20] = {0};
	char chDate[20] = {0};
	
	// 計時器
	DWORD dwTimer = GetTickCount();

	DWORD dwRetheaderLen = 0;
	DWORD dwRetdataLen = 0;
	BOOL bCompress = FALSE;
	BOOL bCommand = FALSE;
	
	UINT uSendLen = 0;
	UINT uSendCount = 0;

	CString strPostUrl;
	CString strPostUpdate;
	CString strPostBody;

	strPostUrl.Format("http://%s:%d/%s/taco/_bulk", theApp.m_chElasticIP, theApp.m_uElasticPORT, parent->m_arrLog[index].chFilename);
	strPostUpdate.Format("http://%s:%d/%s/_mapping/taco?update_all_types", theApp.m_chElasticIP, theApp.m_uElasticPORT, parent->m_arrLog[index].chFilename);

	CPacket *pkt = new CPacket();
	BOOL bUpdateAllType = FALSE;

	int yy = 0;
	int id = 0;
	CString strid;
	
	UINT uFileoffset = 0;
	BY_HANDLE_FILE_INFORMATION info;
	memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
	while (1)
	{
		if (GetFileInformationByHandle(hFile, &info) == 0)
		{
			// ERROR
			err = GetLastError();
			sprintf_s(buf, "處理檔案[%d][取得檔案資訊錯誤]", err);
			LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			break;
		}

		parent->m_arrLog[index].bReadonly = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;

		LARGE_INTEGER size;
		size.QuadPart = 0;
		size.HighPart = info.nFileSizeHigh;
		size.LowPart = info.nFileSizeLow;
		parent->m_arrLog[index].uFilesize = size.QuadPart;

		while (parent->m_arrLog[index].uFilesize > uFileoffset)
		{
			if (ReadFile(hFile, (char*)&dwLen, 4, &dwRetlen, 0) == 0)
			{
				// ERROR
				err = GetLastError();
				sprintf_s(buf, "處理檔案[%d][讀取檔案長度錯誤]", err);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				return 0;
			}
		
			if (dwRetlen != 4)
			{
				// ERROR
				err = GetLastError();
				sprintf_s(buf, "處理檔案[%d][讀取檔案長度異常]", err);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				return 0;
			}

			uFileoffset += 4;

			strid.Format("%07d", id++);

			if (strid == "0094452")
				TRACE("DEBUG");

			DWORD dwCompressType = ((dwLen & 0xC0000000) >> 30);
			if (dwCompressType)
			{
				bCompress = TRUE;
				dwLen &= 0x3FFFFFFF; // 真實長度
			}
			else
			{
				bCompress = FALSE;
			}

			if (dwLen >= (readbuflen-1))
			{
				readbuf = (char*)realloc(readbuf, dwLen+1);
				readbuflen = dwLen+1;
			}

			memset(readbuf, NULL, dwLen+1);

			dwRetlen = 0;
			if (ReadFile(hFile, readbuf, dwLen, &dwRetlen, 0))
			{
				UINT n = dwRetlen;
				while (n < dwLen)
				{
					ReadFile(hFile, readbuf+n, dwLen-n, &dwRetlen, 0);

					if (dwRetlen == 0)
						Sleep(1);

					n += dwRetlen;
				}

				if (dwLen != dwRetlen)
				{
					sprintf_s(buf, "處理檔案[%d][檔案異常]", GetLastError());
					LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
					return 0;
				}

				uFileoffset += dwLen;

				// ------------------------------------------
				// HEADER FORMAT
				// ------------------------------------------
				// UINT :	buf+0 : header length
				// BYTE :	buf+4 : type
				// UINT64 : buf+5 : time
				// WORD :	buf+13 : index
				// WORD :	buf+15 : version
				// CHAR[] : buf+17 : userid
				// ------------------------------------------
				// BODY FORMAT
				// ------------------------------------------
				// UINT :	data+0 : data length
				// WORD :	data+4 : data type
				// CHAR[] : data+6 : data
				// ------------------------------------------
				readbuf[dwLen] = 0;
				LogHeader* head = (LogHeader*)readbuf;

				switch (head->type)
				{
				// 
				default:
					{
						sprintf_s(buf, "處理檔案[%d][檔案內容有誤][type=%d]", GetLastError(), head->type);
						LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
					}
					break;
				// Request Log
				case 1: 
					{
						tm t;
						_localtime64_s(&t, &head->time);
						strftime(chTime, 256, "%Y/%m/%d %H:%M:%S", &t);
							
						int uidlen = head->len-13;
						CString sUid(head->userid, uidlen);
						sUid.Replace(':', '_');
						
						//char uid[100] = {0};
						//memcpy(uid, head->userid, uidlen);

						LogBody* body = (LogBody*)(readbuf+4+(head->len));

						BOOL bCommand = FALSE;
						if ((bCommand = ((body->route & 0x8000) > 0)) == TRUE)
						{
							body->route &= 0x7FFF;
						}

						CString str;
						CString strdata;
						if (bCommand)
						{
							switch(body->data[0])
							{
							case 0x01:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":1,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x02:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":2,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x03:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":3,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x04:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":4,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x05:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":5,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x06:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":6,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x07:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":7,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x08:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":8,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							case 0x09:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":9,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							default:
								{
									string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
									strdata.Format("\"route\":%d,\"cmd_type\":0,\"datab\":\"%s\"", body->route, base64Data.c_str());
								}
								break;
							}

							str.Format("{\"index\":{\"_id\":\"%s000\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"command\",%s}}\n",strid, id*1000, head->type, chTime, head->index, head->version, sUid, strdata);
						}
						else
						{
							if (body->len > 2)
							{
								switch(body->route)
								{
								default:
									{
										sprintf_s(buf, "處理資料[%d][資料內容有誤:%d]", GetLastError(), body->route);
										LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);								
									}
									break;
								case 1:
									{
										CString s((char*)body->data);
										s.Replace("\"", "\\\"");
										s.Replace("\r\n", " ");
										str.Format("{\"index\":{\"_id\":\"%s000\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"request\",\"route\":%d,\"data\":\"%s\"}}\n",strid, id*1000, head->type, chTime, head->index, head->version, sUid, body->route, s);
									}
									break;
								case 2: case 5:
								case 3: case 6:
								case 4:
									{
										CString s((char*)&body->data[4]);
										s.Replace("\"", "\\\"");
										s.Replace("\r\n", " ");

										if (s.Find("Push")>=0 || s.Find("sid")>=0 || s.Find("stk")>=0)
											str.Format("{\"index\":{\"_id\":\"%s000\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"request\",\"route\":%d,\"sno\":%d,\"data\":\"%s\",%s}}\n",strid, id*1000, head->type, chTime, head->index, head->version, sUid, body->route, *(UINT*)body->data, s, MakeStockTerms(s));	
										else
											str.Format("{\"index\":{\"_id\":\"%s000\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"request\",\"route\":%d,\"sno\":%d,\"data\":\"%s\"}}\n",strid, id*1000, head->type, chTime, head->index, head->version, sUid, body->route, *(UINT*)body->data, s);	
									}
									break;
								}
							}
							else
							{
								sprintf_s(buf, "處理資料[%d][資料長度太小:%d]", GetLastError(), body->len);
								LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);	
							}
						}

						strPostBody.Append(str);

						uSendCount ++;
						uSendLen += str.GetLength();
					}
					break;
				// Response Log
				case 2: 
					{
						tm t;
						_localtime64_s(&t, (__time64_t*)(void*)(readbuf+5));
						strftime(chTime, 256, "%Y/%m/%d %H:%M:%S", &t);

						int uidlen = head->len-13;
						CString sUid(head->userid, uidlen);
						sUid.Replace(':', '_');
						
						//char* uid = new char[uidlen+1];
						//memcpy(uid, head->userid, uidlen);
						//uid[uidlen] = 0;

						LogBody* body = (LogBody*)(readbuf+4+(head->len));

						UINT uComp = (body->len & 0xC0000000) >> 30;
						UINT nOutLen = 0;
						UINT udecomLen = 0;

						CString strdata;
						CString str;
						LogBody* debody = NULL;
						char* compbuf = NULL;
						UINT dwL = 0;
						switch (uComp)
						{
						case 0:
							{
								body->len &= ~0xC0000000;
								udecomLen = body->len;

								debody = (LogBody*)((char*)body+4);
							}
							break;
						case 1:
						case 2:
							break;
						case 3:
							{
								body->len &= ~0xC0000000;
								dwL = ZSTD_getDecompressedSize((char*)body+4, body->len);
								compbuf = new char[dwL+1]();

								if (!ZSTD_isError(nOutLen=(int)ZSTD_decompress_usingDDict(zstd, (char*)compbuf, dwL, (const char*)body+4, body->len, sender->m_ptDict)))
								{	
									// 壓縮成功
									udecomLen = nOutLen;	// len(OR)壓縮旗標
									compbuf[nOutLen] = 0;
									debody = (LogBody*)compbuf;
								}
								else
								{
									udecomLen = 0;	// len(OR)壓縮旗標
									body = NULL;
								
									// 壓縮失敗
									//strLog.Format("奇怪的事情 壓縮失敗![com][%s][%d][%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
									//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

									//memset(&DataBuf, NULL, sizeof(WSABUF));
									//DataBuf.len = 0;
									//DataBuf.buf = NULL;
									//DataBuf.len = m_packet->m_pktlen+4;
									//DataBuf.buf = (char*)m_packet;
								}
							}
							break;
						default:
							break;
						}

						int i=0;
						int idx=0;
						while (i<udecomLen)
						{
							body = (LogBody*)((char*)debody + i);

							BOOL bCommand = FALSE;
							if ((bCommand = ((body->route & 0x8000) > 0)) == TRUE)
							{
								body->route &= 0x7FFF;
							}
									
							if (bCommand)
							{
								switch(body->data[0])
								{
								case 0x01:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":1,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x02:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":2,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x03:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":3,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x04:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":4,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x05:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":5,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x06:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":6,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x07:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":7,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x08:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":8,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								case 0x09:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":9,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;
								default:
									{
										string base64Data = base64_encode((unsigned char*)body->data, body->len-2);
										strdata.Format("\"route\":%d,\"cmd_type\":0,\"datab\":\"%s\"", body->route, base64Data.c_str());
									}
									break;

								}

								str.Format("{\"index\":{\"_id\":\"%s%03d\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"command\",%s}}\n", strid, idx, id*1000+idx, head->type, chTime, head->index, head->version, sUid, strdata);
							}
							else
							{
								if (body->len > 2)
								{
									switch(body->route)
									{
									default:
										{
											sprintf_s(buf, "處理資料[%d][資料內容有誤:%d]", GetLastError(), body->route);
											LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);									
										}
										break;
									case 1:
										{
											//string base64Data = base64_encode((unsigned char*)&body->data, body->len-2);
											CString s((char*)body->data);
											s.Replace("\"", "\\\"");
											s.Replace("\r\n", " ");
											str.Format("{\"index\":{\"_id\":\"%s%03d\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"response\",\"route\":%d,\"data\":\"%s\"}}\n", strid, idx, id*1000+idx, head->type, chTime, head->index, head->version, sUid, body->route, s);
								
											// LOGIN
										}
										break;
									case 2: case 5:
										{
											//CString s((char*)&body->data[4]);
											//s.Replace("\"", "\\\"");
											//s.Replace("\r\n", " ");
											string base64Data = base64_encode((unsigned char*)&body->data[4], body->len-4);
											str.Format("{\"index\":{\"_id\":\"%s%03d\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"response\",\"route\":%d,\"sno\":%d,\"datab\":\"%s\"}}\n", strid, idx, id*1000+idx, head->type, chTime, head->index, head->version, sUid, body->route, *(UINT*)body->data, base64Data.c_str());
										}
										break;
									case 3: case 6:
									case 4:
										{
											//CString s((char*)body->data);
											//s.Replace("\"", "\\\"");
											//s.Replace("\r\n", " ");
											//string base64Data = base64_encode((unsigned char*)body->data, body->len);
											str.Format("{\"index\":{\"_id\":\"%s%03d\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"response\",\"route\":%d,\"data\":\"%s\"}}\n", strid, idx, id*1000+idx, head->type, chTime, head->index, head->version, sUid, body->route, ConvertString((char*)body->data, body->len-2));
										}
										break;
									}
								}
								else
								{
									sprintf_s(buf, "處理資料[%d][資料長度太小:%d]", GetLastError(), body->len);
									LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);	
								}
							}

							strPostBody.Append(str);

							uSendCount ++;
							uSendLen += str.GetLength();

							i = i + body->len + 4;
							idx ++;
						}
							

						//delete uid;

						if (compbuf != NULL)
							delete[] compbuf;
					}
					break;
				// Login/out
				case 3: 
					{
						tm t;
						_localtime64_s(&t, (__time64_t*)(void*)(readbuf+5));
						strftime(chLogoutTime, 256, "%Y/%m/%d %H:%M:%S", &t);

						int uidlen = head->len-13;
						CString sUid(head->userid, uidlen);
						sUid.Replace(':', '_');

						//char* uid = new char[uidlen+1];
						//memcpy(uid, head->userid, uidlen);
						//uid[uidlen] = 0;

						CLogBodyEx* body = (CLogBodyEx*)(readbuf+4+(head->len));

						UINT uComp = (body->len & 0xC0000000) >> 30;
						UINT nOutLen = 0;
						UINT udecomLen = 0;

						CString strdata;
						CString str;
						CLogBodyEx* debody = NULL;
						char* compbuf = NULL;
						UINT dwL = 0;
						switch (uComp)
						{
						case 0:
							{
								body->len &= ~0xC0000000;
								udecomLen = body->len;

								debody = (CLogBodyEx*)((char*)body);
							}
							break;
						case 1:
						case 2:
							break;
						case 3:
							{
								body->len &= ~0xC0000000;
								dwL = ZSTD_getDecompressedSize((char*)body+4, body->len);
								compbuf = new char[dwL+1]();

								if (!ZSTD_isError(nOutLen=(int)ZSTD_decompress_usingDDict(zstd, (char*)compbuf, dwL, (const char*)body+4, body->len, sender->m_ptDict)))
								{	
									// 壓縮成功
									udecomLen = nOutLen;	// len(OR)壓縮旗標
									compbuf[nOutLen] = 0;
									debody = (CLogBodyEx*)compbuf;
								}
								else
								{
									udecomLen = 0;	// len(OR)壓縮旗標
									body = NULL;
								
									// 壓縮失敗
									//strLog.Format("奇怪的事情 壓縮失敗![com][%s][%d][%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
									//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

									//memset(&DataBuf, NULL, sizeof(WSABUF));
									//DataBuf.len = 0;
									//DataBuf.buf = NULL;
									//DataBuf.len = m_packet->m_pktlen+4;
									//DataBuf.buf = (char*)m_packet;
								}
							}
							break;
						default:
							break;
						}

						int i=0;
						int idx=0;
						while (i<udecomLen)
						{
							body = (CLogBodyEx*)((char*)debody + i);

							tm st;
							_localtime64_s(&st, (__time64_t*)(&body->time));
							strftime(chLoginTime, 256, "%Y/%m/%d %H:%M:%S", &st);

							__time64_t dift = *(__time64_t*)(void*)(readbuf+5) - body->time;
							
							CString strRemoteIP(inet_ntoa(((struct sockaddr_in*)&body->addr)->sin_addr));
							UINT uRemotePORT = ((struct sockaddr_in*)&body->addr)->sin_port;
								
							strdata.Format("\"address\":\"%s:%d\",\"login\":\"%s\",\"logout\":\"%s\",\"duration\":%d,\"permission\":\"%I64d:%I64d:%I64d\"", strRemoteIP, uRemotePORT, chLoginTime, chLogoutTime, (UINT)dift, body->mask[0], body->mask[1], body->mask[2]);
							str.Format("{\"index\":{\"_id\":\"%s%03d\"}}\n{\"idx\":%d,\"type\":\"%d\",\"time\":\"%s\",\"index\":%d,\"ver\":%d,\"uid\":\"%s\",\"body\":{\"data_type\":\"login\",%s}}\n", strid, idx, id*1000+idx, head->type, chTime, head->index, head->version, sUid, strdata);

							strPostBody.Append(str);

							uSendCount ++;
							uSendLen += str.GetLength();

							i = i + body->len + 4;
							idx ++;
						}
	
						//delete uid;

						if (compbuf != NULL)
							delete[] compbuf;

					}
					break;
				}
			}
			else
			{
				sprintf_s(buf, "處理檔案[%d][讀取檔案內容錯誤]", GetLastError());
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				break;
			}

			if (uSendLen > sendbuflen || uSendCount >= 500 || (((GetTickCount()-dwTimer)>=1000) && uSendCount>0))
			{
				parent->m_arrLog[index].uFileDataCount += uSendCount;
				pkt->content = new char[1];
				pkt->contentlen = 0;
				
				sprintf_s(buf, "資料[%s][%d筆]=>ElasticSearch", parent->m_arrLog[index].chFilename, uSendCount);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				
				curl = curl_easy_init();

				curl_easy_setopt(curl, CURLOPT_URL, strPostUrl);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogConvert::RecvFunc); 
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
				if ((res = curl_easy_perform(curl)) != CURLE_OK)
				{
					err = GetLastError();
					// ERROR
					sprintf_s(buf, "ElasticSearch=>連線錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
					LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				}
				curl_easy_cleanup(curl);
			
				char* f = strstr(pkt->content, "\"errors\":false");
				if (f == NULL)
				{
					f = pkt->content;
					while (f = strstr(f+7, "\"error\":"))
					{
						err = GetLastError();
					}
					parent->m_arrLog[index].uFileErrorCount += uSendCount;
					sprintf_s(buf, "ElasticSearch=>資料錯誤[%s][%d][%d筆][%s]", parent->m_arrLog[index].chFilename, res, uSendCount, strPostBody.Left(100));
					LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
				}

				delete[] pkt->content;		
				strPostBody.Empty();

				uSendLen = 0;
				uSendCount = 0;

				dwTimer = GetTickCount();
				if (!bUpdateAllType)
				{
					pkt->content = new char[1];
					pkt->contentlen = 0;

					strPostBody.Format("{\"properties\":{\"uid\":{\"type\":\"text\",\"fielddata\":true},\"body.stock\":{\"type\":\"text\",\"fielddata\":true},\"body.api\":{\"type\":\"text\",\"fielddata\":true}}}");

					curl = curl_easy_init();
					curl_easy_setopt(curl, CURLOPT_URL, strPostUpdate);
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogConvert::RecvFunc); 
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
					if ((res = curl_easy_perform(curl)) != CURLE_OK)
					{
						err = GetLastError();
						// ERROR
						sprintf_s(buf, "ElasticSearch=>連線錯誤[%s][%d][屬性]", parent->m_arrLog[index].chFilename, res);
						LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
					}
					curl_easy_cleanup(curl);

					char* f = strstr(pkt->content, "\"acknowledged\":true");
					if (f == NULL)
					{
						f = pkt->content;
						while (f = strstr(f+7, "\"error\":"))
						{
							err = GetLastError();
						}
						parent->m_arrLog[index].uFileErrorCount += uSendCount;
						sprintf_s(buf, "ElasticSearch=>資料錯誤[%s][%d][屬性][%s]", parent->m_arrLog[index].chFilename, res, strPostBody.Left(100));
						LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
					}
					else
					{
						sprintf_s(buf, "ElasticSearch=>資料屬性設定[%s][OK]", parent->m_arrLog[index].chFilename);
						LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
						bUpdateAllType = TRUE;
					}

					delete[] pkt->content;
					strPostBody.Empty();
				}
			}
		}
			
		if (uSendLen > sendbuflen || uSendCount >= 500 || (((GetTickCount()-dwTimer)>=1000) && uSendCount>0))
		{
			parent->m_arrLog[index].uFileDataCount += uSendCount;
	//		sprintf_s(buf, "資料[%s][%d筆]=>ElasticSearch", parent->m_arrLog[index].chFilename, uSendCount);
	//		LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			pkt->content = new char[1];
			pkt->contentlen = 0;

			curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_URL, strPostUrl);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogConvert::RecvFunc); 
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
			if ((res = curl_easy_perform(curl)) != CURLE_OK)
			{
				err = GetLastError();
				sprintf_s(buf, "ElasticSearch=>連線錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			}
			curl_easy_cleanup(curl);

			if (strstr(pkt->content, "\"errors\":false") == NULL)
			{
				while (strstr(pkt->content, "\"failed\":1"))
				{
					err = GetLastError();
				}
				parent->m_arrLog[index].uFileErrorCount += uSendCount;
				sprintf_s(buf, "ElasticSearch=>資料錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			}

			delete[] pkt->content;
			strPostBody.Empty();

			uSendLen = 0;
			uSendCount = 0;

			dwTimer = GetTickCount();
		}

		// Readonly=>退場
		if (parent->m_arrLog[index].bReadonly)
			break;

	}

	delete pkt;
	delete readbuf;

	DWORD qos = GetTickCount() - dwTimer;
	CloseHandle(hFile);

	sprintf_s(buf, "處理檔案[OK][%s]", parent->m_arrLog[index].chFilename);
	LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);

	return 0;
}

DWORD/* static*/ WINAPI CLogConvert::m_fnFileProcAG(PVOID lpParameter)
{
	CFileInfo* param = (CFileInfo*)lpParameter;
	CLogBaseDlg* parent = (CLogBaseDlg*)param->filemaster;
	CLogConvert* sender = (CLogConvert*)param->master;
	UINT index = param->fileindex;
	
	int err = 0;
	char buf[1024] = {0};

	ZSTD_DCtx* zstd;
	zstd = ZSTD_createDCtx();

	CURL* curl;
	CURLcode res;
//	curl_global_init(CURL_GLOBAL_ALL);
	char chPostHeader[] = {"Content-Type: application/json"};
	struct curl_slist *pPostList = NULL;
	pPostList = curl_slist_append(pPostList, chPostHeader);

	// Req Output File
	char chFilename[256] = {0};
	sprintf_s(chFilename, "c:\\quote\\log\\%s", parent->m_arrLog[index].chFilename);
	HANDLE hFile = NULL;
	if ((hFile = CreateFile(chFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	{
		// 開檔失敗
		return GetLastError();;
	}

	// 4MB Buffer For Bulk Insert
	UINT sendbuflen = 4*1024*1024;

	// 512B Buffer Foe Recv
	UINT readbuflen = sizeof(CLogApnsGateway);
	char* readbuf = new char[readbuflen+1];
	memset(readbuf, NULL, readbuflen+1);

	// 
	UINT64 uFileLen = 0;
	
	// 
	char chTime[32] = {0};
	char chUid[20] = {0};
	char chDate[20] = {0};
		
	// 計時器
	DWORD dwTimer = GetTickCount();

	DWORD dwRetheaderLen = 0;
	DWORD dwRetdataLen = 0;
	BOOL bCompress = FALSE;
	BOOL bCommand = FALSE;
			
	UINT uSendLen = 0;
	UINT uSendCount = 0;

	CString strPostUrl;
	strPostUrl.Format("http://%s:%d/%s/pushgateway/_bulk", theApp.m_chElasticIP, theApp.m_uElasticPORT, parent->m_arrLog[index].chFilename);

	CString strPostUpdate;
	strPostUpdate.Format("http://%s:%d/%s", theApp.m_chElasticIP, theApp.m_uElasticPORT, parent->m_arrLog[index].chFilename);

	CString strPostBody;
	strPostBody.Format("{\"mappings\":{\"pushgateway\":{\"properties\":{\"pushid\":{\"type\":\"text\",\"fielddata\":true}}}}}");

	CPacket *pkt = new CPacket();
	pkt->content = new char[1];
	pkt->contentlen = 0;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, strPostUpdate);
	curl_easy_setopt(curl, CURLOPT_PUT, strPostBody);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogConvert::RecvFunc); 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
	if ((res = curl_easy_perform(curl)) != CURLE_OK)
	{
		err = GetLastError();
		// ERROR
		sprintf_s(buf, "ElasticSearch=>連線錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
		LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
	}
	curl_easy_cleanup(curl);
	delete[] pkt->content;

	int yy = 0;
	int id = 0;
	CString strid;
	
	DWORD dwRetlen = 0;
	while (1)
	{
		memset(readbuf, NULL, readbuflen);		
		dwRetlen = 0;
		if (ReadFile(hFile, readbuf, readbuflen, &dwRetlen, 0))
		{
			UINT n = dwRetlen;
			while (n < readbuflen)
			{
				ReadFile(hFile, readbuf+n, readbuflen-n, &dwRetlen, 0);

				if (dwRetlen == 0)
					Sleep(1);

				n += dwRetlen;
			}

			if (readbuflen != n)
			{
				sprintf_s(buf, "處理檔案[%d][檔案錯誤]", GetLastError());
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			}

			CLogApnsGateway* agRet = (CLogApnsGateway*)readbuf; 
			strid.Format("%07d", id++);
			readbuf[readbuflen] = 0;

			// 轉換時間格式str=>datetime
			CString strTime(agRet->ctime);
			strTime.Insert(4, '/');
			strTime.Insert(7, '/');
			strTime.Insert(10, ' ');
			strTime.Insert(13, ':');
			strTime.Insert(16, ':');
			
			CString str;
			str.Format("{\"index\":{\"_id\":\"%s\"}}\n{\"idx\":%d,\"msgid\":\"%s\",\"appkey\":\"%s\",\"appsword\":\"%s\",\"catid\":\"%s\",\"pushid\":\"%s\",\"login\":\"%c\",\"msg\":\"%s\",\"stk\":\"%s\",\"counts\":\"%s\",\"time\":\"%s\",\"action\":\"%s\",\"delimiter\":%d}\n",strid, id, agRet->msgid, agRet->appkey, agRet->appsword, agRet->catid, agRet->pushid, agRet->login, ConvertUTF8(agRet->msg), agRet->stk, agRet->counts, strTime, agRet->action, agRet->delimiter);

			strPostBody.Append(str);
			uSendCount ++;
			uSendLen += str.GetLength();
		}
		else
		{
			sprintf_s(buf, "處理檔案[%d][讀檔錯誤]", GetLastError());
			LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			break;
		}

		if (uSendLen > sendbuflen || uSendCount >= 500 || (((GetTickCount()-dwTimer)>=1000) && uSendCount>0))
		{
		//if (strPostBody.GetLength() > 0)
		//{	
			parent->m_arrLog[index].uFileDataCount += uSendCount;
//			sprintf_s(buf, "資料[%s][%d筆]=>ElasticSearch", parent->m_arrLog[index].chFilename, uSendCount);
//			LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
//			CPacket *pkt = new CPacket();
//			pkt->content = new char[1];
//			pkt->contentlen = 0;
			pkt->content = new char[1];
			pkt->contentlen = 0;

			curl = curl_easy_init();
			curl_easy_setopt(curl, CURLOPT_URL, strPostUrl);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogConvert::RecvFunc); 
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
			if ((res = curl_easy_perform(curl)) != CURLE_OK)
			{
				err = GetLastError();
				// ERROR
				sprintf_s(buf, "ElasticSearch=>連線錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			}
			curl_easy_cleanup(curl);

			char* f = strstr(pkt->content, "\"errors\":false");
			if (f == NULL)
			{
				f = pkt->content;
				while (f = strstr(f+7, "\"error\":"))
				{
					err = GetLastError();
				}
				parent->m_arrLog[index].uFileErrorCount += uSendCount;
				sprintf_s(buf, "ElasticSearch=>資料錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
				LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			}
					
			delete[] pkt->content;
//			delete pkt;
			strPostBody.Empty();

			uSendLen = 0;
			uSendCount = 0;

			dwTimer = GetTickCount();
			//TRACE("%d\n", yy++);
		}
	}
			
	if (uSendCount > 0)
	{
		parent->m_arrLog[index].uFileDataCount += uSendCount;
//		sprintf_s(buf, "資料[%s][%d筆]=>ElasticSearch", parent->m_arrLog[index].chFilename, uSendCount);
//		LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
//		CPacket *pkt = new CPacket();
//		pkt->content = new char[1];
//		pkt->contentlen = 0;
		pkt->content = new char[1];
		pkt->contentlen = 0;

		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, strPostUrl);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogConvert::RecvFunc); 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
		if ((res = curl_easy_perform(curl)) != CURLE_OK)
		{
			err = GetLastError();
			sprintf_s(buf, "ElasticSearch=>連線錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
			LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
			// ERROR
		}
		curl_easy_cleanup(curl);

		if (strstr(pkt->content, "\"errors\":false") == NULL)
		{
			while (strstr(pkt->content, "\"failed\":1"))
			{
				err = GetLastError();
			}
			parent->m_arrLog[index].uFileErrorCount += uSendCount;
			sprintf_s(buf, "ElasticSearch=>資料錯誤[%s][%d][%d筆]", parent->m_arrLog[index].chFilename, res, uSendCount);
			LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);
		}

		delete[] pkt->content;
		strPostBody.Empty();

		uSendLen = 0;
		uSendCount = 0;

		dwTimer = GetTickCount();
	}
		
	delete pkt;
	DWORD qos = GetTickCount() - dwTimer;
	CloseHandle(hFile);

	sprintf_s(buf, "處理檔案[OK][%s]", parent->m_arrLog[index].chFilename);
	LogSend(buf, ID_SYSLOG_SEVERITY_INFORMATIONAL);

	return 0;
}

