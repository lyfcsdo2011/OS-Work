/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: ps.h

描述: 进程管理模块的对外接口头文件。



*******************************************************************************/

#ifndef _PS_
#define _PS_

#include "eosdef.h"
#include "rtl.h"

//
// 事件结构体
//
typedef struct _EVENT {
	BOOL IsManual;				// 是否手动类型事件
	BOOL IsSignaled;			// 是否处于 Signaled 状态
	LIST_ENTRY WaitListHead;	// 等待队列
}EVENT, *PEVENT;

//
// 互斥信号量结构体
//
typedef struct _MUTEX {
	PVOID OwnerThread;			// 当前拥有 Mutex 的线程指针
	ULONG RecursionCount;		// 递归拥有 Mutex 的计数器
	LIST_ENTRY WaitListHead;	// 等待队列
}MUTEX, *PMUTEX;

//
// 记录型信号量结构体
//
typedef struct _SEMAPHORE {
	LONG Count;					// 信号量的整形值
	LONG MaximumCount;			// 允许最大值
	LIST_ENTRY WaitListHead;	// 等待队列
}SEMAPHORE, *PSEMAPHORE;

//
// 磁盘互斥信号量结构体
//
typedef struct _DISKMUTEX {
	PVOID OwnerThread;			// 当前拥有 Mutex 的线程指针
	LIST_ENTRY WaitListHead;	// 等待队列
}DISKMUTEX, *PDISKMUTEX;

//
// 当前线程和进程的伪句柄定义。
//
#define CURRENT_PROCESS_HANDLE	((HANDLE)-1)
#define CURRENT_THREAD_HANDLE	((HANDLE)-2)

//
// 初始化进程模块第一步。
//
VOID
PsInitializeSystem1(
	VOID
	);

//
// 初始化进程模块第二步。
//
VOID
PsInitializeSystem2(
	VOID
	);

//
// 处理由Ke模块派遣过来的线程异常。
//
VOID
PsHandleException(
	ULONG ExceptionNumber,
	ULONG ErrorCode,
	PVOID Context
	);

//
// 得到当前进程对象，不会增加引用。
//
PVOID
PsGetCurrentProcessObject(
	VOID
	);

//
// 得到当前线程对象，不会增加引用。
//
PVOID
PsGetCurrentThreadObject(
	VOID
	);

//
// 得到当前进程的标准输入输出句柄。
//
STATUS
PsGetStdHandle(
	IN ULONG StdHandle,
	OUT PHANDLE Handle
	);

//
// 得到当前进程的映像文件名称和命令行参数。
//
VOID
PsGetImageNameAndCmdLine(
	OUT PCHAR ImageNameBuffer,
	OUT PCHAR CmdLineBuffer
	);

//
// 得到当前线程的错误码。
//
ULONG
PsGetLastError(
	VOID
	);

//
// 设置当前线程的错误码。
//
VOID
PsSetLastError(
	ULONG ErrCode
	);

//
// 得到指定进程的退出码。
//
STATUS
PsGetExitCodeProcess(
	IN HANDLE ProcessHandle,
	OUT PULONG ExitCode
	);

//
// 得到指定线程的退出码。
//
STATUS
PsGetExitCodeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG ExitCode
	);

//
// 得到指定进程的内核对象句柄表。
//
STATUS
PsGetObjectTable(
	IN HANDLE ProcessHandle,
	OUT PVOID *ObjectTable
	);

//
// 创建系统进程。
//
VOID
PsCreateSystemProcess(
	IN PTHREAD_START_ROUTINE StartAddr
	);

//
// 创建用户进程。
//
STATUS
PsCreateProcess(
	IN PCSTR ImageName,
	IN PCSTR CmdLine,
	IN ULONG CreateFlags,
	IN PSTARTUPINFO StartupInfo,
	OUT PPROCESS_INFORMATION ProcInfo
	);

//
// 创建线程。
//
STATUS
PsCreateThread(
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PHANDLE ThreadHandle,
	OUT PULONG ThreadId OPTIONAL
	);

//
// 通过进程Id打开进程对象。
//
STATUS
PsOpenProcess(
	IN ULONG ProcessId,
	OUT PHANDLE ProcessHandle
	);

//
// 通过线程Id打开线程对象。
//
STATUS
PsOpenThread(
	IN ULONG ThreadId,
	OUT PHANDLE ThreadHandle
	);

//
// 结束指定进程。
//
STATUS
PsTerminateProcess(
	IN HANDLE Handle,
	IN ULONG ExitCode
	);

//
// 结束指定线程。
//
STATUS
PsTerminateThread(
	IN HANDLE Handle,
	IN ULONG ExitCode
	);

//
// 退出当前进程。
//
VOID
PsExitProcess(
	IN ULONG ExitCode
	);

//
// 退出当前线程。
//
VOID
PsExitThread(
	IN ULONG ExitCode
	);

//
// 设置线程的优先级。
//
STATUS
PsSetThreadPriority(
	IN HANDLE Handle,
	IN UCHAR Priority
	);

//
// 获取线程的优先级。
//
STATUS
PsGetThreadPriority(
	IN HANDLE Handle,
	OUT PUCHAR Priority
	);

//
// 当前线程睡眠等待片刻。
//
VOID
PsSleep(
	IN ULONG Milliseconds
	);

//
// 初始化事件结构体。
//
VOID
PsInitializeEvent(
	IN PEVENT Event,
	IN BOOL ManualReset,
	IN BOOL InitialState
	);

//
// 等待事件。
//
STATUS
PsWaitForEvent(
	IN PEVENT Event,
	IN ULONG Milliseconds
	);

//
// 通知事件。使事件变为 Signaled 状态。
//
VOID
PsSetEvent(
	IN PEVENT Event
	);

//
// 复位事件。使事件变为 Nonsignaled 状态。
//
VOID
PsResetEvent(
	IN PEVENT Event
	);

//
// 创建事件对象。
//
STATUS
PsCreateEventObject(
	IN BOOL ManualReset,
	IN BOOL InitialState,
	IN PCSTR EventName,
	OUT PHANDLE EventHandle
	);

//
// 通知事件对象。使事件对象变为 Signaled 状态。
//
STATUS
PsSetEventObject(
	HANDLE Handle
	);

//
// 复位事件对象。使事件对象变为 Nonsignaled 状态。
//
STATUS
PsResetEventObject(
	HANDLE Handle
	);

//
// 初始化互斥信号量结构体。
//
VOID
PsInitializeMutex(
	IN PMUTEX Mutex,
	IN BOOL InitialOwner
	);

//
// 等待互斥信号量。
//
STATUS
PsWaitForMutex(
	IN PMUTEX Mutex,
	IN ULONG Milliseconds
	);

//
// 释放互斥信号量。
//
STATUS
PsReleaseMutex(
	IN PMUTEX Mutex
	);

//
// 创建互斥信号量对象。
//
STATUS
PsCreateMutexObject(
	IN BOOL InitialOwner,
	IN PCSTR MutexName,
	OUT PHANDLE MutexHandle
	);

//
// 释放互斥信号量对象。
//
STATUS
PsReleaseMutexObject(
	IN HANDLE Handle
	);

//
// 初始化磁盘互斥信号量结构体。
//
VOID
PsInitializeDiskMutex(
	IN PDISKMUTEX DiskMutex
	);

//
// 等待磁盘互斥信号量。
//
STATUS
PsWaitForDiskMutex(
	IN PDISKMUTEX DiskMutex,
	IN ULONG SectorNumber
	);

//
// 释放磁盘互斥信号量。
//
STATUS
PsReleaseDiskMutex(
	IN PDISKMUTEX DiskMutex
	);

#endif // _PS_
