/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: object.c

����: �������ģ���ʵ�֡�



*******************************************************************************/

#include "obp.h"

//
// ȫ�ֶ�������ͷ
//
LIST_ENTRY ObpObjectListHead = {&ObpObjectListHead, &ObpObjectListHead}; 

//
// ȫ�ֶ��������
//
ULONG ObpObjectCount = 0;

//
// ȫ�ֶ���ID������
//
ULONG ObpObjectIdAllocator = 1; 


STATUS
ObCreateObject(
	IN POBJECT_TYPE ObjectType,
	IN PCSTR ObjectName OPTIONAL,
	IN SIZE_T ObjectBodySize,
	IN ULONG_PTR CreateParam,
	OUT PVOID *Object
	)
/*++

����������
	�������һ��ָ�����͵Ķ���������������Ѿ���������������󣬷��򴴽���
	����������������Զ��������

������
	ObjectType -- ����������򿪵Ķ�������͡�
	ObjectName -- ���������ַ���ָ�룬����Ϊ�����򿪻򴴽���������������ơ����
		ΪNULL��ֱ�Ӵ�����������
	ObjectBodySize -- ���������Ķ���ĳߴ硣
	CreateParam -- ���ݸ������캯���Ĳ�����
	Object -- ���ش�����򿪵Ķ����ָ�롣

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	BOOL IntState;
	PCHAR NameBuffer;
	SIZE_T ObjectSize;
	SIZE_T NameBufferSize;
	POBJECT_HEADER ObjectHeader;

	ASSERT(ObjectType != NULL && Object != NULL);

	IntState = KeEnableInterrupts(FALSE);
		
	if(NULL != ObjectName) {

		//
		// ������������Ѿ�������ֱ�Ӵ���������
		//
		if (EOS_SUCCESS(ObRefObjectByName(ObjectName, ObjectType, Object))) {

			KeEnableInterrupts(IntState);

			return STATUS_OBJECT_NAME_EXISTS;
		}

		//
		// ����������ַ����������Ĵ�С��
		//
		NameBufferSize = ROUND_TO_QUAD(strlen(ObjectName) + 1);

	} else {

		NameBufferSize = 0;
	}

	//
	// �������ͷ�Ͷ�����ĳߴ�͡�
	//
	if (ObjectBodySize >= sizeof(QUAD)) {
		ObjectSize = sizeof(OBJECT_HEADER) - sizeof(QUAD) + ObjectBodySize;
	} else {
		ObjectSize = sizeof(OBJECT_HEADER);
	}

	//
	// ��ϵͳ�ڴ����Ϊ������������ͷ�Ͷ��������һ�����ڴ档
	//
	NameBuffer = (PCHAR)MmAllocateSystemPool(NameBufferSize + ObjectSize);
	
	if(NULL == NameBuffer) {

		KeEnableInterrupts(IntState);

		return STATUS_NO_MEMORY;
	}

	ObjectHeader = (POBJECT_HEADER)(NameBuffer + NameBufferSize);

	//
	// ��ʼ������ͷ��
	//
	ObjectHeader->Type = ObjectType;
	ObjectHeader->Id = ObpObjectIdAllocator++;
	ObjectHeader->PointerCount = 1;

	if(NULL != ObjectName) {
		strcpy(NameBuffer, ObjectName);
		ObjectHeader->Name = NameBuffer;
	} else {
		ObjectHeader->Name = NULL;
	}

	//
	// �������Ͷ��������ȫ�ֶ�������
	//
	ListInsertTail(&ObjectType->ObjectListHead, &ObjectHeader->TypeObjectListEntry);
	ObjectType->ObjectCount++;

	ListInsertTail(&ObpObjectListHead, &ObjectHeader->GlobalObjectListEntry);
	ObpObjectCount++;

	//
	// ���ù��캯����
	//
	if (NULL != ObjectType->Create) {
		ObjectType->Create((PVOID)&ObjectHeader->Body, CreateParam);
	}

	//
	// ���ض�����ָ�롣
	//
	*Object = (PVOID)&ObjectHeader->Body;

	KeEnableInterrupts(IntState);

	return STATUS_SUCCESS;
}

ULONG
ObGetObjectId(
	IN PVOID Object
	)
/*++

����������
	�õ����� Id��

������
	Object - ������ָ�롣

����ֵ��
	���� Id��

--*/
{
	return OBJECT_TO_OBJECT_HEADER(Object)->Id;
}

VOID
ObRefObject(
	IN PVOID Object
	)
/*++

����������
	����һ�ݶ�������á�

������
	Object -- ����ָ�롣

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState = KeEnableInterrupts(FALSE);

	ASSERT(OBJECT_TO_OBJECT_HEADER(Object)->PointerCount > 0);

	OBJECT_TO_OBJECT_HEADER(Object)->PointerCount++;

	KeEnableInterrupts(IntState);
}

STATUS
ObRefObjectByName(
	IN PCSTR ObjectName,
	IN POBJECT_TYPE ObjectType,
	OUT PVOID *Object
	)
/*++

����������
	�ɶ������ƺ����͵õ������һ�����á�

������
	ObjectName -- ���������ַ���ָ�롣
	ObjectType -- ��������ָ�롣
	Object -- ָ�룬ָ�����ڱ������ָ��ı�����

����ֵ��
	����ɹ������� STATUS_SUCCESS��

--*/
{
	BOOL IntState;
	PLIST_ENTRY ListEntry;
	POBJECT_HEADER ObjectHeader;

	ASSERT(NULL != ObjectType && NULL != ObjectName && NULL != Object);

	if (NULL == ObjectType || NULL == ObjectName || NULL == Object) {
		return STATUS_INVALID_PARAMETER;
	}
	
	IntState = KeEnableInterrupts(FALSE);

	//
	// ����ָ�����͵Ķ�����������ָ�����ƵĶ���
	//
	for (ListEntry = ObjectType->ObjectListHead.Next;
		ListEntry !=  &ObjectType->ObjectListHead;
		ListEntry = ListEntry->Next)
	{
		ObjectHeader = CONTAINING_RECORD(ListEntry, OBJECT_HEADER, TypeObjectListEntry);

		if (ObjectHeader->Name != NULL && 0 == stricmp(ObjectHeader->Name, ObjectName)) {
			
			*Object = (PVOID)&ObjectHeader->Body;
			ObjectHeader->PointerCount++;

			KeEnableInterrupts(IntState);
			
			return STATUS_SUCCESS;
		}
	}

	KeEnableInterrupts(IntState);
	
	return STATUS_OBJECT_NAME_NOT_FOUND;
}

STATUS
ObRefObjectById(
	IN ULONG Id,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	)
/*++

����������
	�ɶ���Id�õ������һ�����á�

������
	Id -- �����򿪵Ķ����Id��
	ObjectType -- �����Ķ������ͣ���ѡ�����ΪNULL�򲻽���ƥ�䡣
	Object -- ָ�룬ָ�����ڱ������ָ��ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	POBJECT_HEADER ObjectHeader;
	BOOL IntState;

	ASSERT(0 != Id && NULL != Object);

	if (0 == Id || NULL == Object) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ���ָ�������ͣ��������Ͷ��������в��ң�������ȫ�ֶ��������в��ҡ�
	//
	if (ObjectType != NULL) {
		ListHead = &ObjectType->ObjectListHead;
	} else {
		ListHead = &ObpObjectListHead;
	}

	IntState = KeEnableInterrupts(FALSE);

	for (ListEntry = ListHead->Next; ListEntry != ListHead; ListEntry = ListEntry->Next) {

		ObjectHeader = CONTAINING_RECORD(ListEntry, OBJECT_HEADER, TypeObjectListEntry);

		if (ObjectHeader->Id == Id) {

			*Object = &ObjectHeader->Body;
			ObjectHeader->PointerCount++;

			KeEnableInterrupts(IntState);
			
			return STATUS_SUCCESS;
		}
	}
	
	KeEnableInterrupts(IntState);

	return STATUS_OBJECT_ID_NOT_FOUND;
}

VOID
ObDerefObject(
	IN PVOID Object
	)
/*++

����������
	�ͷ�һ�ݶ�������á��������ü�������С1�������Ϊ0��ɾ������

������
	Object -- ����ָ�롣

����ֵ��
	�ޡ�

--*/
{
	POBJECT_HEADER ObjectHeader;
	BOOL IntState;

	ASSERT(NULL != Object);
	
	IntState = KeEnableInterrupts(FALSE);

	ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

	//
	// ��С�������ü������������Ϊ0��
	//	1. ���ö����Delete������
	//	2. ����������Ͷ��������ȫ�ֶ����������Ƴ���
	//	3. �ͷŶ���ռ�õ��ڴ档
	//
	if(0 == --ObjectHeader->PointerCount) {

		if(NULL != ObjectHeader->Type->Delete){
			ObjectHeader->Type->Delete(Object);
		}

		ListRemoveEntry(&ObjectHeader->TypeObjectListEntry);
		ObjectHeader->Type->ObjectCount--;
		ObjectHeader->Type = NULL;

		ListRemoveEntry(&ObjectHeader->GlobalObjectListEntry);
		ObpObjectCount--;

		//
		// �ͷŶ���ռ�õ��ڴ档ע�⣬����ͷ��֮ǰ�����п�ѡ�Ķ������ַ�����
		//
		if (NULL != ObjectHeader->Name) {
			MmFreeSystemPool(ObjectHeader->Name);
		} else {
			MmFreeSystemPool(ObjectHeader);
		}
	}

	KeEnableInterrupts(IntState);
}

ULONG
ObGetAccessMask(
	IN PVOID Object
	)
/*++

����������
	�õ��������Ч�������롣

������
	Object -- �����ָ�롣

����ֵ��
	����Ŀɷ��ʷ��������롣

--*/
{
	POBJECT_TYPE Type;
	ULONG AccessMask;

	Type = OBJECT_TO_OBJECT_TYPE(Object);
	AccessMask = 0;

	if (Type->Wait != NULL) {
		AccessMask |= OB_WAIT_ACCESS;
	}

	if (Type->Read != NULL) {
		AccessMask |= OB_READ_ACCESS;
	}

	if (Type->Write != NULL) {
		AccessMask |= OB_WRITE_ACCESS;
	}

	return AccessMask;
}

POBJECT_HEADER PGlobalObj[30];
PSTR NameArr[30];

VOID
ObpGlobalObjListEntry()
{
	POBJECT_HEADER ObjectHeader;
	PLIST_ENTRY ListEntry;
	INT Num = 0;

	//
	// ����ȫ�ֶ�������
	//
	for (ListEntry = ObpObjectListHead.Next;
		ListEntry != &ObpObjectListHead;
		ListEntry = ListEntry->Next) {

		ObjectHeader = CONTAINING_RECORD(ListEntry, OBJECT_HEADER, GlobalObjectListEntry);

		PGlobalObj[Num] = ObjectHeader;
		
		Num++;
	}
}
