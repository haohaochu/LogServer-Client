// �w�q�@�Ǳ`�Ϊ��u��禡
BOOL	DoParseaData(char *chData, int nDataLen, char cDivideSymbol, CMapStringToString *pMap);
BOOL	IsDirectoryExists(CString strDirectory);	// �ˬd�ؿ��O�_�s�b
BOOL	ForceDirectories(CString strDirectory);		// �j��ͥؿ�
BOOL	RenameFile(CString strFrom, CString strTo);	// ����ɦW
BOOL	MyCopyFile(CString strFrom, CString strTo);	// �ƻs�ɮ�
BOOL	MyMoveFile(CString strFrom, CString strTo);	// �����ɮ�
BOOL	MyDeleteFile(CString strFilePath);			// �R���ɮ�
BOOL	IsFileExist(CString strFileName);			// �ˬd�ɮ׬O�_�s�b
void	Cut_Space(char* chData);					// �R���ťդδ���Ÿ�
void	Cut_Zero(char *chData);						// �h���p���I�H�U��0���Ʀr
int		StringToUTF8(char* chData, int nSize);		// �N²�餤���নUTF8
