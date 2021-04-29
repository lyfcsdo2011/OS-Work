/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: psp.h

描述: 进程管理模块的内部头文件。



*******************************************************************************/

#ifndef _PSP_
#define _PSP_

#include "ke.h"
#include "mm.h"
#include "ob.h"
#include "ps.h"
#include "rtl.h"

//
// 进程对象、线程对象指针。
//
typedef struct _PROCESS *PPROCESS;
typedef struct _THREAD *PTHREAD;

#ifdef _DEBUG

extern void record_task_state(long pid, long new_state, long Tick, const char* fun_name, int line_num);
#define RECORD_TASK_STATE(pid, state, jiffies) record_task_state(pid, state, Tick, __FUNCTION__, __LINE__);

extern volatile INT ThreadSeq;
extern INT MaxTid;

#endif

//
// 进程对象结构体 (PCB)。
//
typedef struct _PROCESS {
	BOOLEAN System;						// 是否系统进程
	UCHAR Priority;						// 进程的优先级
	PMMPAS Pas;							// 进程地址空间 
	PHANDLE_TABLE ObjectTable;			// 进程的内核对象句柄表
	LIST_ENTRY ThreadListHead;			// 线程链表头
	PTHREAD PrimaryThread;				// 主线程指针
	LIST_ENTRY WaitListHead;			// 等待队列，所有等待进程结束的线程都在此队列等待。

	PSTR ImageName;						// 二进制映像文件名称
	PSTR CmdLine;						// 命令行参数
	PVOID ImageBase;					// 可执行映像的加载基址
	PPROCESS_START_ROUTINE ImageEntry;	// 可执行映像的入口地址

	HANDLE StdInput;
	HANDLE StdOutput;
	HANDLE StdError;

	ULONG ExitCode;						// 进程退出码
} PROCESS;

//
// 线程对象结构体 (TCB)。
//
typedef	struct _THREAD {
	PPROCESS Process;					// 线程所属进程指针
	LIST_ENTRY ThreadListEntry;			// 进程的线程链表项
	UCHAR Priority;						// 线程优先级
	UCHAR State;						// 线程当前状态
	ULONG RemainderTicks;				// 剩余时间片，用于时间片轮转调度
	STATUS WaitStatus;					// 阻塞等待的结果状态
	KTIMER WaitTimer;					// 用于有限等待唤醒的计时器
	LIST_ENTRY StateListEntry;			// 所在状态队列的链表项
	LIST_ENTRY WaitListHead;			// 等待队列，所有等待线程结束的线程都在此队列等待。

	//
	// 为了结构简单，EOS没有对内核进行隔离保护，所有线程都运行在内核状态，所以目
	// 前线程没有用户空间的栈。
	//
	PVOID KernelStack;					// 线程位于内核空间的栈
	CONTEXT KernelContext;				// 线程执行在内核状态的上下文环境状态

	//
	// 线程必须在所属进程的地址空间中执行用户代码，但可在任何进程的地址空间中执行
	// 内核代码，因为内核代码位于所有进程地址空间共享的系统地址空间中。
	//
	PMMPAS AttachedPas;					// 线程在执行内核代码时绑定进程地址空间。

	PTHREAD_START_ROUTINE StartAddr;	// 线程的入口函数地址
	PVOID Parameter;					// 传递给入口函数的参数

	ULONG LastError;					// 线程最近一次的错误码
	ULONG ExitCode;						// 线程的退出码
} THREAD;

//
// 线程的四种状态：就绪 (Ready)、运行 (Running)、等待 (Waiting) 和结束 (Terminated)。
// 注意：Zero 不是线程的有效状态，是一种游离状态，是线程状态转换的中间态。
//
typedef enum _THREAD_STATE {
	Zero,		// 0
	Ready,		// 1
	Running,	// 2
	Waiting,	// 3
	Terminated	// 4
} THREAD_STATE;

#define TS_CREATE 	0	// 创建
#define TS_READY	1	// 就绪态
#define TS_RUNNING	2	// 运行态
#define TS_WAIT		3	// 阻塞态
#define TS_STOPPED	4	// 结束

//
// 用于时间片轮转调度的时间片大小（即时间片包含的时钟滴答数）
//
#define TICKS_OF_TIME_SLICE		6

//
// 线程内核栈大小--2 个页面。
//
#define KERNEL_STACK_SIZE	(PAGE_SIZE * 2)

//
// 进程、线程对象类型。 
//
extern POBJECT_TYPE PspProcessType;
extern POBJECT_TYPE PspThreadType;
extern POBJECT_TYPE PspSemaphoreType;
extern POBJECT_TYPE PspMutexType;

extern LIST_ENTRY PspReadyListHeads[32];

//
// 系统进程指针。
//
extern PPROCESS PspSystemProcess;

//
// 当前运行线程指针。
//
extern volatile PTHREAD PspCurrentThread;

//
// 当前进程的指针
//
#define PspCurrentProcess (PspCurrentThread->Process)


//
// 使 Zero 或 Running 线程转入 Ready 状态。
//
VOID
PspReadyThread(
	IN PTHREAD Thread
	);

//
// 使 Ready 线程转入 Zero 状态。
//
VOID
PspUnreadyThread(
	IN PTHREAD Thread
	);

//
// 当前线程按照 FCFS 阻塞等待在指定的等待队列中，如果等待时间超过 Milliseconds 而仍
// 未被唤醒，线程将被自动唤醒并返回 STATUS_TIMEOUT。
//
STATUS
PspWait(
   IN PLIST_ENTRY WaitListHead,
   IN ULONG Milliseconds
   );

//
// 使 Waiting 线程转入 Zero 状态。
//
VOID
PspUnwaitThread(
	IN PTHREAD Thread
	);

//
// 按照 FCFS 的原则唤醒指定等待队列中的一个线程，被唤醒线程进入就绪状态准备运行。
// WaitStatus 将作为被唤醒线程在调用 PspWait() 时的返回值。
//
PTHREAD
PspWakeThread(
	IN PLIST_ENTRY WaitListHead,
	IN STATUS WaitStatus
	);

//
// 执行线程调度。
//
VOID
PspThreadSchedule(
	VOID
	);

//
// 当前线程附着到指定进程的地址空间中执行。
//
VOID
PspThreadAttachProcess(
	IN PPROCESS Process
	);

//
// 加载应用程序的可执行映像。
//
STATUS
PspLoadProcessImage(
	IN PPROCESS Process,
	IN PSTR ImageName,
	OUT PVOID *ImageBase,
	OUT PVOID *ImageEntry
	);

//
// 用户进程的启动函数。
//
ULONG
PspProcessStartup(
	PVOID Parameter
	);

//
// 创建进程环境。
//
STATUS
PspCreateProcessEnvironment(
	IN UCHAR Priority,
	IN PCSTR ImageName,
	IN PCSTR CmdLine,
	OUT PPROCESS *Process
	);

//
// 删除进程环境。
//
VOID
PspDeleteProcessEnvironment(
	IN PPROCESS Process
	);

//
// 在指定的进程内创建一个线程。
//
STATUS
PspCreateThread(
	IN PPROCESS Process,
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PTHREAD *Thread
	);

//
// 初始化线程的上下文环境。
//
VOID
PspInitializeThreadContext(
	IN PTHREAD Thread
	);

//
// 结束指定进程的运行。
//
VOID
PspTerminateProcess(
	IN PPROCESS Process,
	IN ULONG ExitCode
	);

//
// 结束指定线程的运行。
//
VOID
PspTerminateThread(
	IN PTHREAD Thread,
	IN ULONG ExitCode,
	IN BOOL IsTerminatingProcess
	);

#endif // _PSP_
