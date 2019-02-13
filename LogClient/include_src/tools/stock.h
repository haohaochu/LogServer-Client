#define STOCKDLL		"stock.dll"

const int		STOCKDLL_LOAD_ERROR		= -2;	// ���Jstock.dll���~
const int		STOCKDLL_SYSTEM_ERROR	= -1;	// �t�Φ��~
const int		STOCKDLL_DATA_NOT_FOUND	= 0;	// �d�L���
const int		STOCKDLL_CORRECT_DATA	= 1;	// ��ƥ��T
const int		STOCKDLL_ISSUE_DATA		= 2;	// ��Ʀ��~(�ӥ����O��Ƥ��ή�)

#pragma pack(1)
//�Ѳ��ӫ~��쵲�c�ŧi
typedef struct tagStockItem {
	char	MarketType[3];	// �������O
	char	Type[3];		// ���O�N��
	char    IdCode[11];		// Stock ID
	char    Name[11];		// Stock Name
	int     Year;			// �ɶ�:�~(yyyy)
	char    Month;			// �ɶ�:��
	char    Day;			// �ɶ�:��
	char    Hour;			// �ɶ�:��
	char    Minute;			// �ɶ�:��
	char    UpDnFlag;		// ���^���X��  1:�^��  2:����   0:���`
	double  Sell;			// Price of Sell(�j�L:���浧��)
	double  Buy;			// Price of Buy(�j�L:�����`���B)
	double  Deal;			// Price of �����
	double  Hi;				// Price of Hi
	double  Low;			// Price of Low
	double  Volum;			// Price of ����q
	double  YClose;			// Price of ���L
	double  UpPrice;		// Price of ����
	double  DnPrice;		// Price of �^��
	double  UpDnPrice;		// Price of ���^�T
	double  Open;			// Price of �}�L
	double  CBuy;			// �e�R����(�j�L:�`�e�R����)
	double  CSell;			// �e�浧��(�j�L:�`�e�浧��)
	double  CBVolum;		// �e�R�i��(�U��,�j�L:�`�e�R�i��)
	double  CSVolum;		// �e��i��(�U��,�j�L:�`�e��i��)
	char    Class;			// �W���B�d�ӫ~	-- 00�G���`�F01�Gĵ�ܡF02�G�B�m1�F03�Gĵ�ܤγB�m1�F04�G�B�m2�F05�Gĵ�ܤγB�m2
							// ���v�ӫ~		-- 00�G���`���p�F01~60�G���`���p�ɶ�(����)�F98�G���`���p�Ѱ��F99�G���`���p�W�L60����
	long    StartDay;		// ��q(��]�p�γ~�G�W����� 19980615)
	double  Capital;		// �����(��]�p�γ~�G�ѥ�)
	double	Buy5[6];		// �̨Τ��ɶR�i��
	double	BVolum5[6];		// �̨Τ��ɶR�i�q
	double	Sell5[6];		// �̨Τ��ɽ�X��
	double	SVolum5[6];		// �̨Τ��ɽ�X�q
	long	NOffSet;		// �����ܦX����
	double	Reckon;			// �����
	int		nFlag;			// Bit 1�G��ܬO�_���x�W�����ѡC
	double  CUpPrice;		// �L��Price of ����
	double  CDnPrice;		// �L��Price of �^��
} StkItem;

// Useless
// ���������c�ŧi(�ثe�L�ϥ�)
typedef struct tagPower {
	double  Value;			// ���ƭ�
	double  YClose;			// �Q��
	double  UpDn;			// ���^�T
	double  DealMoney;		// ������B(�H�U�T���ثe�u���j�L��)
	double  DealVolum;		// ����q
	double  DealCount;		// ���浧��
} Pow;

typedef struct tagStock20Power {
    Pow		Pow20[20];		// 0:�j�L   1->19 �U��������
} Stk20Pow;

//�s�D(�ثe�L�ϥ�)
typedef struct tagStockNews {
	int		LastNo;
	int		MaxNo;
    char	News[81];
} StkNews;
// Useless

//���O���c
typedef struct tagStockTypeIndex {
	char	MarketType[3];	// �������O
	char	Type[3];		// ���O�N��
    WORD	Idx;			// �����O�_�l��m(�Y�� ? ���}�l)
    WORD	Count;			// �����O�ҧt�ӫ~�ƥ�
    char	Name[30];		// �����O����W��
} TypeIndex;

typedef struct tagMapFileHead {
	WORD		wVersion;			// �����O
	DWORD		dwFileSize;			// Stock.all���ɮפj�p
	DWORD		dwWriteOffset;		// �ثe�g�JStock.all�ɮת���}
	UINT		uCnts;				// Stock.all�ɮפ��Ѳ��ӫ~���`��
	UINT		usage;				// �}��Stock.all�ɮת��{���ƥ�
	DWORD		dwTickCnt;			// �̫���Stock.all��ƪ��ɶ�
	WORD		wEnableSrc;			// �S���D�����
	WORD		wActiveSrc;			// �L�������
	WORD		wHolidaySrc;		// �񰲪����
} MapFileHead;
#pragma pack()

//------------------ Ū�g�ӫ~���� ------------------//
typedef	int	 (__cdecl *DLL_READSTKREC)		(char *ID, StkItem*);
typedef	BOOL (__cdecl *DLL_WRITESTKREC)		(char *ID, StkItem);
typedef	int	 (__cdecl *DLL_READSTKREC_J)	(WORD Idx, StkItem*);
typedef	BOOL (__cdecl *DLL_WRITESTKREC_J)	(WORD Idx, StkItem);
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���O ----------------//
typedef	BOOL (__cdecl *DLL_ADDSTKTYPE)		(char Type, char *TypeName);	// ����{
typedef	BOOL (__cdecl *DLL_DELSTKTYPE)		(char Type);					// ����{
typedef	int	 (__cdecl *DLL_READSTKTYPE)		(char *MarketType, char *Type, TypeIndex*);
typedef	int	 (__cdecl *DLL_READSTKTYPE_J)	(WORD TypeIdx, TypeIndex*);
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���� ----------------//
typedef	BOOL (__cdecl *DLL_ADDSTK)			(char *ID, char *IDName, char* MarketType, char* Type);
typedef	BOOL (__cdecl *DLL_DELSTK)			(char *ID);
typedef	BOOL (__cdecl *DLL_FLUSHSTK)		(char *ID, char *MarketType);
//--------------------------------------------------//

//------------------ �ӫ~���د��� ------------------//
typedef	int	 (__cdecl *DLL_GETSTKCOUNT)		(void);
typedef	int	 (__cdecl *DLL_INDEXOFSTKID)	(char *Idx);
//--------------------------------------------------//

//--------------- �Ұʩ������ӫ~�q�� ---------------//
typedef	BOOL (__cdecl *DLL_SETSTKNOTIFY)	(HWND Hwnd, char *ID, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// ����{
typedef	BOOL (__cdecl *DLL_DELSTKNOTIFY)	(HWND Hwnd, char *ID);	// ����{
typedef	BOOL (__cdecl *DLL_SETSTKNOTIFY_J)	(HWND Hwnd, WORD Idx, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// ����{
typedef	BOOL (__cdecl *DLL_DELSTKNOTIFY_J)	(HWND Hwnd, WORD Idx);	// ����{
typedef	BOOL (__cdecl *DLL_DELSTKALLNOTIFY)	(void);					// ����{
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
typedef	BOOL (__cdecl *DLL_READ20POW)		(Stk20Pow*);	// ����{
typedef	BOOL (__cdecl *DLL_WRITE20POW)		(Stk20Pow);		// ����{
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
typedef	int	 (__cdecl *DLL_READSTKNEWS)		(int, StkNews*);// ����{
typedef	int	 (__cdecl *DLL_WRITESTKNEWS)	(int, char*);	// ����{
typedef	BOOL (__cdecl *DLL_CLEARSTKNEWS)	(void);			// ����{
//--------------------------------------------------//

//---------------- �̫�@�����~�T�� ----------------//
typedef	BOOL (__cdecl *DLL_GETSTKLASTERR)	(char *LastErrorMessage);	// ����{
//--------------------------------------------------//

//---------------- �޲zstock.dll�u��禡 ----------------//
typedef	void (__cdecl *DLL_SETMARKETSTATUS)	(char *chMarket, BOOL bEnable);
typedef	int	 (__cdecl *DLL_GETMARKETSTATUS)	(char *chMarket);
typedef	void (__cdecl *DLL_SETMARKETACTIVE)	(char *chMarket, BOOL bEnable);
typedef	int	 (__cdecl *DLL_GETMARKETACTIVE)	(char *chMarket);
typedef	void (__cdecl *DLL_SETMARKETHOLIDAY)(char *chMarket, BOOL bEnable);
typedef	int	 (__cdecl *DLL_GETMARKETHOLIDAY)(char *chMarket);
typedef	BOOL (__cdecl *DLL_GETSTATUS)		( MapFileHead* pHead);
//--------------------------------------------------//

//------------------ Ū�g�ӫ~���� ------------------//
extern	DLL_READSTKREC		AP_ReadStkRec;
extern	DLL_WRITESTKREC		AP_WriteStkRec;
extern	DLL_READSTKREC_J	AP_ReadStkRec_J;
extern	DLL_WRITESTKREC_J	AP_WriteStkRec_J;
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���O ----------------//
extern	DLL_ADDSTKTYPE		AP_AddStkType;
extern	DLL_DELSTKTYPE		AP_DelStkType;
extern	DLL_READSTKTYPE		AP_ReadStkType;
extern	DLL_READSTKTYPE_J	AP_ReadStkType_J;
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���� ----------------//
extern	DLL_ADDSTK			AP_AddStk;
extern	DLL_DELSTK			AP_DelStk;
extern	DLL_FLUSHSTK		AP_FlushStk;
//--------------------------------------------------//

//------------------ �ӫ~���د��� ------------------//
extern	DLL_GETSTKCOUNT		AP_GetStkCount;
extern	DLL_INDEXOFSTKID	AP_IndexOfStkID;
//--------------------------------------------------//

//--------------- �Ұʩ������ӫ~�q�� ---------------//
extern	DLL_SETSTKNOTIFY	AP_SetStkNotify;
extern	DLL_DELSTKNOTIFY	AP_DelStkNotify;
extern	DLL_SETSTKNOTIFY_J	AP_SetStkNotify_J;
extern	DLL_DELSTKNOTIFY_J	AP_DelStkNotify_J;
extern	DLL_DELSTKALLNOTIFY	AP_DelStkAllNotify;
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
extern	DLL_READ20POW		AP_Read20Pow;
extern	DLL_WRITE20POW		AP_Write20Pow;
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
extern	DLL_READSTKNEWS		AP_ReadStkNews;
extern	DLL_WRITESTKNEWS	AP_WriteStkNews;
extern	DLL_CLEARSTKNEWS	AP_ClearStkNews;
//--------------------------------------------------//

//---------------- �̫�@�����~�T�� ----------------//
extern	DLL_GETSTKLASTERR	AP_GetStkLastErr;
//--------------------------------------------------//

//---------------- �޲zstock.dll�u��禡 -----------//
extern	DLL_SETMARKETSTATUS	AP_SetMarketStatus;
extern	DLL_GETMARKETSTATUS	AP_GetMarketStatus;
extern	DLL_SETMARKETACTIVE	AP_SetMarketActive;
extern	DLL_GETMARKETACTIVE	AP_GetMarketActive;
extern	DLL_SETMARKETHOLIDAY	AP_SetMarketHoliday;
extern	DLL_GETMARKETHOLIDAY	AP_GetMarketHoliday;
extern	DLL_GETSTATUS		AP_GetStatus;
//--------------------------------------------------//

//--------------------------------------------------//
// Define Function
BOOL	LoadStockDll	(void);
void	UnLoadStockDll	(void);

//------------------ Ū�g�ӫ~���� ------------------//
int		ReadStkRec		(char *ID, StkItem*);
BOOL	WriteStkRec		(char *ID, StkItem);
int		ReadStkRec_J	(WORD Idx, StkItem*);
BOOL	WriteStkRec_J	(WORD Idx, StkItem);
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���O ----------------//
BOOL	AddStkType		(char Type, char *TypeName);	// ����{
BOOL	DelStkType		(char Type);					// ����{
int		ReadStkType		(char *MarketType, char *Type, TypeIndex*);
int		ReadStkType_J	(WORD TypeIdx, TypeIndex*);
//--------------------------------------------------//

//---------------- �s�W�R���ӫ~���� ----------------//
BOOL	AddStk			(char *ID, char *IDName, char* MarketType, char* Type);
BOOL	DelStk			(char *ID);
BOOL	FlushStk		(char *ID, char* MarketType);
//--------------------------------------------------//

//------------------ �ӫ~���د��� ------------------//
int		GetStkCount		(void);
int		IndexOfStkID	(char *Idx);
//--------------------------------------------------//

//--------------- �Ұʩ������ӫ~�q�� ---------------//
BOOL	SetStkNotify	(HWND Hwnd, char *ID, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// ����{
BOOL	DelStkNotify	(HWND Hwnd, char *ID);		// ����{
BOOL	SetStkNotify_J	(HWND Hwnd, WORD Idx, UINT WM_Message, WPARAM Wparam, LPARAM Lparam);	// ����{
BOOL	DelStkNotify_J	(HWND Hwnd, WORD Idx );	// ����{
BOOL	DelStkAllNotify	(void);					// ����{
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
BOOL	Read20Pow		(Stk20Pow*);	// ����{
BOOL	Write20Pow		(Stk20Pow);	// ����{
//--------------------------------------------------//

//---------------- 20�����ưӫ~���� ----------------//
int		ReadStkNews		(int, StkNews*);	// ����{
int		WriteStkNews	(int, char*);	// ����{
BOOL	ClearStkNews	(void);			// ����{
//--------------------------------------------------//

//---------------- �̫�@�����~�T�� ----------------//
BOOL	GetStkLastErr	(char *LastErrorMessage);	// ����{
//--------------------------------------------------//

//---------------- �޲zstock.dll�u��禡 -----------//
void	SetMarketStatus	(char* chMarket, BOOL bEnable);
int		GetMarketStatus	(char* chMarket);
void	SetMarketActive	(char* chMarket, BOOL bEnable);
int		GetMarketActive	(char* chMarket);
void	SetMarketHoliday(char* chMarket, BOOL bEnable);
int		GetMarketHoliday(char* chMarket);
BOOL	GetStatus		(MapFileHead* pHead);
//--------------------------------------------------//

// End Define Function
//--------------------------------------------------//