/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: io.h

����: IO ģ�鹫��ͷ�ļ���



*******************************************************************************/


#ifndef _IO_
#define _IO_

#include "eosdef.h"

//
// ��ʼ��IO����ģ���һ����
//
VOID
IoInitializeSystem1(
	VOID
	);

//
// ��ʼ��IO����ģ��ڶ�����
//
VOID
IoInitializeSystem2(
	VOID
	);

//
// �������I/O����
//
STATUS 
IoCreateFile(
	IN PSTR FileName, 
	IN ULONG DesiredAccess, 
	IN ULONG ShareMode, 
	IN ULONG CreationDisposition, 
	IN ULONG FlagsAndAttributes, 
	OUT PHANDLE Handle
	);

//
// ɾ���ļ���
//
STATUS
IoDeleteFile(
	IN PSTR FileName
	);

//
// �õ��ļ���ʱ�����
//
STATUS
IoGetFileTime(
	IN HANDLE FileHandle,
	OUT PFILETIME CreationTime,
	OUT PFILETIME LastAccessTime,
	OUT PFILETIME LastWriteTime
	);

//
// �õ��ļ��ĳ��ȡ�
//
STATUS 
IoGetFileSize(
	IN HANDLE FileHandle,
	OUT PULONG FileSize
	);

//
// �����ļ���дλ�á�
//
STATUS
IoSetFilePointer(
	IN HANDLE FileHandle,
	IN LONG DistanceToMove,
	IN ULONG MoveMethod,
	OUT PULONG NewFilePointer
	);

//
// �õ��ļ�������ֵ��
//
STATUS
IoGetFileAttributes(
	IN PSTR FileName,
	OUT PULONG FileAttributes
	);

//
// �޸��ļ�������ֵ��
//
STATUS
IoSetFileAttributes(
	IN PSTR FileName,
	IN ULONG FileAttributes
	);

//
// ����һ��Ŀ¼��
//
STATUS
IoCreateDirectory(
	IN PSTR PathName
	);

//
// ɾ��һ��Ŀ¼��
//
STATUS
IoRemoveDirectory(
	IN PSTR PathName
	);

//
// ��ָ����ŵĿ���̨��
//
STATUS
IoOpenConsole(
	IN ULONG ConsoleIndex,
	OUT PHANDLE Handle
	);

//
// �趨����̨���ڵĹ��λ�á�
//
STATUS
IoSetConsoleCursorPosition(
	IN HANDLE Handle,
	IN COORD CursorPosition
	);

#endif // _IO_
