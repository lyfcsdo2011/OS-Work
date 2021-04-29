/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: obhandle.c

����: ���������ʵ�֡�



*******************************************************************************/

#include "obp.h"
#include "ps.h"

//
// Ŀǰ������С�̶�Ϊһҳ�ڴ棬������ֵ�̶���
//
#define OB_MAX_HANDLE_VALUE ((HANDLE)(PAGE_SIZE/sizeof(HANDLE_TABLE_ENTRY) - 1))

PHANDLE_TABLE
ObAllocateHandleTable(
	VOID
	)
/*++

����������
	���䲢��ʼ��һ�����̾����

������
	�ޡ�

����ֵ��
	����ɹ��򷵻ؾ����ָ�룬���򷵻�NULL��

--*/
{
	STATUS Status;
	PHANDLE_TABLE ObjectTable;
	PHANDLE_TABLE_ENTRY HandleTable;
	SIZE_T TableSize;
	ULONG_PTR Index;

	//
	// ��ϵͳ�ڴ���з���һ�������
	//
	ObjectTable = (PHANDLE_TABLE)MmAllocateSystemPool(sizeof(HANDLE_TABLE));

	if (NULL != ObjectTable) {

		HandleTable = NULL;
		TableSize = PAGE_SIZE;

		//
		// Ŀǰ�����Ĵ�С�̶�Ϊһҳ�ڴ档
		//
		Status = MmAllocateVirtualMemory( (PVOID*)&HandleTable,
										  &TableSize,
										  MEM_RESERVE | MEM_COMMIT,
										  TRUE );

		if (EOS_SUCCESS(Status)) {

			//
			// ������0����������б����ʼ��Ϊһ���������������������ӡ�
			//
			for (Index = 1; Index < PAGE_SIZE/sizeof(HANDLE_TABLE_ENTRY) - 1; Index++) {
				HandleTable[Index].u.Next = Index + 1;
			}
			HandleTable[Index].u.Next = 0;

			ObjectTable->HandleTable = HandleTable;
			ObjectTable->FreeEntryListHead = 1;
			ObjectTable->HandleCount = 0;

		} else {

			MmFreeSystemPool(ObjectTable);

			ObjectTable = NULL;
		}
	}

	return ObjectTable;
}

VOID
ObFreeHandleTable(
	IN PHANDLE_TABLE ObjectTable
	)
/*++

����������
	�����ڽ��̽���ʱ�ͷŲ���ʹ�õĽ��̾����

������
	ObjectTable -- �����ͷŵľ�����ָ�롣

����ֵ��
	�ޡ�

--*/
{
	PVOID Ptr;
	SIZE_T Size;
	ULONG_PTR Index;

	//
	// �ر�������Ч�����
	//
	for (Index = 1; Index < PAGE_SIZE/sizeof(HANDLE_TABLE_ENTRY); Index++) {
		
		//
		// �ں˶���λ��2G֮�ϵ�ϵͳ��ַ�ռ䣬��Ч�Ķ���ָ������λӦΪ1��
		//
		Ptr = ObjectTable->HandleTable[Index].u.Object;

		if (((ULONG_PTR)Ptr & 0x80000000) != 0) {

			//
			// ��С��������������ü�������
			//
			OBJECT_TO_OBJECT_HEADER(Ptr)->HandleCount--;
			OBJECT_TO_OBJECT_TYPE(Ptr)->HandleCount--;
			ObDerefObject(Ptr);
		}
	}

	//
	// �ͷž����
	//
	Ptr = ObjectTable->HandleTable;
	Size = 0;
	MmFreeVirtualMemory(&Ptr, &Size, MEM_RELEASE, TRUE);
	MmFreeSystemPool(ObjectTable);
}

STATUS
ObAllocateHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	OUT PHANDLE Handle
	)
/*++

����������
	�ھ�����з���һ�����о���

������
	ObjectTable -- �����ָ�롣
	Handle -- ָ�룬ָ�����ڱ�����ֵ�ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	ULONG_PTR Index;

	IntState = KeEnableInterrupts(FALSE);

	if (0 == ObjectTable->FreeEntryListHead) {

		Status = STATUS_MAX_HANDLE_EXCEEDED;

	} else {

		//
		// �ӿ�����������ײ�ժȡһ�������ֵΪNULL��
		//
		Index = ObjectTable->FreeEntryListHead;
		ObjectTable->FreeEntryListHead = ObjectTable->HandleTable[Index].u.Next;
		ObjectTable->HandleTable[Index].u.Object = NULL;

		//
		// ���÷���ֵ��
		//
		*Handle = (HANDLE)Index;
		Status = STATUS_SUCCESS;
	}

	KeEnableInterrupts(IntState);

	return Status;
}

STATUS
ObFreeHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle
	)
/*++

����������
	�ͷſ��о���

������
	ObjectTable -- �����ָ�롣
	Handle -- ���о����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;

	if (NULL == Handle || Handle > OB_MAX_HANDLE_VALUE) {
		return STATUS_INVALID_HANDLE;
	}

	IntState = KeEnableInterrupts(FALSE);

	if (NULL == ObjectTable->HandleTable[(ULONG)Handle].u.Object) {

		//
		// ���վ������������������ײ���
		//
		ObjectTable->HandleTable[(ULONG_PTR)Handle].u.Next = ObjectTable->FreeEntryListHead;
		ObjectTable->FreeEntryListHead = (ULONG_PTR)Handle;

		Status = STATUS_SUCCESS;

	} else {

		Status = STATUS_INVALID_HANDLE;
	}

	KeEnableInterrupts(IntState);

	return Status;
}

STATUS
ObSetHandleValueEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle,
	IN PVOID Object
	)
/*++

����������
	���ÿ��о�����ֵ�������Ϊ��Ч�����

������
	ObjectTable -- ������ָ�롣
	Handle -- ���о����
	Object -- ������ֵ�����ں˶����ָ�롣

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;

	if (NULL == Handle || Handle > OB_MAX_HANDLE_VALUE) {
		return STATUS_INVALID_HANDLE;
	}

	//
	// �ں˶���λ��2G֮�ϵ�ϵͳ��ַ�ռ��У���Ч���ں˶���ָ������λӦ��Ϊ1��
	//
	if (((ULONG_PTR)Object & 0x80000000) == 0) {
		return STATUS_INVALID_PARAMETER;
	}

	IntState = KeEnableInterrupts(FALSE);

	if (NULL == ObjectTable->HandleTable[(ULONG)Handle].u.Object) {

		//
		// ���ÿվ�����ֵ�����Ӿ����������
		//
		ObjectTable->HandleTable[(ULONG_PTR)Handle].u.Object = Object;
		ObjectTable->HandleCount++;
		OBJECT_TO_OBJECT_HEADER(Object)->HandleCount++;
		OBJECT_TO_OBJECT_TYPE(Object)->HandleCount++;

		Status = STATUS_SUCCESS;

	} else {

		Status = STATUS_INVALID_HANDLE;
	}

	KeEnableInterrupts(IntState);

	return Status;
}

STATUS
ObCreateHandle(
	IN PVOID Object,
	OUT PHANDLE Handle
	)
/*++

����������
	�ڵ�ǰ���̵ľ������Ϊ���󴴽�һ�������

������
	Object -- ����ָ�롣
	Handle -- ָ�룬ָ�����ڱ������ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PHANDLE_TABLE ObjectTable;

	Status = PsGetObjectTable(CURRENT_PROCESS_HANDLE, (PVOID*)&ObjectTable);
	ASSERT(EOS_SUCCESS(Status));

	return ObCreateHandleEx(ObjectTable, Object, Handle);
}

STATUS
ObCreateHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN PVOID Object,
	OUT PHANDLE Handle
	)
/*++

����������
	��ָ���������Ϊ���󴴽�һ�������

������
	ObjectTable -- �����ָ�롣
	Object -- ����ָ�롣
	Handle -- ָ�룬ָ�����ڱ������ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;

	//
	// �ں˶���λ��2G֮�ϵ�ϵͳ��ַ�ռ��У���Ч���ں˶���ָ������λӦ��Ϊ1��
	//
	if (((ULONG_PTR)Object & 0x80000000) == 0) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = ObAllocateHandleEx(ObjectTable, Handle);

	if (EOS_SUCCESS(Status)) {
		Status = ObSetHandleValueEx(ObjectTable, *Handle, Object);
		ASSERT(EOS_SUCCESS(Status));
	}

	return Status;
}

STATUS
ObRefObjectByHandle(
	IN HANDLE Handle,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	)
/*++

����������
	�õ���ǰ���̾�����еľ�������Ķ����һ�����á�

������
	Handle -- ������еľ����
	ObjectType -- �����Ķ������ͣ���ѡ�����ΪNULL�򲻽���ƥ�䡣
	Object -- ָ�룬ָ�����ڱ������ָ��ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PVOID PointerObject;
	PHANDLE_TABLE ObjectTable;

	if (CURRENT_PROCESS_HANDLE == Handle) {
		PointerObject = PsGetCurrentProcessObject();
	} else if (CURRENT_THREAD_HANDLE == Handle) {
		PointerObject = PsGetCurrentThreadObject();
	} else {
		PointerObject = NULL;
	}

	if (PointerObject != NULL) {

		if (NULL == ObjectType || ObjectType == OBJECT_TO_OBJECT_TYPE(PointerObject)) {
			ObRefObject(PointerObject);
			*Object = PointerObject;
			return STATUS_SUCCESS;
		} else {
			return STATUS_OBJECT_TYPE_MISMATCH;
		}
	}

	Status = PsGetObjectTable(CURRENT_PROCESS_HANDLE, (PVOID*)&ObjectTable);
	ASSERT(EOS_SUCCESS(Status));

	return ObRefObjectByHandleEx(ObjectTable, Handle, ObjectType, Object);
}

STATUS
ObRefObjectByHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	)
/*++

����������
	�õ�ָ��������еľ�������Ķ����һ�����á�

������
	ObjectTable -- �����ָ�롣
	Handle -- ������еľ����
	ObjectType -- �����Ķ������ͣ���ѡ�����ΪNULL�򲻽���ƥ�䡣
	Object -- ָ�룬ָ�����ڱ������ָ��ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	PVOID PointerObject;

	if (NULL == Handle || Handle > OB_MAX_HANDLE_VALUE) {
		return STATUS_INVALID_HANDLE;
	}

	IntState = KeEnableInterrupts(FALSE);

	//
	// �ں˶���λ��2G֮�ϵ�ϵͳ��ַ�ռ��У���Ч���ں˶���ָ������λӦ��Ϊ1��
	//
	PointerObject = ObjectTable->HandleTable[(ULONG)Handle].u.Object;

	if (((ULONG_PTR)PointerObject & 0x80000000) != 0) {

		if (NULL == ObjectType || ObjectType == OBJECT_TO_OBJECT_TYPE(PointerObject)) {

			ObRefObject(PointerObject);

			*Object = PointerObject;
			Status = STATUS_SUCCESS;

		} else {

			Status = STATUS_OBJECT_TYPE_MISMATCH;
		}

	} else {

		Status = STATUS_INVALID_HANDLE;
	}

	KeEnableInterrupts(IntState);

	return Status;
}

STATUS
ObCloseHandle(
	IN HANDLE Handle
	)
/*++

����������
	�رյ�ǰ���̾�����еĶ�������

������
	Handle -- ��������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PHANDLE_TABLE ObjectTable;

	Status = PsGetObjectTable(CURRENT_PROCESS_HANDLE, (PVOID*)&ObjectTable);
	ASSERT(EOS_SUCCESS(Status));

	return ObCloseHandleEx(ObjectTable, Handle);
}

STATUS
ObCloseHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle
	)
/*++

����������
	�ر�ָ��������еĶ�������

������
	ObjectTable -- �����ָ�롣
	Handle -- ��������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	PVOID Object;

	if (NULL == Handle || Handle > OB_MAX_HANDLE_VALUE) {
		return STATUS_INVALID_HANDLE;
	}

	IntState = KeEnableInterrupts(FALSE);

	//
	// �ں˶���λ��2G֮�ϵ�ϵͳ��ַ�ռ��У���Ч���ں˶���ָ������λӦ��Ϊ1��
	//
	Object = ObjectTable->HandleTable[(ULONG)Handle].u.Object;

	if (((ULONG_PTR)Object & 0x80000000) != 0) {
		
		//
		// �ͷž������(������б���������ײ�)��
		//
		ObjectTable->HandleTable[(ULONG_PTR)Handle].u.Next = ObjectTable->FreeEntryListHead;
		ObjectTable->FreeEntryListHead = (ULONG_PTR)Handle;
		ObjectTable->HandleCount--;

		//
		// ��С��������������ü�������
		//
		OBJECT_TO_OBJECT_HEADER(Object)->HandleCount--;
		OBJECT_TO_OBJECT_TYPE(Object)->HandleCount--;
		ObDerefObject(Object);

		Status = STATUS_SUCCESS;

	} else {

		Status = STATUS_INVALID_HANDLE;
	}

	KeEnableInterrupts(IntState);

	return Status;
}
