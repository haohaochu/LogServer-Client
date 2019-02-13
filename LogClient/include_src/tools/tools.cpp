// �w�q�@�Ǳ`�Ϊ��u��禡
#include "stdafx.h"

BOOL DoParseaData(char *chData, int nDataLen, char cDivideSymbol, CMapStringToString *pMap)
{
	int		i,j,k,l;
	char	Key[128],Value[4069];
	BOOL	bStartFlag;

	if ( !pMap )
		return FALSE;

	bStartFlag = TRUE;
	for ( i=0,j=0,k=0 ; i<nDataLen ; i++ )
	{
		if ( bStartFlag )
		{
			Key[j] = chData[i];
			if ( chData[i] == '=' )
			{
				Key[j] = 0;
				bStartFlag = FALSE;
				k = 0;
			}
			j++;
		}
		else
		{
			Value[k] = chData[i];
			if ( chData[i] == cDivideSymbol )
			{
				Value[k] = 0;
				// �h���ť�
				for ( l=k-1 ; l>=0 ; l-- )
				{
					if ( Value[l] == 0x20 )
						Value[l] = 0;
					else
						break;
				}
				bStartFlag = TRUE;
				if ( l != -1 )
					pMap->SetAt(Key,Value);
				j = 0;
			}
			k++;
		}
	}
	
	if ( !bStartFlag )
	{
		Value[k] = 0;
		pMap->SetAt(Key,Value);
	}
	return TRUE;
}

// �ˬd�ؿ��O�_�s�b
BOOL IsDirectoryExists(CString strDirectory)
{
	DWORD		nCode;

	nCode = GetFileAttributes(strDirectory);
	return (nCode != -1 && (FILE_ATTRIBUTE_DIRECTORY & nCode) != 0 );
}

// �j��ͥؿ�
BOOL ForceDirectories(CString strDirectory)
{
	CString	strBuf;
	int		nLen;

	if ( (nLen=strDirectory.GetLength()) == 0 )
		return FALSE;

	if ( IsDirectoryExists(strDirectory) )
		return TRUE;
	else
	{
		strBuf = strDirectory.Left(strDirectory.ReverseFind('\\'));
		return (ForceDirectories(strBuf) && CreateDirectory(strDirectory,NULL));
	}
}

// ����ɦW
BOOL RenameFile(CString strFrom, CString strTo)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024],buf1[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_RENAME;
	sprintf(buf,"%s%c",strFrom,0);
	FileOp.pFrom = buf;
	sprintf(buf1,"%s%c",strTo,0);
	FileOp.pTo = buf1;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// �ƻs�ɮ�
BOOL MyCopyFile(CString strFrom, CString strTo)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024],buf1[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_COPY;
	sprintf(buf,"%s%c",strFrom,0);
	FileOp.pFrom = buf;
	sprintf(buf1,"%s%c",strTo,0);
	FileOp.pTo = buf1;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// �����ɮ�
BOOL MyMoveFile(CString strFrom, CString strTo)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024],buf1[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_MOVE;
	sprintf(buf,"%s%c",strFrom,0);
	FileOp.pFrom = buf;
	sprintf(buf1,"%s%c",strTo,0);
	FileOp.pTo = buf1;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// �R���ɮ�
BOOL MyDeleteFile(CString strFilePath)
{
	SHFILEOPSTRUCT		FileOp;
	char				buf[1024];

	memset(&FileOp,NULL,sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE;
	sprintf(buf,"%s%c",strFilePath,0);
	FileOp.pFrom = buf;
	FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	return ( SHFileOperation(&FileOp) == 0 );
}

// �ˬd�ɮ׬O�_�s�b
BOOL IsFileExist(CString strFileName)
{
	WIN32_FIND_DATA		FD;
	HANDLE				hFile;
	BOOL				bRetCode;

	if ( (hFile=FindFirstFile((LPCTSTR)strFileName,&FD)) == INVALID_HANDLE_VALUE )
		bRetCode = FALSE;
	else
	{
		bRetCode = TRUE;
		FindClose(hFile);
	}

	return bRetCode;
}

// �R���ťդδ���Ÿ�
void Cut_Space(char* chData)
{
	char* pt = chData + strlen(chData) - 1;

	while ( pt >= chData )
	{
		if ( *pt != 0x20 && *pt != 0x0d && *pt != 0x0a )
		{
			*(pt+1) = 0;
			break;
		}
		pt --;
	}

	if ( pt + 1 == chData )
		*(pt+1) = 0;
}

// �h���p���I�H�U��0���Ʀr
void Cut_Zero(char *chData)
{
	char	*pDot,*pt;

	if ( (pDot=strchr(chData,'.')) != NULL )
	{
		pt = chData + strlen(chData) - 1;
		while ( pt >= pDot )
		{
			if ( *pt != '0' )
			{
				if ( *pt == '.' )
					*pt = 0;
				else
					*(pt+1) = 0;
				break;
			}
			pt --;
		}
	}
}

// �N²�餤���নUTF8
int StringToUTF8(char* chData, int nSize)
{
	WCHAR	wszData[128];

	// char�নwide-char(Unicode)
	if ( MultiByteToWideChar( 936 , 0, chData, -1, wszData, sizeof(wszData)/sizeof(wszData[0])) == 0 )
		return 0;

	// wide-char(Unicode)�নUTF8
	return (WideCharToMultiByte( CP_UTF8 , 0, wszData, -1, chData, nSize, NULL, NULL));
}