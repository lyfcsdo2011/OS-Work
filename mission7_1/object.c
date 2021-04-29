/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: object.c

描述: 对象管理模块的实现。



*******************************************************************************/

#include "obp.h"

//
// 全局对象链表头
//
LIST_ENTRY ObpObjectListHead = {&ObpObjectListHead, &ObpObjectListHead}; 

//
// 全局对象计数器
//
ULONG ObpObjectCount = 0;

//
// 全局对象ID分配器
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

功能描述：
	创建或打开一个指定类型的对象。如果命名对象已经存在则打开命名对象，否则创建命
	名对象。匿名对象永远被创建。

参数：
	ObjectType -- 期望创建或打开的对象的类型。
	ObjectName -- 对象名称字符串指针，名称为期望打开或创建的命名对象的名称。如果
		为NULL则直接创建匿名对象。
	ObjectBodySize -- 期望创建的对象的尺寸。
	CreateParam -- 传递给对象构造函数的参数。
	Object -- 返回创建或打开的对象的指针。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
		// 如果命名对象已经存在则直接打开命名对象。
		//
		if (EOS_SUCCESS(ObRefObjectByName(ObjectName, ObjectType, Object))) {

			KeEnableInterrupts(IntState);

			return STATUS_OBJECT_NAME_EXISTS;
		}

		//
		// 计算对象名字符串缓冲区的大小。
		//
		NameBufferSize = ROUND_TO_QUAD(strlen(ObjectName) + 1);

	} else {

		NameBufferSize = 0;
	}

	//
	// 计算对象头和对象体的尺寸和。
	//
	if (ObjectBodySize >= sizeof(QUAD)) {
		ObjectSize = sizeof(OBJECT_HEADER) - sizeof(QUAD) + ObjectBodySize;
	} else {
		ObjectSize = sizeof(OBJECT_HEADER);
	}

	//
	// 从系统内存池中为对象名、对象头和对象体分配一整块内存。
	//
	NameBuffer = (PCHAR)MmAllocateSystemPool(NameBufferSize + ObjectSize);
	
	if(NULL == NameBuffer) {

		KeEnableInterrupts(IntState);

		return STATUS_NO_MEMORY;
	}

	ObjectHeader = (POBJECT_HEADER)(NameBuffer + NameBufferSize);

	//
	// 初始化对象头。
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
	// 插入类型对象链表和全局对象链表。
	//
	ListInsertTail(&ObjectType->ObjectListHead, &ObjectHeader->TypeObjectListEntry);
	ObjectType->ObjectCount++;

	ListInsertTail(&ObpObjectListHead, &ObjectHeader->GlobalObjectListEntry);
	ObpObjectCount++;

	//
	// 调用构造函数。
	//
	if (NULL != ObjectType->Create) {
		ObjectType->Create((PVOID)&ObjectHeader->Body, CreateParam);
	}

	//
	// 返回对象体指针。
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

功能描述：
	得到对象 Id。

参数：
	Object - 对象体指针。

返回值：
	对象 Id。

--*/
{
	return OBJECT_TO_OBJECT_HEADER(Object)->Id;
}

VOID
ObRefObject(
	IN PVOID Object
	)
/*++

功能描述：
	复制一份对象的引用。

参数：
	Object -- 对象指针。

返回值：
	无。

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

功能描述：
	由对象名称和类型得到对象的一份引用。

参数：
	ObjectName -- 对象名称字符串指针。
	ObjectType -- 对象类型指针。
	Object -- 指针，指向用于保存对象指针的变量。

返回值：
	如果成功，返回 STATUS_SUCCESS。

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
	// 遍历指定类型的对象链表，查找指定名称的对象。
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

功能描述：
	由对象Id得到对象的一份引用。

参数：
	Id -- 期望打开的对象的Id。
	ObjectType -- 期望的对象类型，可选，如果为NULL则不进行匹配。
	Object -- 指针，指向用于保存对象指针的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
	// 如果指定了类型，则在类型对象链表中查找，否则在全局对象链表中查找。
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

功能描述：
	释放一份对象的引用。对象引用计数器减小1，如果变为0则删除对象。

参数：
	Object -- 对象指针。

返回值：
	无。

--*/
{
	POBJECT_HEADER ObjectHeader;
	BOOL IntState;

	ASSERT(NULL != Object);
	
	IntState = KeEnableInterrupts(FALSE);

	ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

	//
	// 减小对象引用计数器，如果变为0则：
	//	1. 调用对象的Delete方法；
	//	2. 将对象从类型对象链表和全局对象链表中移除；
	//	3. 释放对象占用的内存。
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
		// 释放对象占用的内存。注意，对象头部之前可能有可选的对象名字符串。
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

功能描述：
	得到对象的有效访问掩码。

参数：
	Object -- 对象的指针。

返回值：
	对象的可访问方法的掩码。

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
	// 遍历全局对象链表。
	//
	for (ListEntry = ObpObjectListHead.Next;
		ListEntry != &ObpObjectListHead;
		ListEntry = ListEntry->Next) {

		ObjectHeader = CONTAINING_RECORD(ListEntry, OBJECT_HEADER, GlobalObjectListEntry);

		PGlobalObj[Num] = ObjectHeader;
		
		Num++;
	}
}
