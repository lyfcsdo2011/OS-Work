/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: obtype.c

����: �������͹���ģ���ʵ�֡�



*******************************************************************************/

#include "obp.h"

//
// ������������ͷ��
//
SINGLE_LIST_ENTRY ObpTypeListHead = {NULL};

//
// ���ͼ�������
//
ULONG ObpTypeCount = 0;


STATUS
ObCreateObjectType(
	IN PCSTR TypeName,
	IN POBJECT_TYPE_INITIALIZER Initializer,
	OUT POBJECT_TYPE *ObjectType
	)
/*++

����������
	����һ���������͡�

������
	TypeName -- ���������ַ���ָ�롣
	Initializer -- �����½����ͳ�ʼ���Ľṹ��ָ�룬���������͵��麯��ָ�롣
	ObjectType -- ָ����������������͵Ļ�������ָ�롣

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS�������ʾʧ�ܡ�

--*/
{
	STATUS Status;
	BOOL IntState;
	POBJECT_TYPE Type;

	if (NULL == TypeName || NULL == ObjectType) {

		ASSERT(FALSE);

		return STATUS_INVALID_PARAMETER;
	}
	
	IntState = KeEnableInterrupts(FALSE);

	if (NULL != ObpLookupObjectTypeByName(TypeName)) {

		ASSERT(FALSE);

		Status = STATUS_OBJECT_NAME_COLLISION;

	} else {

		//
		// ��ϵͳ�ڴ����Ϊ���ͽṹ������������ַ�������ռ䡣
		//
		Type = (POBJECT_TYPE)MmAllocateSystemPool(sizeof(OBJECT_TYPE) + strlen(TypeName) + 1);

		if (NULL != Type) {

			//
			// ���������ַ������������ͽṹ��ĺ��档
			//
			Type->Name = (PCHAR)Type + sizeof(OBJECT_TYPE);
			strcpy(Type->Name, TypeName);

			//
			// ��ʼ�����͵Ķ������������������
			//
			ListInitializeHead(&Type->ObjectListHead);
			Type->ObjectCount = 0;
			Type->HandleCount = 0;

			//
			// ��ʼ�����͵ĺ���ָ�롣
			//
			Type->Create = Initializer->Create;
			Type->Delete = Initializer->Delete;
			Type->Wait = Initializer->Wait;
			Type->Read = Initializer->Read;
			Type->Write = Initializer->Write;

			//
			// �����Ͳ������������в��������ͼ�������
			//
			SListPushEntry(&ObpTypeListHead, &Type->TypeListEntry);
			ObpTypeCount++;

			//
			// �����´��������͡�
			//
			*ObjectType = Type;

			Status = STATUS_SUCCESS;

		} else {

			Status = STATUS_NO_MEMORY;
		}
	}

	KeEnableInterrupts(IntState);

	return Status;
}

POBJECT_TYPE
ObpLookupObjectTypeByName(
	IN PCSTR TypeName
	)
/*++

����������
	�����������Ʋ��ҵõ�����ָ�롣

������
	TypeName -- �������ơ�

����ֵ��
	��������򷵻�����ָ�룬���򷵻� NULL��

--*/
{
	POBJECT_TYPE ObjectType;
	PSINGLE_LIST_ENTRY ListEntry;

	//
	// ����������������ָ�����Ƶ����͡�
	//
	for (ListEntry = ObpTypeListHead.Next;
		ListEntry != NULL;
		ListEntry = ListEntry->Next) {

		ObjectType = CONTAINING_RECORD(ListEntry, OBJECT_TYPE, TypeListEntry);

		if (0 == strcmp(ObjectType->Name, TypeName)) {
			return ObjectType;
		}
	}

	return NULL;
}

PVOID
ObpLookupObjectByName(
	IN POBJECT_TYPE ObjectType,
	IN PCSTR ObjectName
	)
/*++

����������
	���ݶ������ͺͶ������Ʋ��Ҷ���

������
	ObjectType -- ��������ָ�롣
	ObjectName -- �������ơ�

����ֵ��
	��������򷵻ض���ָ�룬���򷵻� NULL��

--*/
{
	POBJECT_HEADER ObjectHeader;
	PLIST_ENTRY ListEntry;

	//
	// �������������еĶ�����������ָ�����ƵĶ���
	//
	for (ListEntry = ObjectType->ObjectListHead.Next;
		ListEntry != &ObjectType->ObjectListHead;
		ListEntry = ListEntry->Next) {

		ObjectHeader = CONTAINING_RECORD(ListEntry, OBJECT_HEADER, TypeObjectListEntry);

		if (0 == strcmp(ObjectHeader->Name, ObjectName)) {
		
			//
			// ���ض���ͷ�еĶ����塣
			//
			return (PVOID)&ObjectHeader->Body;
		}
	}
	
	return NULL;
}

POBJECT_TYPE PObjType[20];
POBJECT_HEADER ObjectHeaderArr[10][10];
VOID
ObpAllObjectTypeName()
/*++

����������
	�õ����ж�������ơ�

������


����ֵ��
	

--*/
{
	POBJECT_TYPE ObjectType;
	PSINGLE_LIST_ENTRY ListEntry;
	INT Num = 0;

	//
	// ����������������ָ�����Ƶ����͡�
	//
	for (ListEntry = ObpTypeListHead.Next;
		ListEntry != NULL;
		ListEntry = ListEntry->Next) {

		ObjectType = CONTAINING_RECORD(ListEntry, OBJECT_TYPE, TypeListEntry);

		PObjType[Num] = ObjectType;
		
		PSINGLE_LIST_ENTRY InListEntry;
		POBJECT_HEADER ObjectHeader;
		
		INT InNum = 0;
		for(InListEntry = (PSINGLE_LIST_ENTRY)(ObjectType->ObjectListHead.Next);
		InListEntry != (PSINGLE_LIST_ENTRY)(&ObjectType->ObjectListHead);
		InListEntry = InListEntry->Next)
		{
			ObjectHeader = CONTAINING_RECORD(InListEntry, OBJECT_HEADER, TypeObjectListEntry);
			ObjectHeaderArr[Num][InNum] = ObjectHeader;
			InNum++;
		}
		Num++;
	}
}
