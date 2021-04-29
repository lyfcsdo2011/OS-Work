/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: event.c

描述: 进程同步对象之事件的实现。



*******************************************************************************/

#include "psp.h"

VOID
PsInitializeEvent(
	IN PEVENT Event,
	IN BOOL ManualReset,
	IN BOOL InitialState
	)
/*++

功能描述：
	初始化事件结构体。

参数：
	Event -- 事件结构体指针。
	ManualReset -- 是否初始化为手动事件。TRUE 为手动，FALSE 为自动。
	InitialState -- 事件初始化的状态。TRUE 为有效，FALSE 为无效。

返回值：
	无。

--*/
{
	Event->IsManual = ManualReset;
	Event->IsSignaled = InitialState;
	ListInitializeHead(&Event->WaitListHead);
}

STATUS
PsWaitForEvent(
	IN PEVENT Event,
	IN ULONG Milliseconds
	)
/*++

功能描述：
	阻塞等待事件直到事件变为 Signaled 状态。如果等待的是自动事件，成功等待后会复位
	事件为 Nonsignaled 状态。

参数：
	Event -- 事件结构体指针。
	Milliseconds -- 等待时间上限，如果等待超时则返回STATUS_TIMEOUT。如果为INFINIT
		则永久等待直到等待成功。

返回值：
	STATUS_SUCCESS表示等待成功，STATUS_TIMEOUT表示等待超时。

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	//
	// 如果事件处于 Signaled 状态那么立刻返回成功，否则当前线程阻塞等待事件。
	//
	if (Event->IsSignaled) {

		//
		// 如果是自动事件，那么成功等待的同时还要复位事件。
		//
		if (!Event->IsManual) {
			Event->IsSignaled = FALSE;
		}

		Status = STATUS_SUCCESS;

	} else {

		//
		// 按照FCFS原则阻塞在事件等待队列的队尾。
		//
		Status = PspWait(&Event->WaitListHead, Milliseconds);
	}

	KeEnableInterrupts(IntState);

	return Status;
}

VOID
PsSetEvent(
	IN PEVENT Event
	)
/*++

功能描述：
	使事件变为 Singnaled 状态。如果事件的等待队列上有线程正在等待，等待线程将被唤醒。

参数：
	Event -- 事件结构体指针。

返回值：
	无。

--*/
{
	BOOL IntState = KeEnableInterrupts(FALSE);

	//
	// 如果事件处于 Nonsignaled 状态，则修改事件为 Signaled 状态。
	//
	if (!Event->IsSignaled) {

		Event->IsSignaled = TRUE;

		while (Event->IsSignaled && !ListIsEmpty(&Event->WaitListHead)) {

			//
			// 事件处于 Signaled 状态且等待队列非空，按照 FCFS 的原则唤醒等待队列的队首线程。
			//
			PspWakeThread(&Event->WaitListHead, STATUS_SUCCESS);

			//
			// 如果是自动事件，那么在通知一个线程后事件即被复位（变为 Nonsignaled 状态）。
			//
			if (!Event->IsManual) {
				Event->IsSignaled = FALSE;
			}
		}

		//
		// 可能有线程被唤醒，执行线程调度。
		//
		PspThreadSchedule();
	}

	KeEnableInterrupts(IntState);
}

VOID
PsResetEvent(
	IN PEVENT Event
	)
/*++

功能描述：
	复位事件状态为 Nonsignaled 状态。

参数：
	Event -- 事件结构体指针。

返回值：
	无。

--*/
{
	BOOL IntState = KeEnableInterrupts(FALSE);

	Event->IsSignaled = FALSE;

	KeEnableInterrupts(IntState);
}

//////////////////////////////////////////////////////////////////////////
//
// 下面是和事件对象类型相关的代码。
//

//
// 事件对象类型指针。
//
POBJECT_TYPE PspEventType;

VOID
PspOnCreateEventObject(
	IN PVOID EventObject,
	IN ULONG_PTR CreateParam
	)
/*++

功能描述：
	事件对象的构造函数，被ObCreateObject调用。

参数：
	EventObject -- 新创建的事件对象的指针。
	CreateParam -- 构造参数，位0标志是否为手动事件，位1标志初始状态。

返回值：
	无。

--*/
{
	PsInitializeEvent( (PEVENT)EventObject,
					   (CreateParam & 0x1) != 0,
					   (CreateParam & 0x2) != 0 );
}

VOID
PspCreateEventObjectType(
	VOID
	)
/*++

功能描述：
	创建事件对象类型。

参数：
	无。

返回值：
	无。

--*/
{
	STATUS Status;
	OBJECT_TYPE_INITIALIZER Initializer;
	
	Initializer.Create = PspOnCreateEventObject;
	Initializer.Delete = NULL;
	Initializer.Wait = (OB_WAIT_METHOD)PsWaitForEvent;
	Initializer.Read = NULL;
	Initializer.Write = NULL;
	
	Status = ObCreateObjectType("EVENT", &Initializer, &PspEventType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create event object type!");
	}
}

STATUS
PsCreateEventObject(
	IN BOOL ManualReset,
	IN BOOL InitialState,
	IN PCSTR EventName,
	OUT PHANDLE EventHandle
	)
/*++

功能描述：
	创建事件对象。

参数：
	ManualReset -- 是否初始化为手动事件。
	InitialState -- 事件初始化的状态。
	EventName -- 名称字符串指针，如果为NULL则创建一个匿名对象，否则尝试打开已存在的命
		名对象，如果命名对象不存在则创建一个新的命名对象。
	EventHandle -- 输出新创建或打开的事件对象句柄。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PVOID EventObject;
	ULONG_PTR CreateParam = 0;

	if (ManualReset) {
		CreateParam |= 0x1;
	}

	if (InitialState) {
		CreateParam |= 0x2;
	}

	//
	// 创建事件对象。
	//
	Status = ObCreateObject( PspEventType,
							 EventName,
							 sizeof(EVENT),
							 CreateParam,
							 &EventObject );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	//
	// 为事件对象创建句柄。
	//
	Status = ObCreateHandle(EventObject, EventHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(EventObject);
	}

	return Status;
}

STATUS
PsSetEventObject(
	HANDLE Handle
	)
/*++

功能描述：
	使事件对象变为 Singnaled 状态。如果事件的等待队列上有线程正在等待，等待线程将被唤醒。

参数：
	Handle -- 事件对象的句柄。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PEVENT Event;

	//
	// 由事件对象句柄得到对象指针。
	//
	Status = ObRefObjectByHandle(Handle, PspEventType, (PVOID*)&Event);

	if (EOS_SUCCESS(Status)) {

		PsSetEvent(Event);

		//
		// 关闭事件对象指针。
		//
		ObDerefObject(Event);
	}

	return Status;
}

STATUS
PsResetEventObject(
	HANDLE Handle
	)
/*++

功能描述：
	复位事件对象状态为 Nonsignaled 状态。

参数：
	Handle -- 事件对象的句柄。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PEVENT Event;

	//
	// 由事件对象句柄得到对象指针。
	//
	Status = ObRefObjectByHandle(Handle, PspEventType, (PVOID*)&Event);

	if (EOS_SUCCESS(Status)) {

		PsResetEvent(Event);

		//
		// 关闭对象指针。
		//
		ObDerefObject(Event);
	}

	return Status;
}
