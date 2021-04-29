/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: delete.c

描述: 线程、进程结束以及相关函数的实现。



*******************************************************************************/

#include "psp.h"

extern LIST_ENTRY PspTerminatedListHead;

STATUS
PsTerminateProcess(
	IN HANDLE Handle,
	IN ULONG ExitCode
	)
/*++

功能描述：
	结束指定的进程，可以在进程或者中断中调用。

参数：
	Handle -- 被结束进程的句柄。
	ExitCode -- 进程结束码。

返回值：
	如果成功则返回STATUSS_SUCESS。

--*/
{
	STATUS Status;
	PPROCESS Process;

	Status = ObRefObjectByHandle(Handle, PspProcessType, (PVOID*)&Process);

	if (EOS_SUCCESS(Status)) {

		if (Process == PspCurrentProcess) {
			ObDerefObject(Process);
		}

		PspTerminateProcess(Process, ExitCode);

		//
		// 如果当前进程被结束，下面的代码就没有机会执行了。
		//
		ObDerefObject(Process);
	}

	return Status;
}

STATUS
PsTerminateThread(
	IN HANDLE Handle,
	IN ULONG ExitCode
	)
/*++

功能描述：
	结束指定的线程。

参数：
	Handle -- 被结束线程的句柄。
	ExitCode -- 线程结束码。

返回值：
	如果成功则返回STATUSS_SUCESS。

--*/
{
	STATUS Status;
	PTHREAD Thread;

	Status = ObRefObjectByHandle(Handle, PspThreadType, (PVOID*)&Thread);

	if (EOS_SUCCESS(Status)) {

		if (Thread == PspCurrentThread || Thread == PspCurrentProcess->PrimaryThread) {
			ObDerefObject(Thread);
		}

		PspTerminateThread(Thread, ExitCode, FALSE);

		//
		// 如果当前线程或当前进程被结束，下面的代码就没有机会再执行了。
		//
		ObDerefObject(Thread);
	}

	return Status;
}

VOID
PsExitProcess(
	IN ULONG ExitCode
	)
/*++

功能描述：
	退出当前进程。

参数：
	ExitCode -- 退出码。

返回值：
	无。

--*/
{
	ASSERT(KeGetIntNesting() == 0);
	PspTerminateProcess(PspCurrentProcess, ExitCode);
}

VOID
PsExitThread(
	IN ULONG ExitCode
	)
/*++

功能描述：
	退出当前线程。

参数：
	ExitCode -- 进程结束码。

返回值：
	无。

--*/
{
	ASSERT(KeGetIntNesting() == 0);
	PspTerminateThread(PspCurrentThread, ExitCode, FALSE);
}

VOID
PspDeleteProcessEnvironment(
	IN PPROCESS Process
	)
{
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	//
	// 进程的线程必须已经全部被结束或者根本还没有创建线程。
	//
	ASSERT(NULL == Process->PrimaryThread);

	//
	// 不能对同一个进程对象重复调用这个函数。
	//
	ASSERT(NULL != Process->ObjectTable && NULL != Process->Pas);

	//
	// 释放句柄表（关闭所有已打开句柄）。
	//
	ObFreeHandleTable(Process->ObjectTable);
	Process->ObjectTable = NULL;

	//
	// 当前线程附着到被结束进程的地址空间中执行，以执行清理操作。
	//
	PspThreadAttachProcess(Process);

	//
	// 清理进程用户地址空间中的虚拟内存。
	//
	MmCleanVirtualMemory();

	//
	// 当前线程返回到自己所属进程的地址空间中继续执行。
	// 注意：如果当前线程属于被结束进程则附着到系统进程地址空间中继续执行。
	//
	if (PspCurrentProcess == Process) {
		PspThreadAttachProcess(PspSystemProcess);
	} else {
		PspThreadAttachProcess(PspCurrentProcess);
	}

	//
	// 删除进程地址空间。
	//
	MmDeleteProcessAddressSpace(Process->Pas);
	Process->Pas = NULL;

	//
	// 进程已经被彻底删除，不再保留对自己的引用。
	//
	ObDerefObject(Process);

	KeEnableInterrupts(IntState);
}

VOID
PspTerminateProcess(
	IN PPROCESS Process,
	IN ULONG ExitCode
	)
/*++

功能描述：
	结束指定进程。

参数：
	Process -- 进程对象指针。
	ExitCode -- 进程结束码。

返回值：
	无。

--*/
{
	BOOL IntState;
	PTHREAD Thread;

	IntState = KeEnableInterrupts(FALSE);

	if (NULL != Process->PrimaryThread) {

		//
		// 设置进程结束标志（主线程指针为NULL）和结束码。
		//
		Process->PrimaryThread = NULL;
		Process->ExitCode = ExitCode;

		//
		// 唤醒等待进程结束的所有线程。
		//
		while (!ListIsEmpty(&Process->WaitListHead)) {
			PspWakeThread(&Process->WaitListHead, STATUS_SUCCESS);
		}

		//
		// 结束进程内的所有线程。
		// 注意：并不急于在结束每个线程后都立刻执行线程调度，所有线程都被结束后再执
		// 行一次调度即可。
		//
		while (!ListIsEmpty(&Process->ThreadListHead)) {
			Thread = CONTAINING_RECORD(Process->ThreadListHead.Next, THREAD, ThreadListEntry);
			PspTerminateThread(Thread, ExitCode, TRUE);
		}

		//
		// 删除进程环境。
		//
		PspDeleteProcessEnvironment(Process);

		//
		// 执行线程调度。
		//
		PspThreadSchedule();
	}

	KeEnableInterrupts(IntState);
}

VOID
PspTerminateThread(
	IN PTHREAD Thread,
	IN ULONG ExitCode,
	IN BOOL IsTerminatingProcess
	)
/*++

功能描述：
	结束指定的线程。

参数：
	Thread -- 目标线程对象指针。
	ExitCode -- 线程结束码。
	IsTerminatingProcess -- 正在执行结束进程操作。

返回值：
	无。

--*/
{
	STATUS Status;
	BOOL IntState;
	SIZE_T StackSize;

	IntState = KeEnableInterrupts(FALSE);

	ASSERT(Thread->State != Zero);

	if (Thread->State != Terminated) {

		if (Thread == Thread->Process->PrimaryThread) {

			//
			// 被结束线程是所在进程的主线程，结束线程所在的整个进程。
			//
			PspTerminateProcess(Thread->Process, ExitCode);

		} else {

			//
			// 唤醒等待线程结束的所有线程。
			//
			while (!ListIsEmpty(&Thread->WaitListHead)) {
				PspWakeThread(&Thread->WaitListHead, STATUS_SUCCESS);
			}

			//
			// 线程脱离目前所处状态并转入结束状态。
			//
			if(Ready == Thread->State) {
				PspUnreadyThread(Thread);
			} else if (Waiting == Thread->State) {
				PspUnwaitThread(Thread);
			}

			Thread->State = Terminated;	
	
#ifdef	_DEBUG	
			RECORD_TASK_STATE(ObGetObjectId(Thread) , TS_STOPPED, Tick);
#endif
				
			ListInsertTail(&PspTerminatedListHead, &Thread->StateListEntry);

			//
			// 设置线程结束码并将线程从进程的线程链表中移除。
			//
			Thread->ExitCode = ExitCode;
			ListRemoveEntry(&Thread->ThreadListEntry);

			//
			// 释放线程的内核模式栈。
			// 注意：如果当前线程正在结束自己，则不能释放线程正在使用的内核栈。
			//
			if (Thread != PspCurrentThread) {

				StackSize = 0;

				Status = MmFreeVirtualMemory( &Thread->KernelStack,
											  &StackSize,
											  MEM_RELEASE,
											  TRUE );
				ASSERT(EOS_SUCCESS(Status));
			}

			//
			// 如果被删除线程不是系统线程，还需释放线程的用户模式栈。目前所有线
			// 程都执行在内核模式栈中，没有用户模式栈，忽略此操作。
			//
			/*if (!Thread->Process->System) {

				//
				// 用户模式栈在进程的用户地址空间中，当前线程需要附着到被结束线
				// 程所属进程的地址空间中执行。
				//
				PspThreadAttachProcess(Thread->Process);

				//
				// 释放被删除线程的用户模式栈。
				//
				
				//
				// 当前线程重新返回到自己所属进程的地址空间中执行。
				//
				PspThreadAttachProcess(PspCurrentProcess);
			}*/

			//
			// 线程结束，线程不再保留对自己的引用。
			//
			ObDerefObject(Thread);	

			//
			// 注意：正在结束进程时不需要执行线程调度，因为结束进程函数最后会执行线程调度。
			//
			if (!IsTerminatingProcess) {
				PspThreadSchedule();
			}
		}
	}

	KeEnableInterrupts(IntState);
}

STATUS
PspOnWaitForProcessObject(
	IN PVOID ProcessObject,
	IN ULONG Milliseconds
	)
/*++

功能描述：
	等待进程结束。

参数：
	ProcessObject -- 进程对象指针。
	Milliseconds -- 等待时间上限，如果等待超时则返回STATUS_TIMEOUT。如果为INFINIT
		则永久等待直到等待成功。

返回值：
	STATUS_SUCCESS -- 等待成功；
	STATUS_TIMEOUT -- 等待超时。

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	//
	// 如果进程已经结束则立刻返回，否则在进程的等待队列中等待直到进程结束。
	//
	if (NULL == ((PPROCESS)ProcessObject)->PrimaryThread) {
		Status = STATUS_SUCCESS;
	} else {
		Status = PspWait(&((PPROCESS)ProcessObject)->WaitListHead, Milliseconds);
	}

	KeEnableInterrupts(IntState);

	return Status;
}

STATUS
PspOnWaitForThreadObject(
	IN PVOID ThreadObject,
	IN ULONG Milliseconds
	)
/*++

功能描述：
	等待线程结束。

参数：
	ThreadObject -- 线程对象指针。
	Milliseconds -- 等待时间上限，如果等待超时则返回STATUS_TIMEOUT。如果为INFINIT
		则永久等待直到等待成功。

返回值：
	STATUS_SUCCESS -- 等待成功；
	STATUS_TIMEOUT -- 等待超时。

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	ASSERT(Zero != ((PTHREAD)ThreadObject)->State);

	//
	// 如果被结束线程已经结束则立刻返回，否则在被结束线程的等待队列中等待。
	//
	if (Terminated == ((PTHREAD)ThreadObject)->State) {
		Status = STATUS_SUCCESS;
	} else {
		Status = PspWait(&((PTHREAD)ThreadObject)->WaitListHead, Milliseconds);
	}

	KeEnableInterrupts(IntState);

	return Status;
}

VOID
PspOnDeleteThreadObject(
	IN PVOID ThreadObject
	)
/*++

功能描述：
	线程对象的析构函数，在删除线程对象时被调用。

参数：
	ThreadObject -- 线程对象的指针。

返回值：
	无。

--*/
{
	ASSERT( Zero == ((PTHREAD)ThreadObject)->State ||
			Terminated == ((PTHREAD)ThreadObject)->State );

	//
	// 将处于结束状态的线程对象从结束状态队列中移除。
	//
	if (Terminated == ((PTHREAD)ThreadObject)->State) {
		ListRemoveEntry(&((PTHREAD)ThreadObject)->StateListEntry);
	}
}
