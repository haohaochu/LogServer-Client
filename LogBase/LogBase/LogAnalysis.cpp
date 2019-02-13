
// LogAnalysis.cpp
//

#include "stdafx.h"
#include "LogAnalysis.h"
#include "LogBase.h"
#include "LogBaseDlg.h"
#include <time.h>
#include <curl/curl.h>
#include "base64.h"
#include "syslog\syslogclient.h"

CLogAnalysis::CLogAnalysis(AFX_THREADPROC pfnThreadProc, LPVOID param)
: CMyThread(pfnThreadProc, param)
, m_hStopTimerQueue(NULL)
, m_hTimerQueue(NULL)
{
}

CLogAnalysis::~CLogAnalysis(void)
{
}

/*virtual */void CLogAnalysis::Go(void)
{
	CLogBaseDlg* parent = (CLogBaseDlg*)GetParentsHandle();
	LogSend("CLogAnalysis GO", ID_SYSLOG_SEVERITY_INFORMATIONAL);

	char buf[1024]={0};

	curl_global_init(CURL_GLOBAL_ALL);
	
//	CURL* curl;
//	CURLcode res;

	DWORD dw = 0;
	m_hTimerQueue = CreateTimerQueue();
	m_hStopTimerQueue = CreateEvent(NULL, FALSE, FALSE, _T("StopTimerQueue"));
	while (!IsStop())
	{
		time_t t = time(NULL);
		struct tm tt;
		localtime_s(&tt, &t);
		
		// 電文使用人數:每小時hh:00:10開始,每10分鐘作一次 Ex.00:00:10=>00:10:10=>00:20:10=>...
		CreateTimerQueueTimer(&m_hTimer[0], m_hTimerQueue, (WAITORTIMERCALLBACK)m_fnAnalysisApiA, this, (((60-tt.tm_min-1)%10)*60000)+((60-tt.tm_sec+10)*1000), 600000, WT_EXECUTELONGFUNCTION);
		
		// 商品搜尋人數:每小時hh:00:10開始,每1小時作一次 Ex.00:00:10=>01:00:10=>02:00:10=>...
		CreateTimerQueueTimer(&m_hTimer[1], m_hTimerQueue, (WAITORTIMERCALLBACK)m_fnAnalysisApiB, this, ((60-tt.tm_min-1)*60000)+((60-tt.tm_sec+10)*1000), 3600000, WT_EXECUTELONGFUNCTION);

		// 連線行為:每小時hh:00:10開始,每1小時作一次 Ex.00:00:10=>01:00:10=>02:00:10=>...
		CreateTimerQueueTimer(&m_hTimer[2], m_hTimerQueue, (WAITORTIMERCALLBACK)m_fnAnalysisUserA, this, ((60-tt.tm_min-1)*60000)+((60-tt.tm_sec+10)*1000), 3600000, WT_EXECUTELONGFUNCTION);

		dw = WaitForSingleObject(m_hStopTimerQueue, INFINITE);
		if (dw == WAIT_OBJECT_0)
		{

		}
		else
		{
			break;
		}

		ResetEvent(m_hStopTimerQueue);

		DeleteTimerQueueTimer(m_hTimerQueue, m_hTimer[0], NULL);
		DeleteTimerQueueTimer(m_hTimerQueue, m_hTimer[1], NULL);
	}
}

VOID/* static*/ CALLBACK CLogAnalysis::m_fnAnalysisApiA(PVOID lpParameter, BOOL bTimer)
{
	CFileInfo* param = (CFileInfo*)lpParameter;
	CLogBaseDlg* parent = (CLogBaseDlg*)param->filemaster;
	CLogConvert* sender = (CLogConvert*)param->master;
	UINT index = param->fileindex;

	int err = 0;
	char buf[1024] = {0};
	
	time_t t = time(NULL);
	struct tm tt;
	localtime_s(&tt, &t);
	sprintf_s(buf, "AnalysisApiA=>Analysis[%04d/%02d/%02d %02d:%02d:%02d]", 1900+tt.tm_year, 1+tt.tm_mon, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
	LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);

	CURL* curl;
	CURLcode res;

	char chPostHeader[] = {"Content-Type: application/json"};
	struct curl_slist *pPostList = NULL;
	pPostList = curl_slist_append(pPostList, chPostHeader);	

	CString strDate;
	strDate.Format("%04d/%02d/%02d", 1900+tt.tm_year, 1+tt.tm_mon, tt.tm_mday);
	strDate = "2018/09/05";

	CString strPostUrl;
	strPostUrl.Format("http://%s:%d/_all/taco/_search", theApp.m_chElasticIP, theApp.m_uElasticPORT);

	CString strPostBody;

	CString strUpdateDBUrl;
	strUpdateDBUrl.Format("http://10.99.0.85/EsInsert?type=A&api=");

	CPacket *pkt = new CPacket();
	pkt->content = new char[1];
	pkt->contentlen = 0;

	// 取得分析資料
	strPostBody.Format("{\"from\":0,\"size\":0,\"query\":{\"bool\":{\"must\":{\"range\":{\"time\":{\"gte\":\"%s 00:00:00\",\"lte\": \"%s 23:59:59\"}}}}},\"aggs\":{\"api\":{\"terms\":{\"field\":\"body.api\"},\"aggs\":{\"count\":{\"date_histogram\":{\"field\":\"time\",\"interval\":\"hour\"}}}}}}", strDate, strDate);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, strPostUrl);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogAnalysis::RecvFunc); 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
	if ((res = curl_easy_perform(curl)) != CURLE_OK)
	{
		// ERROR
		sprintf_s(buf, "AnalysisTradeA=>ElasticSearch[連線錯誤][%d]", res);
		LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);
		
		curl_easy_cleanup(curl);
		return;
	}
	curl_easy_cleanup(curl);

	CJsonParser jsp;
	jsp.Parse(pkt->content, pkt->contentlen);
	jsp.Trace();

	CString strPostValue;
	CString stri, strj, strapi, strname, strvalue, strApi, strName, strValue;
	for (UINT i=0; i<jsp.GetCount(CString("root.aggregations.api.buckets")); i++)
	{
		stri.Format("root.aggregations.api.buckets[%d]", i);
		strapi.Format("%s.key", stri);
		strApi = jsp[strapi];

		strPostBody.Empty();

		strj.Format("%s.count.buckets", stri);
		for (UINT j=0; j<jsp.GetCount(strj); j++)
		{
			strname.Format("%s.count.buckets[%d].key_as_string", stri, j);
			strvalue.Format("%s.count.buckets[%d].doc_count", stri, j);

			strName = jsp[strname];
			strValue = jsp[strvalue];

			strName.Replace("/", "");
			strName.Replace(" ", "");
			strName.Replace(":", "");

			strPostBody.Append(strName);
			strPostBody.Append(strValue);
			strPostBody.Append(",");
		}
	
		// 更新資料庫
		if (strPostBody.Right(1) == ',')
			strPostBody.TrimRight(',');
		
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, strUpdateDBUrl+strApi);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
		if ((res = curl_easy_perform(curl)) != CURLE_OK)
		{
			// ERROR
			sprintf_s(buf, "AnalysisTradeA=>DB[連線錯誤][%d]", res);
			LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);
		
			curl_easy_cleanup(curl);
			return;
		}
		curl_easy_cleanup(curl);
	}

	if (pkt->content)
		delete[] pkt->content;

	if (pkt)
		delete pkt;
}

VOID/* static*/ CALLBACK CLogAnalysis::m_fnAnalysisApiB(PVOID lpParameter, BOOL bTimer)
{
	CFileInfo* param = (CFileInfo*)lpParameter;
	CLogBaseDlg* parent = (CLogBaseDlg*)param->filemaster;
	CLogConvert* sender = (CLogConvert*)param->master;
	UINT index = param->fileindex;

	int err = 0;
	char buf[1024] = {0};
	
	time_t t = time(NULL);
	struct tm tt;
	localtime_s(&tt, &t);
	sprintf_s(buf, "AnalysisApiB=>Analysis[%04d/%02d/%02d %02d:%02d:%02d]", 1900+tt.tm_year, 1+tt.tm_mon, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
	LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);

	CURL* curl;
	CURLcode res;

	char chPostHeader[] = {"Content-Type: application/json"};
	struct curl_slist *pPostList = NULL;
	pPostList = curl_slist_append(pPostList, chPostHeader);	

	CString strDate;
	strDate.Format("%04d/%02d/%02d", 1900+tt.tm_year, 1+tt.tm_mon, tt.tm_mday);
	strDate = "2018/09/05";

	CString strPostUrl;
	strPostUrl.Format("http://%s:%d/_all/taco/_search", theApp.m_chElasticIP, theApp.m_uElasticPORT);

	CString strPostBody;

	CString strUpdateDBUrl;
	strUpdateDBUrl.Format("http://10.99.0.85/EsInsert?type=B&api=");

	CPacket *pkt = new CPacket();
	pkt->content = new char[1];
	pkt->contentlen = 0;

	// 再取得分析資料
	strPostBody.Format("{\"from\":0,\"size\":0,\"query\":{\"bool\":{\"must\":{\"range\":{\"time\":{\"gte\":\"%s 00:00:00\",\"lte\":\"%s 23:59:59\"}}}}},\"aggs\":{\"api\":{\"terms\":{\"field\":\"body.api\"},\"aggs\":{\"hot10\":{\"terms\":{\"field\":\"body.stock\",\"size\":\"10\"}}}}}}", strDate, strDate);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, strPostUrl);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogAnalysis::RecvFunc); 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
	if ((res = curl_easy_perform(curl)) != CURLE_OK)
	{
		// ERROR
		sprintf_s(buf, "AnalysisTradeB=>ElasticSearch[連線錯誤][%d]", res);
		LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);
		
		curl_easy_cleanup(curl);
		return;
	}
	curl_easy_cleanup(curl);

	CJsonParser jsp;
	jsp.Parse(pkt->content, pkt->contentlen);
	jsp.Trace();

	CString strPostValue;
	CString stri, strj, strapi, strname, strvalue, strApi, strName, strValue;
	for (UINT i=0; i<jsp.GetCount(CString("root.aggregations.api.buckets")); i++)
	{
		stri.Format("root.aggregations.api.buckets[%d]", i);
		strapi.Format("%s.key", stri);
		strApi = jsp[strapi];

		strPostBody.Empty();
		strPostBody.Format("{");

		CString strtmp("");

		strj.Format("%s.hot10.buckets", stri);
		for (UINT j=0; j<jsp.GetCount(strj); j++)
		{
			strname.Format("%s.hot10.buckets[%d].key", stri, j);
			strvalue.Format("%s.hot10.buckets[%d].doc_count", stri, j);

			strName = jsp[strname];
			strValue = jsp[strvalue];

			strName.Replace("/", "");
			strName.Replace(" ", "");
			strName.Replace(":", "");

			strtmp.Format("\"top%d\":{\"stk\":\"%s\",\"val\":%s}", j+1, strName, strValue);

			strPostBody.Append(strtmp);
			strPostBody.Append(",");
		}
	
		// 更新資料庫
		if (strPostBody.Right(1) == ',')
			strPostBody.TrimRight(',');
		
		strPostBody.Append("}");

		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, strUpdateDBUrl+strApi);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
		if ((res = curl_easy_perform(curl)) != CURLE_OK)
		{
			// ERROR
			sprintf_s(buf, "AnalysisTradeB=>DB[連線錯誤][%d]", res);
			LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);
		
			curl_easy_cleanup(curl);
			return;
		}
		curl_easy_cleanup(curl);
	}

	if (pkt->content)
		delete[] pkt->content;

	if (pkt)
		delete pkt;
}

VOID/* static*/ CALLBACK CLogAnalysis::m_fnAnalysisUserA(PVOID lpParameter, BOOL bTimer)
{
	CFileInfo* param = (CFileInfo*)lpParameter;
	CLogBaseDlg* parent = (CLogBaseDlg*)param->filemaster;
	CLogConvert* sender = (CLogConvert*)param->master;
	UINT index = param->fileindex;

	int err = 0;
	char buf[1024] = {0};
	
	time_t t = time(NULL);
	struct tm tt;
	localtime_s(&tt, &t);
	sprintf_s(buf, "AnalysisUserA=>Analysis[%04d/%02d/%02d %02d:%02d:%02d]", 1900+tt.tm_year, 1+tt.tm_mon, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
	LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);

	CURL* curl;
	CURLcode res;

	char chPostHeader[] = {"Content-Type: application/json"};
	struct curl_slist *pPostList = NULL;
	pPostList = curl_slist_append(pPostList, chPostHeader);	

	CString strDate;
	strDate.Format("%04d/%02d/%02d", 1900+tt.tm_year, 1+tt.tm_mon, tt.tm_mday);
	strDate = "2018/09/05";

	CString strPostUrl;
	strPostUrl.Format("http://%s:%d/_all/taco/_search", theApp.m_chElasticIP, theApp.m_uElasticPORT);

	CString strPostBody;

	CString strUpdateDBUrl;
	strUpdateDBUrl.Format("http://10.99.0.85/EsInsert?type=C&api=");

	CPacket *pkt = new CPacket();
	pkt->content = new char[1];
	pkt->contentlen = 0;

	// 再取得分析資料
//	strPostBody.Format("{\"from\":0,\"size\":0,\"query\":{\"bool\":{\"must\":{\"range\":{\"time\":{\"gte\":\"%s 00:00:00\",\"lte\":\"%s 23:59:59\"}}}}},\"aggs\":{\"api\":{\"terms\":{\"field\":\"body.api\"},\"aggs\":{\"hot10\":{\"terms\":{\"field\":\"body.stock\",\"size\":\"10\"}}}}}}", strDate, strDate);
//	strPostBody.Format("{\"from\":0,\"size\":0,\"query\":{\"match\":{\"body.data_type\":\"login\"}},\"aggs\":{\"user\":{\"terms\":{\"field\":\"uid\",\"size\":99999},\"aggs\":{\"duration\":{\"sum\":{\"field\":\"body.duration\"}},\"lastlogin\":{\"terms\":{\"field\":\"body.login\",\"size\":1,\"order\":{\"_key\":\"desc\"}}}}}}}"/*, strDate, strDate*/); 
	strPostBody.Format("{\"from\":0,\"size\":0,\"query\":{\"match\":{\"body.data_type\":\"login\"}},\"aggs\":{\"user\":{\"terms\":{\"field\":\"uid\",\"size\":99999},\"aggs\":{\"state\":{\"stats\":{\"field\":\"body.duration\"}},\"lastlogin\":{\"terms\":{\"field\":\"body.login\",\"size\":1,\"order\":{\"_key\":\"desc\"}},\"aggs\":{\"duration\":{\"max\":{\"field\":\"body.duration\"}}}}}}}}");

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, strPostUrl);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CLogAnalysis::RecvFunc); 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)pkt); 
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pPostList);
	if ((res = curl_easy_perform(curl)) != CURLE_OK)
	{
		// ERROR
		sprintf_s(buf, "AnalysisTradeB=>ElasticSearch[連線錯誤][%d]", res);
		LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);
		
		curl_easy_cleanup(curl);
		return;
	}
	curl_easy_cleanup(curl);

	CJsonParser jsp;
	jsp.Parse(pkt->content, pkt->contentlen);
	jsp.Trace();
	/*
	CString strPostValue;
	CString stri, strj, strapi, strname, strvalue, strApi, strName, strValue;
	for (UINT i=0; i<jsp.GetCount(CString("root.aggregations.buckets")); i++)
	{
		stri.Format("root.aggregations.buckets[%d]", i);
		struid.Format("%s.key", stri);
		strUid = jsp[struid];

		strPostBody.Empty();
		strPostBody.Format("{");

		CString strtmp("");

		strj.Format("%s.state.count", stri);
		strUserConnectCount = jsp[strj];

		strj.Format("%s.state.sum", stri);
		strUserConnectTime = jsp[strj];

		strj.Format("%s.state.min", stri);
		strUserConnectTimeMin = jsp[strj];

		strj.Format("%s.state.max", stri);
		strUserConnectTimeMax = jsp[strj];

		strj.Format("%s.state.avg", stri);
		strUserConnectTimeAvg = jsp[strj];

		strj.Format("%s.state.lastlogin.bucket[0].key_as_string", stri);
		strUserLastConnect = jsp[strj];

		strj.Format("%s.state.lastlogin.bucket[0].duration.value", stri);
		strUserLastConnectTime = jsp[strj];
	
		// 更新資料庫
		if (strPostBody.Right(1) == ',')
			strPostBody.TrimRight(',');
		
		strPostBody.Append("}");

		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, strUpdateDBUrl+strApi);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPostBody);
		if ((res = curl_easy_perform(curl)) != CURLE_OK)
		{
			// ERROR
			sprintf_s(buf, "AnalysisTradeB=>DB[連線錯誤][%d]", res);
			LogSend(buf, ID_SYSLOG_SEVERITY_ERROR);
		
			curl_easy_cleanup(curl);
			return;
		}
		curl_easy_cleanup(curl);
	}
	*/
	if (pkt->content)
		delete[] pkt->content;

	if (pkt)
		delete pkt;
}

/*static */size_t CLogAnalysis::RecvFunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t realsize = size * nmemb;

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
