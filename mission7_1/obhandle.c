/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: obhandle.c

描述: 对象句柄表的实现。



*******************************************************************************/

#include "obp.h"
#include "ps.h"

//
// 目前句柄表大小固定为一页内存，句柄最大值固定。
//
#define OB_MAX_HANDLE_VALUE ((HANDLE)(PAGE_SIZE/sizeof(HANDLE_TABLE_ENTRY) - 1))

PHANDLE_TABLE
ObAllocateHandleTable(
	VOID
	)
/*++

功能描述：
	分配并初始化一个进程句柄表。

参数：
	无。

返回值：
	如果成功则返回句柄表指针，否则返回NULL。

--*/
{
	STATUS Status;
	PHANDLE_TABLE ObjectTable;
	PHANDLE_TABLE_ENTRY HandleTable;
	SIZE_T TableSize;
	ULONG_PTR Index;

	//
	// 从系统内存池中分配一个句柄表。
	//
	ObjectTable = (PHANDLE_TABLE)MmAllocateSystemPool(sizeof(HANDLE_TABLE));

	if (NULL != ObjectTable) {

		HandleTable = NULL;
		TableSize = PAGE_SIZE;

		//
		// 目前句柄表的大小固定为一页内存。
		//
		Status = MmAllocateVirtualMemory( (PVOID*)&HandleTable,
										  &TableSize,
										  MEM_RESERVE | MEM_COMMIT,
										  TRUE );

		if (EOS_SUCCESS(Status)) {

			//
			// 将除第0项以外的所有表项初始化为一个链表，按照索引进行链接。
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

功能描述：
	用于在进程结束时释放不再使用的进程句柄表。

参数：
	ObjectTable -- 期望释放的句柄表的指针。

返回值：
	无。

--*/
{
	PVOID Ptr;
	SIZE_T Size;
	ULONG_PTR Index;

	//
	// 关闭所有有效句柄。
	//
	for (Index = 1; Index < PAGE_SIZE/sizeof(HANDLE_TABLE_ENTRY); Index++) {
		
		//
		// 内核对象位于2G之上的系统地址空间，有效的对象指针的最高位应为1。
		//
		Ptr = ObjectTable->HandleTable[Index].u.Object;

		if (((ULONG_PTR)Ptr & 0x80000000) != 0) {

			//
			// 减小句柄计数器和引用计数器。
			//
			OBJECT_TO_OBJECT_HEADER(Ptr)->HandleCount--;
			OBJECT_TO_OBJECT_TYPE(Ptr)->HandleCount--;
			ObDerefObject(Ptr);
		}
	}

	//
	// 释放句柄表。
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

功能描述：
	在句柄表中分配一个空闲句柄项。

参数：
	ObjectTable -- 句柄表指针。
	Handle -- 指针，指向用于保存句柄值的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
		// 从空闲项链表的首部摘取一项并设置其值为NULL。
		//
		Index = ObjectTable->FreeEntryListHead;
		ObjectTable->FreeEntryListHead = ObjectTable->HandleTable[Index].u.Next;
		ObjectTable->HandleTable[Index].u.Object = NULL;

		//
		// 设置返回值。
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

功能描述：
	释放空闲句柄项。

参数：
	ObjectTable -- 句柄表指针。
	Handle -- 空闲句柄。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
		// 将空句柄项插入空闲项链表的首部。
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

功能描述：
	设置空闲句柄项的值，句柄变为有效句柄。

参数：
	ObjectTable -- 句柄表的指针。
	Handle -- 空闲句柄。
	Object -- 句柄项的值——内核对象的指针。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	BOOL IntState;

	if (NULL == Handle || Handle > OB_MAX_HANDLE_VALUE) {
		return STATUS_INVALID_HANDLE;
	}

	//
	// 内核对象位于2G之上的系统地址空间中，有效的内核对象指针的最高位应该为1。
	//
	if (((ULONG_PTR)Object & 0x80000000) == 0) {
		return STATUS_INVALID_PARAMETER;
	}

	IntState = KeEnableInterrupts(FALSE);

	if (NULL == ObjectTable->HandleTable[(ULONG)Handle].u.Object) {

		//
		// 设置空句柄项的值并增加句柄计数器。
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

功能描述：
	在当前进程的句柄表中为对象创建一个句柄。

参数：
	Object -- 对象指针。
	Handle -- 指针，指向用于保存句柄的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

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

功能描述：
	在指定句柄表中为对象创建一个句柄。

参数：
	ObjectTable -- 句柄表指针。
	Object -- 对象指针。
	Handle -- 指针，指向用于保存句柄的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;

	//
	// 内核对象位于2G之上的系统地址空间中，有效的内核对象指针的最高位应该为1。
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

功能描述：
	得到当前进程句柄表中的句柄关联的对象的一份引用。

参数：
	Handle -- 句柄表中的句柄。
	ObjectType -- 期望的对象类型，可选，如果为NULL则不进行匹配。
	Object -- 指针，指向用于保存对象指针的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

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

功能描述：
	得到指定句柄表中的句柄关联的对象的一份引用。

参数：
	ObjectTable -- 句柄表指针。
	Handle -- 句柄表中的句柄。
	ObjectType -- 期望的对象类型，可选，如果为NULL则不进行匹配。
	Object -- 指针，指向用于保存对象指针的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
	// 内核对象位于2G之上的系统地址空间中，有效的内核对象指针的最高位应该为1。
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

功能描述：
	关闭当前进程句柄表中的对象句柄。

参数：
	Handle -- 对象句柄。

返回值：
	如果成功则返回STATUS_SUCCESS。

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

功能描述：
	关闭指定句柄表中的对象句柄。

参数：
	ObjectTable -- 句柄表指针。
	Handle -- 对象句柄。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
	// 内核对象位于2G之上的系统地址空间中，有效的内核对象指针的最高位应该为1。
	//
	Object = ObjectTable->HandleTable[(ULONG)Handle].u.Object;

	if (((ULONG_PTR)Object & 0x80000000) != 0) {
		
		//
		// 释放句柄表项(插入空闲表项链表的首部)。
		//
		ObjectTable->HandleTable[(ULONG_PTR)Handle].u.Next = ObjectTable->FreeEntryListHead;
		ObjectTable->FreeEntryListHead = (ULONG_PTR)Handle;
		ObjectTable->HandleCount--;

		//
		// 减小句柄计数器和引用计数器。
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
