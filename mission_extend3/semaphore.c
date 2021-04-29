/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: semaphore.c

描述: 进程同步对象之记录型信号量的实现。



*******************************************************************************/

#include "psp.h"

VOID
PsInitializeSemaphore(
	IN PSEMAPHORE Semaphore,
	IN LONG InitialCount,
	IN LONG MaximumCount
	)
/*++

功能描述：
	初始化信号量结构体。

参数：
	Semaphore -- 要初始化的信号量结构体指针。
	InitialCount -- 信号量的初始值，不能小于 0 且不能大于 MaximumCount。
	MaximumCount -- 信号量的最大值，必须大于 0。

返回值：
	无。

--*/
{
	ASSERT(InitialCount >= 0 && InitialCount <= MaximumCount && MaximumCount > 0);

	Semaphore->Count = InitialCount;
	Semaphore->MaximumCount = MaximumCount;
	ListInitializeHead(&Semaphore->WaitListHead);
}

STATUS
PsWaitForSemaphore(
	IN PSEMAPHORE Semaphore,
	IN ULONG Milliseconds
	)
/*++

功能描述：
	信号量的 Wait 操作（P 操作）。

参数：
	Semaphore -- Wait 操作的信号量对象。
	Milliseconds -- 等待超时上限，单位毫秒。

返回值：
	STATUS_SUCCESS。
	当你修改信号量使之支持超时唤醒功能后，如果等待超时，应该返回 STATUS_TIMEOUT。

--*/
{
	BOOL IntState;

	ASSERT(KeGetIntNesting() == 0); // 中断环境下不能调用此函数。

	IntState = KeEnableInterrupts(FALSE); // 开始原子操作，禁止中断。

	//
	// 目前仅实现了标准记录型信号量，不支持超时唤醒功能，所以 PspWait 函数
	// 的第二个参数的值只能是 INFINITE。
	//
	Semaphore->Count--;
	if (Semaphore->Count < 0) {
		PspWait(&Semaphore->WaitListHead, INFINITE);
	}

	KeEnableInterrupts(IntState); // 原子操作完成，恢复中断。

	return STATUS_SUCCESS;
}

STATUS
PsReleaseSemaphore(
	IN PSEMAPHORE Semaphore,
	IN LONG ReleaseCount,
	OUT PLONG PreviousCount
	)
/*++

功能描述：
	信号量的 Signal 操作（V 操作）。

参数：
	Semaphore -- Wait 操作的信号量对象。
	ReleaseCount -- 信号量计数增加的数量。当前只能为 1。当你修改信号量使之支持
					超时唤醒功能后，此参数的值能够大于等于 1。
	PreviousCount -- 返回信号量计数在增加之前的值。

返回值：
	如果成功释放信号量，返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE); // 开始原子操作，禁止中断。

	if (Semaphore->Count + ReleaseCount > Semaphore->MaximumCount) {

		Status = STATUS_SEMAPHORE_LIMIT_EXCEEDED;

	} else {

		//
		// 记录当前的信号量的值。
		//
		if (NULL != PreviousCount) {
			*PreviousCount = Semaphore->Count;
		}

		//
		// 目前仅实现了标准记录型信号量，每执行一次信号量的释放操作
		// 只能使信号量的值增加 1。
		//
		Semaphore->Count++;
		if (Semaphore->Count <= 0) {
			PspWakeThread(&Semaphore->WaitListHead, STATUS_SUCCESS);
		}

		//
		// 可能有线程被唤醒，执行线程调度。
		//
		PspThreadSchedule();

		Status = STATUS_SUCCESS;
	}

	KeEnableInterrupts(IntState); // 原子操作完成，恢复中断。

	return Status;
}

//////////////////////////////////////////////////////////////////////////
//
// 下面是和信号量对象类型相关的代码。
//

//
// 信号量对象类型指针。
//
POBJECT_TYPE PspSemaphoreType = NULL;

//
// 用于初始化 semaphore 结构体的参数结构体。
//
typedef struct _SEM_CREATE_PARAM{
	LONG InitialCount;
	LONG MaximumCount;
}SEM_CREATE_PARAM, *PSEM_CREATE_PARAM;

//
// semaphore 对象的构造函数，在创建新 semaphore 对象时被调用。
//
VOID
PspOnCreateSemaphoreObject(
	IN PVOID SemaphoreObject,
	IN ULONG_PTR CreateParam
	)
{
	PsInitializeSemaphore( (PSEMAPHORE)SemaphoreObject, 
						   ((PSEM_CREATE_PARAM)CreateParam)->InitialCount,
						   ((PSEM_CREATE_PARAM)CreateParam)->MaximumCount );
}

//
// semaphore 对象类型的初始化函数。
//
VOID
PspCreateSemaphoreObjectType(
	VOID
	)
{
	STATUS Status;
	OBJECT_TYPE_INITIALIZER Initializer;

	Initializer.Create = PspOnCreateSemaphoreObject;
	Initializer.Delete = NULL;
	Initializer.Wait = (OB_WAIT_METHOD)PsWaitForSemaphore;
	Initializer.Read = NULL;
	Initializer.Write = NULL;
	
	Status = ObCreateObjectType("SEMAPHORE", &Initializer, &PspSemaphoreType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create semaphore object type!");
	}
}
 
//
// semaphore 对象的构造函数。
//
STATUS
PsCreateSemaphoreObject(
	IN LONG InitialCount,
	IN LONG MaximumCount,
	IN PSTR Name,
	OUT PHANDLE SemaphoreHandle
	)
{
	STATUS Status;
	PVOID SemaphoreObject;
	SEM_CREATE_PARAM CreateParam;

	if(InitialCount < 0 || MaximumCount <= 0 || InitialCount > MaximumCount){
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 创建信号量对象。
	//
	CreateParam.InitialCount = InitialCount;
	CreateParam.MaximumCount = MaximumCount;

	Status = ObCreateObject( PspSemaphoreType,
							 Name,
							 sizeof(SEMAPHORE),
							 (ULONG_PTR)&CreateParam,
							 &SemaphoreObject);

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	Status = ObCreateHandle(SemaphoreObject, SemaphoreHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(SemaphoreObject);
	}

	return Status;
}

//
// semaphore 对象的 signal 操作函数。
//
STATUS
PsReleaseSemaphoreObject(
	IN HANDLE Handle,
	IN LONG ReleaseCount,
	IN PLONG PreviousCount
	)
{
	STATUS Status;
	PSEMAPHORE Semaphore;

	if (ReleaseCount < 1) {
		return STATUS_INVALID_PARAMETER;
	}

	// 由 semaphore 句柄得到 semaphore 对象的指针。
	Status = ObRefObjectByHandle(Handle, PspSemaphoreType, (PVOID*)&Semaphore);

	if (EOS_SUCCESS(Status)) {
		Status = PsReleaseSemaphore(Semaphore, ReleaseCount, PreviousCount);
		ObDerefObject(Semaphore);
	}

	return Status;
}

