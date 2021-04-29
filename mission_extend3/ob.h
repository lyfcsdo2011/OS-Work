/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: ob.h

����: ��������������ӿ�ͷ�ļ���



*******************************************************************************/

#ifndef _OB_
#define _OB_

#include "eosdef.h"

#define OB_WAIT_ACCESS		0x00000001
#define OB_READ_ACCESS		0x00000002
#define OB_WRITE_ACCESS		0x00000004

//
// ��������ָ������
//
typedef struct _OBJECT_TYPE *POBJECT_TYPE;

//
// �����ָ������
//
typedef struct _HANDLE_TABLE *PHANDLE_TABLE;

//
// �������͵��鷽���Ľӿڶ��塣
//
typedef
VOID
(*OB_CREATE_METHOD)(
	IN PVOID Object,
	IN ULONG_PTR Param
	);

typedef
VOID
(*OB_DELETE_METHOD)(
	IN PVOID Object
	);

typedef
STATUS
(*OB_WAIT_METHOD)(
	IN PVOID Object,
	IN ULONG Milliseconds
	);

typedef 
STATUS
(*OB_READ_METHOD)(
	IN PVOID Object,
	OUT PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	);

typedef
STATUS
(*OB_WRITE_METHOD)(
	IN PVOID Object,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);

//
// �������ͳ�ʼ���Ĳ����ṹ�塣
//
typedef struct _OBJECT_TYPE_INITIALIZER {
	OB_CREATE_METHOD Create;
	OB_DELETE_METHOD Delete;
	OB_WAIT_METHOD Wait;
	OB_READ_METHOD Read;
	OB_WRITE_METHOD Write;
}OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

//
// ��ʼ���������ģ���һ����
//
VOID
ObInitializeSystem1(
	VOID
	);

//
// ��ʼ���������ģ��ڶ�����
//
VOID
ObInitializeSystem2(
	VOID
	);

//
// ����һ���������͡�
//
STATUS
ObCreateObjectType(
	IN PCSTR TypeName,
	IN POBJECT_TYPE_INITIALIZER Initializer,
	OUT POBJECT_TYPE *ObjectType
	);

//
// ����һ��ָ�����͵Ķ���
//
STATUS
ObCreateObject(
	IN POBJECT_TYPE ObjectType,
	IN PCSTR ObjectName,
	IN SIZE_T ObjectBodySize,
	IN ULONG_PTR CreateParam,
	OUT PVOID *Object
	);

//
// �õ������ID
//
ULONG
ObGetObjectId(
	IN PVOID Object
	);

//
// ����һ�ݶ�������á�
//
VOID
ObRefObject(
	IN PVOID Object
	);

//
// �ɶ������ƺ����͵õ������һ�����á�
//
STATUS
ObRefObjectByName(
	IN PCSTR ObjectName,
	IN POBJECT_TYPE ObjectType,
	OUT PVOID *Object
	);

//
// �ɶ���Id�õ������һ�����á�
//
STATUS
ObRefObjectById(
	IN ULONG Id,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	);

//
// �ͷŶ����һ�����á�
//
VOID
ObDerefObject(
	IN PVOID Object
	);

//
// �õ��������Ч�������롣
//
ULONG
ObGetAccessMask(
	IN PVOID Object
	);

//
// ����һ�������
//
PHANDLE_TABLE
ObAllocateHandleTable(
	VOID
	);

//
// �ͷž����
//
VOID
ObFreeHandleTable(
	IN PHANDLE_TABLE ObjectTable
	);

//
// �ھ�����з���һ���վ����
//
STATUS
ObAllocateHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	OUT PHANDLE Handle
	);

//
// �ͷſվ����
//
STATUS
ObFreeHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle
	);

//
// ���ÿվ����ֵ��
//
STATUS
ObSetHandleValueEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle,
	IN PVOID Object
	);

//
// �ڵ�ǰ���̵ľ������Ϊ���󴴽�һ�������
//
STATUS
ObCreateHandle(
	IN PVOID Object,
	OUT PHANDLE Handle
	);

//
// ��ָ���������Ϊ���󴴽�һ�������
//
STATUS
ObCreateHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN PVOID Object,
	OUT PHANDLE Handle
	);

//
// �õ�����ڵ�ǰ���̾�����������������һ�����á�
//
STATUS
ObRefObjectByHandle(
	IN HANDLE Handle,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	);

//
// �õ������ָ��������������������һ�����á�
//
STATUS
ObRefObjectByHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	);

//
// �رյ�ǰ���̾�����еĶ�������
//
STATUS
ObCloseHandle(
	IN HANDLE Handle
	);

//
// �ر�ָ��������еĶ�������
//
STATUS
ObCloseHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle
	);

//
// ���ö����Wait�麯����
//
STATUS
ObWaitForObject(
	IN HANDLE Handle,
	IN ULONG Milliseconds
	);

//
// ���ö����Read�麯����
//
STATUS
ObRead(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	);

//
// ���ö����Write�麯����
//
STATUS
ObWrite(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);

#endif // _OB_
