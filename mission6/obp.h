/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: obp.h

����: �������ģ����ڲ�ͷ�ļ���



*******************************************************************************/

#ifndef _OBP_
#define _OBP_

#include "ke.h"
#include "mm.h"
#include "ob.h"

//
// �������ͽṹ�塣
//
typedef struct _OBJECT_TYPE{
	PSTR Name;							// ���������ַ���ָ��
	SINGLE_LIST_ENTRY TypeListEntry;	// �������������OBJECT_TYPE�ṹ�屻����������������
	LIST_ENTRY ObjectListHead;			// ��������ͷ���������ڴ����͵Ķ��󶼲������������
	ULONG ObjectCount;					// ������������������ڴ����͵Ķ��������
	ULONG HandleCount;					// ������������������ڴ����͵Ķ���ľ������
	OB_CREATE_METHOD Create;			// �����Ͷ���Ĺ��캯��ָ�룬��������ʱ������
	OB_DELETE_METHOD Delete;			// �����Ͷ������������ָ�룬ɾ������ʱ������
	OB_WAIT_METHOD Wait;				// �����Ͷ����WaitForSingleObject����ָ��
	OB_READ_METHOD Read;				// �����Ͷ����ReadFile����ָ��
	OB_WRITE_METHOD Write;				// �����Ͷ����WriteFile����ָ��
} OBJECT_TYPE;

//
// ����ͷ�ṹ�塣
//
typedef struct _OBJECT_HEADER {
	PSTR Name;							// ���������ַ���ָ��
	ULONG Id;							// ����ID��ȫ��Ψһ
	POBJECT_TYPE Type;					// ��������ָ��
	ULONG PointerCount;					// �������ü�����
	ULONG HandleCount;					// ������������
	LIST_ENTRY TypeObjectListEntry;		// �������͵Ķ���������
	LIST_ENTRY GlobalObjectListEntry;	// ȫ�ֵĶ���������
	QUAD Body;							// ���������ʼ��
} OBJECT_HEADER, *POBJECT_HEADER;

//
// �ɶ�����ָ��õ�����ͷָ��ĺꡣ
//
#define OBJECT_TO_OBJECT_HEADER( o )\
	CONTAINING_RECORD( (o), OBJECT_HEADER, Body )

//
// �õ���������ָ��ĺꡣ
//
#define OBJECT_TO_OBJECT_TYPE( o )\
	( OBJECT_TO_OBJECT_HEADER( o )->Type )

//
// �������ṹ�壬Ŀǰ����һ���򣺶���ָ�롣
//
typedef struct _HANDLE_TABLE_ENTRY {
	union {
		PVOID Object;
		ULONG_PTR Next;
	} u;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

//
// ���̾����ṹ�塣
//
typedef struct _HANDLE_TABLE {
	PHANDLE_TABLE_ENTRY HandleTable;
	ULONG_PTR FreeEntryListHead;
	ULONG_PTR HandleCount;
} HANDLE_TABLE;

//
// �����������Ʋ�������
//
POBJECT_TYPE
ObpLookupObjectTypeByName(
	IN PCSTR TypeName
	);

//
// ���ݶ������ͺͶ������Ʋ��Ҷ���
//
PVOID
ObpLookupObjectByName(
	IN POBJECT_TYPE ObjectType,
	IN PCSTR ObjectName
	);

#endif // _OBP_
