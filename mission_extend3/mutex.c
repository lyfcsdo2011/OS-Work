/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: mutex.c

描述: 进程同步对象之互斥信号量的实现。



*******************************************************************************/

#include "psp.h"


VOID
PsInitializeMutex(
	IN PMUTEX Mutex,
	IN BOOL InitialOwner
	)
/*++

功能描述：
	初始化互斥信号量结构体。

参数：
	Mutex -- 互斥信号量结构体的指针。
	InitialOwner -- 互斥信号量的初始拥有者线程的指针。

返回值：
	无。

--*/
{
	ASSERT(KeGetIntNesting() == 0);

	if (InitialOwner) {
		Mutex->OwnerThread = PspCurrentThread;
		Mutex->RecursionCount = 1;
	} else {
		Mutex->OwnerThread = NULL;
		Mutex->RecursionCount = 0;
	}

	ListInitializeHead(&Mutex->WaitListHead);
}

STATUS
PsWaitForMutex(
	IN PMUTEX Mutex,
	IN ULONG Milliseconds
	)
/*++

功能描述：
	阻塞等待互斥信号量（P操作）。

参数：
	Mutex -- 信号量结构体的指针。
	Milliseconds -- 等待时间上限，单位毫秒。如果等待超时则返回 STATUS_TIMEOUT。
		如果为 INFINIT 则永久等待直到成功。

返回值：
	STATUS_SUCCESS -- 等待成功；
	STATUS_TIMEOUT -- 等待超时。

--*/
{
	STATUS Status;
	BOOL IntState = KeEnableInterrupts(FALSE);

	ASSERT(KeGetIntNesting() == 0);

	//
	// 如果 MUTEX 是空闲的，则设置其拥有者为当前线程并设置其递归计数器为 1；
	// 如果当前线程本就是 MUTEX 的拥有者，则增加 MUTEX 的递归计数器；
	// 如果 MUTEX 被其它线程占有，则将当前线程加入到 MUTEX 的等待队列中。
	//
	if (NULL == Mutex->OwnerThread) {

		Mutex->OwnerThread = PspCurrentThread;
		Mutex->RecursionCount = 1;
		Status = STATUS_SUCCESS;

	} else if (PspCurrentThread == Mutex->OwnerThread) {

		Mutex->RecursionCount++;
		Status = STATUS_SUCCESS;

	} else {

		Status = PspWait(&Mutex->WaitListHead, Milliseconds);
	}

	KeEnableInterrupts(IntState);

	return Status;
}

STATUS
PsReleaseMutex(
	IN PMUTEX Mutex
	)
/*++

功能描述：
	释放互斥信号量。注意：释放互斥信号量的线程必须是互斥信号量的拥有者线程。

参数：
	Mutex -- 互斥信号量结构体的指针。

返回值：
	STATUS_SUCCESS -- Release操作成功；
	STATUS_MUTEX_NOT_OWNED -- 当前调用线程非互斥信号量的拥有者。

--*/
{
	STATUS Status;
	BOOL IntState = KeEnableInterrupts(FALSE);

	ASSERT(KeGetIntNesting() == 0);

	if (PspCurrentThread == Mutex->OwnerThread) {

		//
		// 减小递归计数器，如果变为0则释放互斥信号量。
		//
		if (0 == --Mutex->RecursionCount) {

			//
			// 唤醒一个等待获得互斥信号量的线程，并将之设置为信号量的拥有者线程。
			//
			Mutex->OwnerThread = PspWakeThread(&Mutex->WaitListHead, STATUS_SUCCESS);

			if(NULL != Mutex->OwnerThread) {

				Mutex->RecursionCount = 1;

				//
				// 有线程被唤醒，执行线程调度。
				//
				PspThreadSchedule();
			}
		}

		Status = STATUS_SUCCESS;

	} else {

		Status = STATUS_MUTEX_NOT_OWNED;
	}

	KeEnableInterrupts(IntState);

	return Status;
}

//////////////////////////////////////////////////////////////////////////
//
// 下面是和互斥信号量对象类型相关的代码。
//

//
// 互斥信号量对象类型指针。
//
POBJECT_TYPE PspMutexType = NULL;

VOID
PspCreateMutexObjectType(
	VOID
	)
/*++

功能描述：
	创建互斥信号量对象类型。

参数：
	无。

返回值：
	无。

--*/
{
	STATUS Status;
	OBJECT_TYPE_INITIALIZER Initializer;

	Initializer.Create = (OB_CREATE_METHOD)PsInitializeMutex;
	Initializer.Delete = NULL;
	Initializer.Wait = (OB_WAIT_METHOD)PsWaitForMutex;
	Initializer.Read = NULL;
	Initializer.Write = NULL;

	Status = ObCreateObjectType("MUTEX", &Initializer, &PspMutexType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create mutex object type!");
	}
}

STATUS
PsCreateMutexObject(
	IN BOOL InitialOwner,
	IN PCSTR MutexName,
	OUT PHANDLE MutexHandle
	)
/*++

功能描述：
	创建互斥信号量对象。

参数：
	InitialOwner -- 如果为TRUE则初始化当前调用线程为新创建互斥信号量对象的拥有者。
	MutexName -- 名称字符串指针，如果为NULL则创建一个匿名对象，否则尝试打开已存在的命
		名对象，如果命名对象不存在则创建一个新的命名对象。
	MutexHandle -- 输出新创建或打开的互斥信号量对象句柄。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PVOID MutexObject;

	//
	// 创建MUTEX对象。
	//
	Status = ObCreateObject( PspMutexType,
							 MutexName,
							 sizeof(MUTEX),
							 InitialOwner,
							 &MutexObject );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	Status = ObCreateHandle(MutexObject, MutexHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(MutexObject);
	}
		
	return Status;
}

STATUS
PsReleaseMutexObject(
	IN HANDLE Handle
	)
/*++

功能描述：
	释放互斥信号量对象。

参数：
	Handle -- 互斥信号量对象的句柄。

返回值：
	STATUS_SUCCESS -- Release操作成功；
	STATUS_INVALID_HANDLE -- 句柄不是一个有效的互斥信号量对象句柄；
	STATUS_MUTEX_NOT_OWNED -- 当前调用线程非互斥信号量对象的拥有者。

--*/
{
	STATUS Status;
	PMUTEX Mutex;

	//
	// 由事件对象句柄得到对象指针。
	//
	Status = ObRefObjectByHandle(Handle, PspMutexType, (PVOID*)&Mutex);

	if (EOS_SUCCESS(Status)) {

		PsReleaseMutex(Mutex);

		//
		// 关闭对象指针。
		//
		ObDerefObject(Mutex);
	}

	return Status;
}
