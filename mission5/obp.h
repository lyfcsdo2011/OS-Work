/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: obp.h

描述: 对象管理模块的内部头文件。



*******************************************************************************/

#ifndef _OBP_
#define _OBP_

#include "ke.h"
#include "mm.h"
#include "ob.h"

//
// 对象类型结构体。
//
typedef struct _OBJECT_TYPE{
	PSTR Name;							// 类型名称字符串指针
	SINGLE_LIST_ENTRY TypeListEntry;	// 类型链表项，所有OBJECT_TYPE结构体被插入了类型链表中
	LIST_ENTRY ObjectListHead;			// 对象链表头，所有属于此类型的对象都插入这个链表中
	ULONG ObjectCount;					// 对象计数器，所有属于此类型的对象的总数
	ULONG HandleCount;					// 句柄计数器，所有属于此类型的对象的句柄总数
	OB_CREATE_METHOD Create;			// 此类型对象的构造函数指针，创建对象时被调用
	OB_DELETE_METHOD Delete;			// 此类型对象的析构函数指针，删除对象时被调用
	OB_WAIT_METHOD Wait;				// 此类型对象的WaitForSingleObject函数指针
	OB_READ_METHOD Read;				// 此类型对象的ReadFile函数指针
	OB_WRITE_METHOD Write;				// 此类型对象的WriteFile函数指针
} OBJECT_TYPE;

//
// 对象头结构体。
//
typedef struct _OBJECT_HEADER {
	PSTR Name;							// 对象名称字符串指针
	ULONG Id;							// 对象ID，全局唯一
	POBJECT_TYPE Type;					// 对象类型指针
	ULONG PointerCount;					// 对象引用计数器
	ULONG HandleCount;					// 对象句柄计数器
	LIST_ENTRY TypeObjectListEntry;		// 所属类型的对象链表项
	LIST_ENTRY GlobalObjectListEntry;	// 全局的对象链表项
	QUAD Body;							// 对象体的起始域
} OBJECT_HEADER, *POBJECT_HEADER;

//
// 由对象体指针得到对象头指针的宏。
//
#define OBJECT_TO_OBJECT_HEADER( o )\
	CONTAINING_RECORD( (o), OBJECT_HEADER, Body )

//
// 得到对象类型指针的宏。
//
#define OBJECT_TO_OBJECT_TYPE( o )\
	( OBJECT_TO_OBJECT_HEADER( o )->Type )

//
// 句柄表项结构体，目前仅有一个域：对象指针。
//
typedef struct _HANDLE_TABLE_ENTRY {
	union {
		PVOID Object;
		ULONG_PTR Next;
	} u;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

//
// 进程句柄表结构体。
//
typedef struct _HANDLE_TABLE {
	PHANDLE_TABLE_ENTRY HandleTable;
	ULONG_PTR FreeEntryListHead;
	ULONG_PTR HandleCount;
} HANDLE_TABLE;

//
// 根据类型名称查找类型
//
POBJECT_TYPE
ObpLookupObjectTypeByName(
	IN PCSTR TypeName
	);

//
// 根据对象类型和对象名称查找对象。
//
PVOID
ObpLookupObjectByName(
	IN POBJECT_TYPE ObjectType,
	IN PCSTR ObjectName
	);

#endif // _OBP_
