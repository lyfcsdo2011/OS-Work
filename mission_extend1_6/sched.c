/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: sched.c

描述: 线程调度的实现。包括线程状态的转换。



*******************************************************************************/

#include "psp.h"

//
// 32个链表头组成的数组，分别对应了0~31的32个优先级的就绪队列。
// 下标为n的链表对应优先级为n的就绪队列。
//
LIST_ENTRY PspReadyListHeads[32];

//
// 32位就绪位图。
// 如果位图的第n位为1，则表明优先级为n的就绪队列非空。
//
volatile ULONG PspReadyBitmap = 0;

//
// 睡眠等待线程队列。
// 线程调用Sleep后，在这个队列中进行等待。
//
LIST_ENTRY PspSleepingListHead;

//
// 已结束线程队列。
//
LIST_ENTRY PspTerminatedListHead;

//
// 当前运行线程。
//
volatile PTHREAD PspCurrentThread = NULL;


VOID
PspInitSchedulingQueue(
	VOID
	)
/*++

功能描述：
	初始化线程调度要使用的各种队列。

参数：
	无。

返回值：
	无。

--*/
{
	INT i;

	//
	// 初始化32个优先级对应的32个就绪队列。
	//
	for(i = 0; i < 32; i++)
		ListInitializeHead(&PspReadyListHeads[i]);

	//
	// 初始化睡眠等待队列。
	//
	ListInitializeHead(&PspSleepingListHead);

	//
	// 初始化已结束线程队列。
	//
	ListInitializeHead(&PspTerminatedListHead);
}

//
// 得到当前线程对象指针。
//
PVOID
PsGetCurrentThreadObject(
	VOID
	)
{
	return PspCurrentThread;
}

//
// 得到当前进程对象指针。
//
PVOID
PsGetCurrentProcessObject(
	VOID
	)
{
	return PspCurrentProcess;
}

VOID
PspReadyThread(
	PTHREAD Thread
	)
/*++

功能描述：
	使 Zero 状态或者运行状态的线程转入就绪状态。

参数：
	Thread -- 线程指针。

返回值：
	无。

--*/
{
	ASSERT(NULL != Thread);
	ASSERT(Zero == Thread->State || Running == Thread->State);

	//
	// 将线程插入其优先级对应的就绪队列的队尾，并设置就绪位图中对应的位。
	// 最后将线程的状态修改为就绪状态。
	//
	ListInsertTail(&PspReadyListHeads[Thread->Priority], &Thread->StateListEntry);
	BIT_SET(PspReadyBitmap, Thread->Priority);
	Thread->State = Ready;

#ifdef _DEBUG
	RECORD_TASK_STATE(ObGetObjectId(Thread) , TS_READY, Tick);
#endif
}

VOID
PspUnreadyThread(
	PTHREAD Thread
	)
/*++

功能描述：
	取消线程的就绪状态，使线程转入 Zero 状态。

参数：
	Thread - 当前处于就绪状态的线程的指针。

返回值：
	无。

--*/
{
	ASSERT(NULL != Thread && Ready == Thread->State);

	//
	// 将线程从所在的就绪队列中取出，如果线程优先级对应的就绪队列变为空，
	// 则清除就绪位图中对应的位。
	//
	ListRemoveEntry(&Thread->StateListEntry);

	if(ListIsEmpty(&PspReadyListHeads[Thread->Priority])) {
		BIT_CLEAR(PspReadyBitmap, Thread->Priority);
	}

	Thread->State = Zero;
}

PRIVATE
VOID
PspOnWaitTimeout(
	IN ULONG_PTR Param
	)
/*++

功能描述：
	等待超时计时器回调函数，唤醒等待超时的线程。

参数：
	Param -- 等待超时的线程的指针（需要类型强制转换）。

返回值：
	无

--*/
{
	PspUnwaitThread((PTHREAD)Param);
	PspReadyThread((PTHREAD)Param);
}

STATUS
PspWait(
	IN PLIST_ENTRY WaitListHead,
	IN ULONG Milliseconds
	)
/*++

功能描述：
	当前线程按照FCFS的原则插入指定的等待队列的队尾，线程阻塞等待直到等待超时或
	者PspWakeThread被调用。

参数：
	WaitListHead -- 欲加入的等待队列的指针。
	Milliseconds -- 有限等待时间(单位ms)，如果等待时间超出，则被系统自动唤醒并
		返回STATUS_TIMEOUT。如果为0，则立即返回STATUS_TIMEOUT。如果为INFINIT，
		则永久等待直到PspWakeThread被调用。

返回值：
	如果线程等待超时则返回STATUS_TIMEOUT，否则返回PspWakeThread的第二个参数
	WaitStatus。

--*/
{
	ASSERT(0 == KeGetIntNesting());
	ASSERT(Running == PspCurrentThread->State);
	ASSERT(0 != PspReadyBitmap);

	if(0 == Milliseconds) {
		return STATUS_TIMEOUT;
	}

	//
	// 将当前线程插入等待队列的队尾并修改线程状态码为Waiting。
	//
	ListInsertTail(WaitListHead, &PspCurrentThread->StateListEntry);
	PspCurrentThread->State = Waiting;
	
#ifdef _DEBUG
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_WAIT, Tick);
#endif

	//
	// 如果不是永久等待，就注册一个用于超时唤醒线程的等待计时器。
	//
	if (INFINITE != Milliseconds) {

		KeInitializeTimer( &PspCurrentThread->WaitTimer,
						   Milliseconds,
						   PspOnWaitTimeout,
						   (ULONG_PTR)PspCurrentThread );

		KeRegisterTimer(&PspCurrentThread->WaitTimer);

		PspCurrentThread->WaitStatus = STATUS_TIMEOUT;

	} else {

		PspCurrentThread->WaitStatus = STATUS_SUCCESS;
	}

	//
	// 当前线程进入等待状态后需要让出处理器（让权等待），执行线程调度。
	//
	PspThreadSchedule();

	//
	// zzZ...
	//

	//
	// 线程被唤醒继续执行，返回等待结果状态码。
	//
	return PspCurrentThread->WaitStatus;
}

VOID
PspUnwaitThread(
	IN PTHREAD Thread
	)
/*++

功能描述：
	使处于等待状态的线程脱离等待队列并转入 Zero 状态。

参数：
	Thread -- 目标线程对象指针。

返回值：
	无。

--*/
{
	ASSERT(Waiting == Thread->State);

	//
	// 将线程从所在等待队列中移除并修改状态码为Zero。
	//
	ListRemoveEntry(&Thread->StateListEntry);
	Thread->State = Zero;

	//
	// 如果线程注册了等待计时器，则注销等待计时器。
	//
	if (STATUS_TIMEOUT == Thread->WaitStatus) {
		KeUnregisterTimer(&Thread->WaitTimer);
	}
}

PTHREAD
PspWakeThread(
	IN PLIST_ENTRY WaitListHead,
	IN STATUS WaitStatus
	)
/*++

功能描述：
	唤醒指定等待队列的队首线程。

参数：
	WaitListHead -- 等待队列指针。
	WaitStatus -- 被唤醒线程从PspWait返回的返回值。

返回值：
	如果等待队列为空则返回NULL，否则返回被唤醒线程的指针。

--*/
{
	PTHREAD Thread;

	if (!ListIsEmpty(WaitListHead)) {

		//
		// 唤醒等待队列的队首线程。
		//
		Thread = CONTAINING_RECORD(WaitListHead->Next, THREAD, StateListEntry);
		PspUnwaitThread(Thread);
		PspReadyThread(Thread);

		//
		// 设置线程从PspWait返回的返回值。
		//
		Thread->WaitStatus = WaitStatus;

	} else {

		Thread = NULL;
	}

	return Thread;
}

VOID
PspRoundRobin(
	VOID
	)
/*++

功能描述：
	时间片轮转调度函数，被定时计数器中断服务程序 KiIsrTimer 调用。

参数：
	无。

返回值：
	无。

--*/
{
	//
	// 在此添加代码，实现时间片轮转调度算法。
	//
	
	return;
}

PCONTEXT
PspSelectNextThread(
	VOID
	)
/*++

功能描述：
	线程调度函数。当最外层中断服务程序执行完成后，并不是立刻返回被中断运行的线
	程，而是调用这个函数选择一个合适的线程继续运行（被中断的线程可能继续运行）。

参数：
	无。

返回值：
	应执行线程的CPU环境块指针。

--*/
{
	ULONG HighestPriority;
	SIZE_T StackSize;

	//
	// 扫描就绪位图，获得当前最高优先级。注意：就绪位图可能为空。
	//
	BitScanReverse(&HighestPriority, PspReadyBitmap);

	if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {

		if (0 != PspReadyBitmap && HighestPriority > PspCurrentThread->Priority) {

			//
			// 如果存在比当前运行线程优先级更高的就绪线程，当前线程应被抢先。
			// 因为当前线程仍处于运行状态，所以被高优先级线程抢先后应插入其
			// 优先级对应的就绪队列的队首。注意，不能调用 PspReadyThread。
			//
			ListInsertHead( &PspReadyListHeads[PspCurrentThread->Priority],
							&PspCurrentThread->StateListEntry );
			BIT_SET(PspReadyBitmap, PspCurrentThread->Priority);
			PspCurrentThread->State = Ready;

#ifdef _DEBUG
			RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_READY, Tick);
#endif

		} else {

			//
			// 当前线程继续运行。
			// 注意：中断程序执行时可能换出了当前线程绑定运行的地址空间。
			//
			MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);
			return &PspCurrentThread->KernelContext;
		}

	} else if(0 == PspReadyBitmap) {

		//
		// 被中断运行线程处于非运行状态，必须存在一个可运行的就绪线程。
		//
		ASSERT(FALSE);
		KeBugCheck("No ready thread to run!");
	}

	if (NULL != PspCurrentThread) {

		//
		// 如果当前线程结束了自己则在这里释放线程的内核栈，因为线程在执行时不能
		// 释放自己正在占用的栈。
		//
		if (Terminated == PspCurrentThread->State) {

			StackSize = 0;

			MmFreeVirtualMemory( &PspCurrentThread->KernelStack,
								 &StackSize,
								 MEM_RELEASE,
								 TRUE );
		}

		//
		// 取消指针 PspCurrentThread 对线程对象的引用。
		//
		ObDerefObject(PspCurrentThread);
	}

	//
	// 选择优先级最高的非空就绪队列的队首线程作为当前运行线程。
	//
	PspCurrentThread = CONTAINING_RECORD(PspReadyListHeads[HighestPriority].Next, THREAD, StateListEntry);
	ObRefObject(PspCurrentThread);

	PspUnreadyThread(PspCurrentThread);
	PspCurrentThread->State = Running;
	
#ifdef _DEBUG
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_RUNNING, Tick);
#endif

	//
	// 换入线程绑定运行的地址空间。
	//
	MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);

	//
	// 返回线程的上下文环境块，恢复线程运行。
	//
	return &PspCurrentThread->KernelContext;
}

VOID
PspThreadSchedule(
	VOID
	)
/*++

功能描述：
	执行线程调度。

参数：
	无。

返回值：
	无。

--*/
{
	ULONG HighestPriority;

	//
	// 注意，如果当前正在处理中断（中断嵌套深度不为 0）则什么也不做，
	// 因为在中断返回时系统会自动执行线程调度。
	//
	if (KeGetIntNesting() == 0) {

		if (Running != PspCurrentThread->State) {

			//
			// 当前线程已经处于非运行状态，执行线程调度。
			//
			KeThreadSchedule();

		} else if (0 != PspReadyBitmap) {

			//
			// 扫描就绪位图，如果存在比当前线程优先级高的就绪线程则执行线程调度。
			//
			BitScanReverse(&HighestPriority, PspReadyBitmap);
			if (HighestPriority > PspCurrentThread->Priority)
				KeThreadSchedule();
		}
	}
}

VOID
PsSleep(
	IN ULONG Milliseconds
	)
/*++

功能描述：
	当前线程停止运行指定的时间间隔。

参数：
	Milliseconds -- 停止运行的时间间隔，单位毫秒。
					注意，如果此值为 INFINITE，当前线程将永远停止运行。

返回值：
	无。

--*/
{
	BOOL IntState;

	ASSERT(KeGetIntNesting() == 0);

	IntState = KeEnableInterrupts(FALSE);

	if (0 == Milliseconds) {

		//
		// 如果存在同优先级的就绪线程，则当前线程进入就绪状态并执行线程调度。
		//
		if (BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
			PspReadyThread(PspCurrentThread);
			PspThreadSchedule();
		}

	} else {

		//
		// 当前线程在睡眠队列中等待超时。
		//
		PspWait(&PspSleepingListHead, Milliseconds);
	}

	KeEnableInterrupts(IntState);
}

STATUS
PsSetThreadPriority(
	IN HANDLE Handle,
	IN UCHAR Priority
	)
/*++

功能描述：
	设置指定线程优先级。注意：目标线程优先级改变后可能会触发线程调度。

参数：
	Handle -- 目标线程句柄。
	Priority -- 期望得到的优先级。

返回值：
	如果成功则返回STATUS_SUCCESS，否则说明参数无效。

--*/
{
	STATUS Status;
	BOOL IntState;
	PTHREAD Thread;

	ASSERT(Priority <= 31);

	if (Priority > 31) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = ObRefObjectByHandle(Handle, PspThreadType, (PVOID*)&Thread);

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}
	
	IntState = KeEnableInterrupts(FALSE);

	if (Thread->Priority != Priority) {

		//
		// 就绪线程在改变优先级时需在不同优先级对应的就绪队列之间迁移。
		//
		if (Ready == Thread->State) {

			//
			// 线程脱离当前优先级对应的就绪队列。
			//
			PspUnreadyThread(Thread);

			Thread->Priority = Priority;

			//
			// 线程插入新优先级对应的就绪队列的队尾。
			//
			PspReadyThread(Thread);

		} else {

			Thread->Priority = Priority;
		}

		//
		// 优先级改变后需要执行线程调度。
		//
		PspThreadSchedule();
	}

	KeEnableInterrupts(IntState);

	ObDerefObject(Thread);

	return STATUS_SUCCESS;
}

STATUS
PsGetThreadPriority(
	IN HANDLE Handle,
	OUT PUCHAR Priority
	)
/*++

功能描述：
	获取指定线程的优先级。

参数：
	Handle -- 目标线程句柄。
	Priority -- 返回线程的优先级。

返回值：
	如果成功则返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	BOOL IntState;
	PTHREAD Thread;

	ASSERT(Priority != NULL);

	Status = ObRefObjectByHandle(Handle, PspThreadType, (PVOID*)&Thread);

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}
	
	IntState = KeEnableInterrupts(FALSE);
	
	//
	// 获取线程的优先级。
	//
	*Priority = Thread->Priority;

	KeEnableInterrupts(IntState);

	ObDerefObject(Thread);

	return STATUS_SUCCESS;
}

VOID
PspThreadAttachProcess(
	IN PPROCESS Process
	)
/*++

功能描述：
	当前线程附着在指定进程的地址空间中执行。

参数：
	Process -- 进程对象指针。

返回值：
	无。

--*/
{
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	if (PspCurrentThread->AttachedPas != Process->Pas) {

		MmSwapProcessAddressSpace(Process->Pas);

		PspCurrentThread->AttachedPas = Process->Pas;
	}

	KeEnableInterrupts(IntState);
}

#ifdef _DEBUG

/*
定义了一个线性表，用于记录进程状态转换的轨迹。
*/															
struct task_state_entry
{
	long pid;
	long new_state;
	long Tick;
	char fun_name[64];
	int line_num;
};

#define MAX_TASK_TRANS 1000		// 直接将此值修改的更大一些，就可以记录更多进程状态变换的轨迹。不要设定的太大，否则会导致绘制轨迹变慢或内存不足。
struct task_state_entry task_trans_table[MAX_TASK_TRANS]; 
long task_trans_count = 0;

void record_task_state(long pid, long new_state, long Tick, const char* fun_name, int line_num)
{
	if(task_trans_count >= MAX_TASK_TRANS)
		return;
		
	task_trans_table[task_trans_count].pid = pid;
	task_trans_table[task_trans_count].new_state = new_state;
	task_trans_table[task_trans_count].Tick = Tick;
	strcpy(task_trans_table[task_trans_count].fun_name, fun_name);
	task_trans_table[task_trans_count].line_num = line_num;
	
	task_trans_count++;
}

#endif
