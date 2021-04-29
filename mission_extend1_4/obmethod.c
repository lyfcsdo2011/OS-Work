/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: obmethod.c

描述: 对象虚函数的调用，包括 Wait、Read 和 Write。



*******************************************************************************/

#include "obp.h"

STATUS
ObWaitForObject(
	IN HANDLE Handle,
	IN ULONG Milliseconds
	)
/*++

功能描述：
	等待一个支持同步功能的内核对象，例如互斥信号量对象。

参数：
	Handle -- 支持同步功能的对象的句柄。
	Milliseconds -- 等待超时上限，单位毫秒。

返回值：
	返回值依对象类型而定。

--*/
{
	STATUS Status;
	PVOID Object;
	POBJECT_TYPE Type;

	//
	// 由对象句柄得到对象指针
	//
	Status = ObRefObjectByHandle(Handle, NULL, &Object);

	if (EOS_SUCCESS(Status)) {

		//
		// 由对象指针得到注册的对象类型
		//
		Type = OBJECT_TO_OBJECT_TYPE(Object);

		if (NULL != Type->Wait) {
			//
			// 调用对象类型中注册的 Wait 操作
			//
			Status = Type->Wait(Object, Milliseconds);
		} else {
			Status = STATUS_INVALID_HANDLE;
		}

		ObDerefObject(Object);
	}

	return Status;
}

STATUS
ObRead(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	)
/*++

功能描述：
	读支持读操作的内核对象，例如文件、控制台、管道、串口设备等对象。

参数：
	Handle -- 支持读操作的对象的句柄。
	Buffer -- 指针，指向用于保存读取结果的缓冲区。
	NumberOfBytesToRead -- 期望读取的字节数。
	NumberOfBytesRead -- 指针，指向用于保存实际读取字节数的整形变量。

返回值：
	依对象类型而定。

--*/
{
	STATUS Status;
	PVOID Object;
	POBJECT_TYPE Type;

	//
	// 由对象句柄得到对象指针
	//
	Status = ObRefObjectByHandle(Handle, NULL, &Object);

	if (EOS_SUCCESS(Status)) {

		//
		// 由对象指针得到注册的对象类型
		//
		Type = OBJECT_TO_OBJECT_TYPE(Object);

		if (NULL != Type->Read) {
			//
			// 调用对象类型中注册的 Read 操作
			//
			Status = Type->Read(Object, Buffer, NumberOfBytesToRead, NumberOfBytesRead);
		} else {
			Status = STATUS_INVALID_HANDLE;
		}

		ObDerefObject(Object);
	}

	return Status;
}

STATUS
ObWrite(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	)
/*++

功能描述：
	写支持写操作的内核对象，例如文件、控制台、管道、串口设备等对象。

参数：
	Handle -- 支持写操作的对象的句柄。
	Buffer -- 指针，指向用于保存待写数据的缓冲区。
	NumberOfBytesToWrite -- 期望写的字节数。
	NumberOfBytesWritten -- 指针，指向用于保存实际写入字节数的整形变量。

返回值：
	依对象类型而定。

--*/
{
	STATUS Status;
	PVOID Object;
	POBJECT_TYPE Type;

	//
	// 由对象句柄得到对象指针
	//
	Status = ObRefObjectByHandle(Handle, NULL, &Object);

	if (EOS_SUCCESS(Status)) {

		//
		// 由对象指针得到注册的对象类型
		//
		Type = OBJECT_TO_OBJECT_TYPE(Object);

		if (NULL != Type->Write) {
			//
			// 调用对象类型中注册的 Write 操作
			//
			Status = Type->Write(Object, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
		} else {
			Status = STATUS_INVALID_HANDLE;
		}

		ObDerefObject(Object);
	}

	return Status;
}
