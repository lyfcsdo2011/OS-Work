/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: psspnd.c

描述: 挂起线程的简单实现——只能将处于就绪状态的线程挂起，或者将挂起的线程
      恢复为就绪状态。



*******************************************************************************/

#include "psp.h"


//
// 挂起线程队列的头（已初始化为空队列）
//
LIST_ENTRY SuspendListHead = {&SuspendListHead, &SuspendListHead};


STATUS
PsSuspendThread(
	IN HANDLE hThread
	)
/*++

功能描述：
	挂起指定的线程。目前只能将处于就绪状态的线程挂起。

参数：
	hThread - 需要被挂起的线程的句柄。

返回值：
	如果成功则返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	BOOL IntState;
	PTHREAD Thread;

	//
	// 根据线程句柄获得线程对象的指针
	//
	Status = ObRefObjectByHandle(hThread, PspThreadType, (PVOID*)&Thread);

	if (EOS_SUCCESS(Status)) {
	
		IntState = KeEnableInterrupts(FALSE);	// 关中断

		if (Ready == Thread->State) {

			//
			// 将处于就绪状态（Ready）的线程从就绪队列中移除，
			// 从而使该线程进入游离状态（Zero）。
			//
			PspUnreadyThread(Thread);
			
			//
			// 将处于游离状态的线程插入挂起线程队列的末尾，
			// 挂起线程的操作就完成了。线程由活动就绪状态（Active Ready）进入静止就绪状态（Static Ready）。
			//
			ListInsertTail(&SuspendListHead, &Thread->StateListEntry);

			Status = STATUS_SUCCESS;
			
		} else {
		
			Status = STATUS_NOT_SUPPORTED;
			
		}

		KeEnableInterrupts(IntState);	// 开中断

		ObDerefObject(Thread);
	}

	return Status;
}

STATUS
PsResumThread(
	IN HANDLE hThread
	)
/*++

功能描述：
	恢复指定的线程。

参数：
	hThread - 需要被恢复的线程的句柄。

返回值：
	如果成功则返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	BOOL IntState;
	PTHREAD Thread;

	//
	// 根据线程句柄获得线程对象的指针
	//
	Status = ObRefObjectByHandle(hThread, PspThreadType, (PVOID*)&Thread);

	if (EOS_SUCCESS(Status)) {

		IntState = KeEnableInterrupts(FALSE);	// 关中断

		if (Zero == Thread->State) {

			//
			// 在此添加代码将线程恢复为就绪状态
			//
			
			Status = STATUS_SUCCESS;
			
		} else {
		
			Status = STATUS_NOT_SUPPORTED;
			
		}

		KeEnableInterrupts(IntState);	// 开中断

		ObDerefObject(Thread);
	}

	return Status;
}
