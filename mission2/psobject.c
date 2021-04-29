/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: psobject.c

描述: 和进程、线程对象相关的一些函数。



*******************************************************************************/

#include "psp.h"

//
// 进程、线程对象的类型指针。
//
POBJECT_TYPE PspProcessType;
POBJECT_TYPE PspThreadType;

STATUS
PspOnWaitForProcessObject(
	IN PVOID ProcessObject,
	IN ULONG Milliseconds
	);

STATUS
PspOnWaitForThreadObject(
	IN PVOID ThreadObject,
	IN ULONG Milliseconds
	);

VOID
PspOnDeleteThreadObject(
	IN PVOID ThreadObject
	);

VOID
PspCreateProcessObjectType(
	VOID
	)
/*++

功能描述：
	创建进程和线程对象类型。

参数：
	无。

返回值：
	无。

--*/
{
	STATUS Status;
	OBJECT_TYPE_INITIALIZER Initializer;

	//
	// 创建进程对象类型。
	//
	Initializer.Create = NULL;
	Initializer.Delete = NULL;
	Initializer.Wait = PspOnWaitForProcessObject;
	Initializer.Read = NULL;
	Initializer.Write = NULL;

	Status = ObCreateObjectType("PROCESS", &Initializer, &PspProcessType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create process object type!");
	}

	//
	// 创建线程对象类型。
	//
	Initializer.Delete = PspOnDeleteThreadObject;
	Initializer.Wait = PspOnWaitForThreadObject;

	Status = ObCreateObjectType("THREAD", &Initializer, &PspThreadType );

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create thread object type!");
	}
}

STATUS
PsGetObjectTable(
	IN HANDLE ProcessHandle,
	OUT PVOID *ObjectTable
	)
/*++

功能描述：
	得到指定进程的内核对象句柄表。

参数：
	ProcessHandle -- 进程句柄。
	ObjectTable -- 指针，指向用于保存句柄指针的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PPROCESS Process;

	if (CURRENT_PROCESS_HANDLE == ProcessHandle) {

		*ObjectTable = PspCurrentProcess->ObjectTable;
		Status = STATUS_SUCCESS;

	} else {

		Status = ObRefObjectByHandleEx( PspCurrentProcess->ObjectTable,
										ProcessHandle,
										PspProcessType,
										(PVOID*)&Process );

		if (EOS_SUCCESS(Status)) {

			if (NULL != Process->ObjectTable) {
				*ObjectTable = Process->ObjectTable;
			} else {
				Status = STATUS_PROCESS_IS_TERMINATING;
			}
		}
	}

	return Status;
}

STATUS
PsGetStdHandle(
	IN ULONG StdHandle,
	OUT PHANDLE Handle
	)
/*++

功能描述：
	得到当前进程的标准输入输出句柄。

参数：
	StdHandle -- 标准输入输出句柄的索引，其值必须是STD_INPUT_HANDLE、
		STD_OUTPUT_HANDLE和STD_ERROR_HANDLE中之一。

	Handle -- 指针，指向用于保存句柄的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	if (STD_INPUT_HANDLE == StdHandle) {
		*Handle = PspCurrentProcess->StdInput;
	} else if (STD_OUTPUT_HANDLE == StdHandle) {
		*Handle = PspCurrentProcess->StdOutput;
	} else if (STD_ERROR_HANDLE == StdHandle) {
		*Handle = PspCurrentProcess->StdError;
	} else {
		return STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}

//
// 得到当前进程的映像文件名称和命令行参数。
//
VOID
PsGetImageNameAndCmdLine(
	OUT PCHAR ImageNameBuffer,
	OUT PCHAR CmdLineBuffer
	)
{
	ASSERT(PspCurrentProcess != PspSystemProcess);

	if (NULL != ImageNameBuffer) {
		strcpy(ImageNameBuffer, PspCurrentProcess->ImageName);
	}

	if (NULL != CmdLineBuffer) {
		if (NULL != PspCurrentProcess->CmdLine) {
			strcpy(CmdLineBuffer, PspCurrentProcess->CmdLine);
		}
		else {
			*CmdLineBuffer = 0;
		}
	}
}

//
// 得到当前线程的错误码。
//
ULONG
PsGetLastError(
	VOID
	)
{
	return PspCurrentThread->LastError;
}

//
// 设置当前线程的错误码。
//
VOID
PsSetLastError(
	IN ULONG ErrCode
	)
{
	PspCurrentThread->LastError = ErrCode;
}

STATUS
PsGetExitCodeProcess(
	IN HANDLE ProcessHandle,
	OUT PULONG ExitCode
	)
/*++

功能描述：
	得到指定进程的退出码。

参数：
	ProcessHandle -- 进程句柄。
	ExitCode -- 指针，指向用于保存退出码的变量。

返回值：
	如果成功则返回STATUS_SUCESS。

--*/
{
	STATUS Status;
	PPROCESS ProcessObject;

	Status = ObRefObjectByHandle( ProcessHandle,
								  PspProcessType,
								  (PVOID*)&ProcessObject );

	if (EOS_SUCCESS(Status)) {
		*ExitCode = ProcessObject->ExitCode;
		
		ObDerefObject(ProcessObject);
	}

	return Status;
}

STATUS
PsGetExitCodeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG ExitCode
	)
/*++

功能描述：
	得到指定线程的退出码。

参数：
	ThreadHandle -- 线程句柄。
	ExitCode -- 指针，指向用于保存退出码的变量。

返回值：
	如果成功则返回STATUS_SUCESS。

--*/
{
	STATUS Status;
	PPROCESS ThreadObject;

	Status = ObRefObjectByHandle( ThreadHandle,
								  PspProcessType,
								  (PVOID*)&ThreadObject );

	if (EOS_SUCCESS(Status)) {
		*ExitCode = ThreadObject->ExitCode;
		
		ObDerefObject(ThreadObject);
	}

	return Status;
}

STATUS
PsOpenProcess(
	IN ULONG ProcessId,
	OUT PHANDLE ProcessHandle
	)
/*++

功能描述：
	通过进程Id打开进程对象的句柄。

参数：
	ProcessId -- 期望打开的进程对象的Id。
	ProcessHandle -- 指针，指向用于保存进程句柄的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PVOID ProcessObject;

	Status = ObRefObjectById(ProcessId, PspProcessType, &ProcessObject);

	if (!EOS_SUCCESS(Status)) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = ObCreateHandle(ProcessObject, ProcessHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(ProcessObject);
	}

	return Status;
}

STATUS
PsOpenThread(
	IN ULONG ThreadId,
	OUT PHANDLE ThreadHandle
	)
/*++

功能描述：
	通过线程Id打开线程对象的句柄。

参数：
	ThreadId -- 期望打开的线程对象的Id。
	ThreadHandle -- 指针，指向用于保存线程句柄的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PVOID ThreadObject;

	Status = ObRefObjectById(ThreadId, PspThreadType, &ThreadObject);

	if (!EOS_SUCCESS(Status)) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = ObCreateHandle(ThreadObject, ThreadHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(ThreadObject);
	}

	return Status;
}
