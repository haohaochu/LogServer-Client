#include "stdafx.h"
#include ".\stock.h"

// Load Stock Dll
HINSTANCE			hInstStockDll;

DLL_READSTKREC		AP_ReadStkRec;
DLL_WRITESTKREC		AP_WriteStkRec;
DLL_READSTKREC_J	AP_ReadStkRec_J;
DLL_WRITESTKREC_J	AP_WriteStkRec_J;

DLL_ADDSTKTYPE		AP_AddStkType;
DLL_DELSTKTYPE		AP_DelStkType;
DLL_READSTKTYPE		AP_ReadStkType;
DLL_READSTKTYPE_J	AP_ReadStkType_J;

DLL_ADDSTK			AP_AddStk;
DLL_DELSTK			AP_DelStk;
DLL_FLUSHSTK		AP_FlushStk;

DLL_GETSTKCOUNT		AP_GetStkCount;
DLL_INDEXOFSTKID	AP_IndexOfStkID;

DLL_SETSTKNOTIFY	AP_SetStkNotify;
DLL_DELSTKNOTIFY	AP_DelStkNotify;
DLL_SETSTKNOTIFY_J	AP_SetStkNotify_J;
DLL_DELSTKNOTIFY_J	AP_DelStkNotify_J;
DLL_DELSTKALLNOTIFY	AP_DelStkAllNotify;

DLL_READ20POW		AP_Read20Pow;
DLL_WRITE20POW		AP_Write20Pow;

DLL_READSTKNEWS		AP_ReadStkNews;
DLL_WRITESTKNEWS	AP_WriteStkNews;
DLL_CLEARSTKNEWS	AP_ClearStkNews;

DLL_GETSTKLASTERR	AP_GetStkLastErr;

DLL_SETMARKETSTATUS AP_SetMarketStatus;
DLL_GETMARKETSTATUS AP_GetMarketStatus;
DLL_SETMARKETACTIVE	AP_SetMarketActive;
DLL_GETMARKETACTIVE	AP_GetMarketActive;
DLL_SETMARKETHOLIDAY	AP_SetMarketHoliday;
DLL_GETMARKETHOLIDAY	AP_GetMarketHoliday;
DLL_GETSTATUS		AP_GetStatus;

//--------------------------------------------------//
// Define Function
BOOL LoadStockDll(void)
{
	hInstStockDll = LoadLibrary(STOCKDLL);

	if ( hInstStockDll == NULL )
		return FALSE ;
	else
	{
		AP_ReadStkRec = (DLL_READSTKREC) GetProcAddress(hInstStockDll,"DLL_ReadStkRec");
		AP_WriteStkRec = (DLL_WRITESTKREC) GetProcAddress(hInstStockDll,"DLL_WriteStkRec");
		AP_ReadStkRec_J = (DLL_READSTKREC_J) GetProcAddress(hInstStockDll,"DLL_ReadStkRec_J");
		AP_WriteStkRec_J = (DLL_WRITESTKREC_J) GetProcAddress(hInstStockDll,"DLL_WriteStkRec_J");

		AP_AddStkType = (DLL_ADDSTKTYPE) GetProcAddress(hInstStockDll,"DLL_AddStkType");
		AP_DelStkType = (DLL_DELSTKTYPE) GetProcAddress(hInstStockDll,"DLL_DelStkType");
		AP_ReadStkType = (DLL_READSTKTYPE) GetProcAddress(hInstStockDll,"DLL_ReadStkType");
		AP_ReadStkType_J = (DLL_READSTKTYPE_J) GetProcAddress(hInstStockDll,"DLL_ReadStkType_J");

		AP_AddStk = (DLL_ADDSTK) GetProcAddress(hInstStockDll,"DLL_AddStk");
		AP_DelStk = (DLL_DELSTK) GetProcAddress(hInstStockDll,"DLL_DelStk");
		AP_FlushStk = (DLL_FLUSHSTK) GetProcAddress(hInstStockDll,"DLL_FlushStk");

		AP_GetStkCount = (DLL_GETSTKCOUNT) GetProcAddress(hInstStockDll,"DLL_GetStkCount");
		AP_IndexOfStkID = (DLL_INDEXOFSTKID) GetProcAddress(hInstStockDll,"DLL_IndexOfStkID");

		AP_SetStkNotify = (DLL_SETSTKNOTIFY) GetProcAddress(hInstStockDll,"DLL_SetStkNotify");
		AP_DelStkNotify = (DLL_DELSTKNOTIFY) GetProcAddress(hInstStockDll,"DLL_DelStkNotify");
		AP_SetStkNotify_J = (DLL_SETSTKNOTIFY_J) GetProcAddress(hInstStockDll,"DLL_SetStkNotify_J");
		AP_DelStkNotify_J = (DLL_DELSTKNOTIFY_J) GetProcAddress(hInstStockDll,"DLL_DelStkNotify_J");
		AP_DelStkAllNotify = (DLL_DELSTKALLNOTIFY) GetProcAddress(hInstStockDll,"DLL_DelStkAllNotify");
		AP_Read20Pow = (DLL_READ20POW) GetProcAddress(hInstStockDll,"DLL_Read20Pow");
		AP_Write20Pow = (DLL_WRITE20POW) GetProcAddress(hInstStockDll,"DLL_Write20Pow");
		AP_ReadStkNews = (DLL_READSTKNEWS) GetProcAddress(hInstStockDll,"DLL_ReadStkNews");
		AP_WriteStkNews = (DLL_WRITESTKNEWS) GetProcAddress(hInstStockDll,"DLL_WriteStkNews");
		AP_ClearStkNews = (DLL_CLEARSTKNEWS) GetProcAddress(hInstStockDll,"DLL_ClearStkNews");
		AP_GetStkLastErr = (DLL_GETSTKLASTERR) GetProcAddress(hInstStockDll,"DLL_GetStkLastErr");

		AP_SetMarketStatus = (DLL_SETMARKETSTATUS) GetProcAddress(hInstStockDll,"DLL_SetMarketStatus");
		AP_GetMarketStatus = (DLL_GETMARKETSTATUS) GetProcAddress(hInstStockDll,"DLL_GetMarketStatus");

		AP_SetMarketActive = (DLL_SETMARKETACTIVE) GetProcAddress(hInstStockDll,"DLL_SetMarketActive");
		AP_GetMarketActive = (DLL_GETMARKETACTIVE) GetProcAddress(hInstStockDll,"DLL_GetMarketActive");

		AP_SetMarketHoliday = (DLL_SETMARKETHOLIDAY) GetProcAddress(hInstStockDll,"DLL_SetMarketHoliday");
		AP_GetMarketHoliday = (DLL_GETMARKETHOLIDAY) GetProcAddress(hInstStockDll,"DLL_GetMarketHoliday");

		AP_GetStatus = (DLL_GETSTATUS) GetProcAddress(hInstStockDll,"DLL_GetStatus");
	}
	return TRUE ;
}

void UnLoadStockDll(void)
{
	if ( hInstStockDll )
		FreeLibrary(hInstStockDll) ;
}

//------------------ Ū�g�ӫ~���� ------------------//
int ReadStkRec(char *ID, StkItem *SI)
{
	if ( hInstStockDll && AP_ReadStkRec )
		return (AP_ReadStkRec(ID,SI));
	else
		return STOCKDLL_LOAD_ERROR;
}

BOOL WriteStkRec(char *ID, StkItem SI)
{
	if ( hInstStockDll && AP_WriteStkRec )
		return (AP_WriteStkRec(ID,SI));
	else
		return FALSE;
}

int ReadStkRec_J(WORD Idx, StkItem *SI)
{
	if ( hInstStockDll && AP_ReadStkRec_J )
		return (AP_ReadStkRec_J(Idx,SI));
	else
		return STOCKDLL_LOAD_ERROR;
}

BOOL WriteStkRec_J(WORD Idx, StkItem SI)
{
	if ( hInstStockDll && AP_WriteStkRec_J )
		return (AP_WriteStkRec_J(Idx,SI));
	else
		return FALSE;
}
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���O ----------------//
BOOL AddStkType(char Type, char *TypeName)
{
	// ����{
	return TRUE;
}

BOOL DelStkType(char Type)
{
	// ����{
	return TRUE;
}

int ReadStkType(char *MarketType, char *Type, TypeIndex *TI)
{
	if ( hInstStockDll && AP_ReadStkType )
		return (AP_ReadStkType(MarketType,Type,TI));
	else
		return -1;
}

int ReadStkType_J(WORD TypeIdx, TypeIndex *TI)
{
	if ( hInstStockDll && AP_ReadStkType_J )
		return (AP_ReadStkType_J(TypeIdx,TI));
	else
		return -1;
}
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���� ----------------//
BOOL AddStk(char *ID, char *IDName, char* MarketType, char* Type)
{
	if ( hInstStockDll && AP_AddStk )
		return (AP_AddStk(ID,IDName,MarketType,Type));
	else
		return FALSE;
}

BOOL DelStk(char *ID)
{
	if ( hInstStockDll && AP_DelStk )
		return (AP_DelStk(ID));
	else
		return FALSE;
}

BOOL FlushStk(char *ID, char *MarketType)
{
	if ( hInstStockDll && AP_FlushStk )
		return (AP_FlushStk(ID,MarketType));
	else
		return FALSE;
}
//--------------------------------------------------//

//------------------ �ӫ~���د��� ------------------//
int GetStkCount(void)
{
	if ( hInstStockDll && AP_GetStkCount )
		return (AP_GetStkCount());
	else
		return -1;
}

int IndexOfStkID(char *ID)
{
	if ( hInstStockDll && AP_IndexOfStkID )
		return (AP_IndexOfStkID(ID));
	else
		return -1;
}
//--------------------------------------------------//

//--------------- �Ұʩ������ӫ~�q�� ---------------//
BOOL SetStkNotify(HWND Hwnd, char *ID, UINT WM_Message, WPARAM Wparam, LPARAM Lparam)
{
	// ����{
	return TRUE;
}

BOOL DelStkNotify(HWND Hwnd, char *ID)
{
	// ����{
	return TRUE;
}

BOOL SetStkNotify_J(HWND Hwnd, WORD Idx, UINT WM_Message, WPARAM Wparam, LPARAM Lparam)
{
	// ����{
	return TRUE;
}

BOOL DelStkNotify_J(HWND Hwnd, WORD Idx)
{
	// ����{
	return TRUE;
}

BOOL DelStkAllNotify(void)
{
	// ����{
	return TRUE;
}
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
BOOL Read20Pow(Stk20Pow *SP)
{
	// ����{
	return TRUE;
}

BOOL Write20Pow(Stk20Pow SP)
{
	// ����{
	return TRUE;
}
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
int ReadStkNews(int Idx, StkNews *SN)
{
	// ����{
	return 0;
}

int WriteStkNews(int Idx, char *ID)
{
	// ����{
	return 0;
}

BOOL ClearStkNews(void)
{
	// ����{
	return TRUE;
}
//--------------------------------------------------//

//---------------- �̫�@�����~�T�� ----------------//
BOOL GetStkLastErr(char *LastErrorMessage)
{
	// ����{
	return TRUE;
}
//--------------------------------------------------//

//---------------- �޲zstock.dll�u��禡 -----------//
void SetMarketStatus(char *chMarket, BOOL bEnable)
{
	if ( hInstStockDll && AP_SetMarketStatus )
		AP_SetMarketStatus(chMarket,bEnable);
}

int GetMarketStatus(char *chMarket)
{
	if ( hInstStockDll && AP_GetMarketStatus )
		return (AP_GetMarketStatus(chMarket));
	else
		return STOCKDLL_LOAD_ERROR;
}

void SetMarketActive(char* chMarket, BOOL bEnable)
{
	if ( hInstStockDll && AP_SetMarketActive )
		AP_SetMarketActive(chMarket,bEnable);
}

int GetMarketActive(char* chMarket)
{
	if ( hInstStockDll && AP_GetMarketActive )
		return (AP_GetMarketActive(chMarket));
	else
		return STOCKDLL_LOAD_ERROR;
}

void SetMarketHoliday(char* chMarket, BOOL bEnable)
{
	if ( hInstStockDll && AP_SetMarketHoliday )
		AP_SetMarketHoliday(chMarket,bEnable);
}

int GetMarketHoliday(char* chMarket)
{
	if ( hInstStockDll && AP_GetMarketHoliday )
		return (AP_GetMarketHoliday(chMarket));
	else
		return STOCKDLL_LOAD_ERROR;
}

BOOL GetStatus(MapFileHead* pHead)
{
	if ( hInstStockDll && AP_GetStatus )
		return (AP_GetStatus(pHead));
	else
		return FALSE;
}
//--------------------------------------------------//