/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: obtype.c

描述: 对象类型管理模块的实现。



*******************************************************************************/

#include "obp.h"

//
// 对象类型链表头。
//
SINGLE_LIST_ENTRY ObpTypeListHead = {NULL};

//
// 类型计数器。
//
ULONG ObpTypeCount = 0;


STATUS
ObCreateObjectType(
	IN PCSTR TypeName,
	IN POBJECT_TYPE_INITIALIZER Initializer,
	OUT POBJECT_TYPE *ObjectType
	)
/*++

功能描述：
	创建一个对象类型。

参数：
	TypeName -- 类型名称字符串指针。
	Initializer -- 用于新建类型初始化的结构体指针，包含了类型的虚函数指针。
	ObjectType -- 指向用于输出对象类型的缓冲区的指针。

返回值：
	如果成功则返回STATUS_SUCCESS，否则表示失败。

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
		// 在系统内存池中为类型结构体和类型名称字符串分配空间。
		//
		Type = (POBJECT_TYPE)MmAllocateSystemPool(sizeof(OBJECT_TYPE) + strlen(TypeName) + 1);

		if (NULL != Type) {

			//
			// 类型名称字符串紧接在类型结构体的后面。
			//
			Type->Name = (PCHAR)Type + sizeof(OBJECT_TYPE);
			strcpy(Type->Name, TypeName);

			//
			// 初始化类型的对象链表及对象计数器。
			//
			ListInitializeHead(&Type->ObjectListHead);
			Type->ObjectCount = 0;
			Type->HandleCount = 0;

			//
			// 初始化类型的函数指针。
			//
			Type->Create = Initializer->Create;
			Type->Delete = Initializer->Delete;
			Type->Wait = Initializer->Wait;
			Type->Read = Initializer->Read;
			Type->Write = Initializer->Write;

			//
			// 将类型插入类型链表中并增加类型计数器。
			//
			SListPushEntry(&ObpTypeListHead, &Type->TypeListEntry);
			ObpTypeCount++;

			//
			// 返回新创建的类型。
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

功能描述：
	根据类型名称查找得到类型指针。

参数：
	TypeName -- 类型名称。

返回值：
	如果存在则返回类型指针，否则返回 NULL。

--*/
{
	POBJECT_TYPE ObjectType;
	PSINGLE_LIST_ENTRY ListEntry;

	//
	// 遍历类型链表，查找指定名称的类型。
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

功能描述：
	根据对象类型和对象名称查找对象。

参数：
	ObjectType -- 对象类型指针。
	ObjectName -- 对象名称。

返回值：
	如果存在则返回对象指针，否则返回 NULL。

--*/
{
	POBJECT_HEADER ObjectHeader;
	PLIST_ENTRY ListEntry;

	//
	// 遍历对象类型中的对象链表，查找指定名称的对象。
	//
	for (ListEntry = ObjectType->ObjectListHead.Next;
		ListEntry != &ObjectType->ObjectListHead;
		ListEntry = ListEntry->Next) {

		ObjectHeader = CONTAINING_RECORD(ListEntry, OBJECT_HEADER, TypeObjectListEntry);

		if (0 == strcmp(ObjectHeader->Name, ObjectName)) {
		
			//
			// 返回对象头中的对象体。
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

功能描述：
	得到所有对象的名称。

参数：


返回值：
	

--*/
{
	POBJECT_TYPE ObjectType;
	PSINGLE_LIST_ENTRY ListEntry;
	INT Num = 0;

	//
	// 遍历类型链表，查找指定名称的类型。
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
