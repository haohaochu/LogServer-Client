#if !defined(AFX_MYLISTBOX_H__F158C05D_7447_4B18_BF13_C1C14F5F0011__INCLUDED_)
#define AFX_MYLISTBOX_H__F158C05D_7447_4B18_BF13_C1C14F5F0011__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MyListBox.h : header file
//

#define				MAXDATASIZE			256				// �̤j���
/////////////////////////////////////////////////////////////////////////////
// CMyListBox window

class CMyListBox : public CListBox
{
// Construction
public:
	CMyListBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyListBox)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMyListBox();

	// �s�W�@�����
	BOOL		InsertData(LPCTSTR chData);
	// �]�w��ܦ��(��l��, ���i�ʺA�蠟[�h���|�X���D])
	BOOL		InitializeMyListBox(int nRow,int nFlushTimer = -1);
	// �M��List Box�����
	BOOL		ResetContext(void);

	// Generated message map functions
protected:
	LPCTSTR			GetArrayText(int nIndex);		// �qBuffer(pContextArray)�����o���
	void			IncreaseLastIndex(void);		// �޲z�̫�@����ƪ�Index

	long			nContextArrayLastIndex;			// �̫�@����Index
	int				nMaxContextArraySize;			// ��̤ܳj�C��
	CStringArray*	pContextArray;					// �C�C���Buffer
	BOOL			bRealTimeFlush;					// �Y�ɧ�sUI�X��
	UINT_PTR		nTimerID;
	//{{AFX_MSG(CMyListBox)
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYLISTBOX_H__F158C05D_7447_4B18_BF13_C1C14F5F0011__INCLUDED_)
