/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: ob.h

描述: 对象管理器公开接口头文件。



*******************************************************************************/

#ifndef _OB_
#define _OB_

#include "eosdef.h"

#define OB_WAIT_ACCESS		0x00000001
#define OB_READ_ACCESS		0x00000002
#define OB_WRITE_ACCESS		0x00000004

//
// 对象类型指针类型
//
typedef struct _OBJECT_TYPE *POBJECT_TYPE;

//
// 句柄表指针类型
//
typedef struct _HANDLE_TABLE *PHANDLE_TABLE;

//
// 对象类型的虚方法的接口定义。
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
// 对象类型初始化的参数结构体。
//
typedef struct _OBJECT_TYPE_INITIALIZER {
	OB_CREATE_METHOD Create;
	OB_DELETE_METHOD Delete;
	OB_WAIT_METHOD Wait;
	OB_READ_METHOD Read;
	OB_WRITE_METHOD Write;
}OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

//
// 初始化对象管理模块第一步。
//
VOID
ObInitializeSystem1(
	VOID
	);

//
// 初始化对象管理模块第二步。
//
VOID
ObInitializeSystem2(
	VOID
	);

//
// 创建一个对象类型。
//
STATUS
ObCreateObjectType(
	IN PCSTR TypeName,
	IN POBJECT_TYPE_INITIALIZER Initializer,
	OUT POBJECT_TYPE *ObjectType
	);

//
// 创建一个指定类型的对象。
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
// 得到对象的ID
//
ULONG
ObGetObjectId(
	IN PVOID Object
	);

//
// 复制一份对象的引用。
//
VOID
ObRefObject(
	IN PVOID Object
	);

//
// 由对象名称和类型得到对象的一份引用。
//
STATUS
ObRefObjectByName(
	IN PCSTR ObjectName,
	IN POBJECT_TYPE ObjectType,
	OUT PVOID *Object
	);

//
// 由对象Id得到对象的一份引用。
//
STATUS
ObRefObjectById(
	IN ULONG Id,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	);

//
// 释放对象的一份引用。
//
VOID
ObDerefObject(
	IN PVOID Object
	);

//
// 得到对象的有效访问掩码。
//
ULONG
ObGetAccessMask(
	IN PVOID Object
	);

//
// 分配一个句柄表。
//
PHANDLE_TABLE
ObAllocateHandleTable(
	VOID
	);

//
// 释放句柄表。
//
VOID
ObFreeHandleTable(
	IN PHANDLE_TABLE ObjectTable
	);

//
// 在句柄表中分配一个空句柄。
//
STATUS
ObAllocateHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	OUT PHANDLE Handle
	);

//
// 释放空句柄。
//
STATUS
ObFreeHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle
	);

//
// 设置空句柄的值。
//
STATUS
ObSetHandleValueEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle,
	IN PVOID Object
	);

//
// 在当前进程的句柄表中为对象创建一个句柄。
//
STATUS
ObCreateHandle(
	IN PVOID Object,
	OUT PHANDLE Handle
	);

//
// 在指定句柄表中为对象创建一个句柄。
//
STATUS
ObCreateHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN PVOID Object,
	OUT PHANDLE Handle
	);

//
// 得到句柄在当前进程句柄表中所关联对象的一份引用。
//
STATUS
ObRefObjectByHandle(
	IN HANDLE Handle,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	);

//
// 得到句柄在指定句柄表中所关联对象的一份引用。
//
STATUS
ObRefObjectByHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle,
	IN POBJECT_TYPE ObjectType OPTIONAL,
	OUT PVOID *Object
	);

//
// 关闭当前进程句柄表中的对象句柄。
//
STATUS
ObCloseHandle(
	IN HANDLE Handle
	);

//
// 关闭指定句柄表中的对象句柄。
//
STATUS
ObCloseHandleEx(
	IN PHANDLE_TABLE ObjectTable,
	IN HANDLE Handle
	);

//
// 调用对象的Wait虚函数。
//
STATUS
ObWaitForObject(
	IN HANDLE Handle,
	IN ULONG Milliseconds
	);

//
// 调用对象的Read虚函数。
//
STATUS
ObRead(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	);

//
// 调用对象的Write虚函数。
//
STATUS
ObWrite(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);

#endif // _OB_
