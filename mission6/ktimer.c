/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: ktimer.c

描述: 内核定时器的实现。



*******************************************************************************/

#include "ki.h"

//
// 计时器链表头链表项。
//
static LIST_ENTRY KiTimerListHead = {&KiTimerListHead, &KiTimerListHead};

//
// 下一个要处理的计时器的链表项指针。
//
static volatile PLIST_ENTRY KiNextTimerListEntry = NULL;

volatile long Tick = 0;

//
// 时间片轮转调度函数原型。
//
VOID
PspRoundRobin(
	VOID
	);

VOID
KiIsrTimer(
	VOID
	)
/*++

功能描述：
	定时计数器（PIT: Programmable Interval Timer）的中断服务程序。

参数：
	无。

返回值：
	无。

--*/
{
	BOOL IntState;
	PKTIMER Timer;

	++Tick;
	//
	// 遍历计时器链表。
	//
	KiNextTimerListEntry = KiTimerListHead.Next;
	while (KiNextTimerListEntry != &KiTimerListHead) {

		Timer = CONTAINING_RECORD(KiNextTimerListEntry, KTIMER, TimerListEntry);

		//
		// 务必在调用回调前将链表项指针后移，因为回调函数在执行时可能会注销此计时器。
		//
		KiNextTimerListEntry = KiNextTimerListEntry->Next;

		//
		// 增加计时器已经使用的时间
		//
		Timer->ElapsedTicks++;

		//
		// 如果时间到达则重新开始计时并调用计时器回调函数。
		// 注意：一定要先重新开始计时，然后再调用计时器回调函数，
		// 因为回调函数在执行时可能会注销计时器。
		//
		if (Timer->IntervalTicks == Timer->ElapsedTicks) {

			Timer->ElapsedTicks = 0;
			Timer->TimerRoutine(Timer->Parameter);
		}
	}
	KiNextTimerListEntry = NULL;

	//
	// 时间片轮转调度。
	//
	IntState = KeEnableInterrupts(FALSE);
	PspRoundRobin();
	KeEnableInterrupts(IntState);
}

VOID
KeInitializeTimer(
	IN PKTIMER Timer,
	IN ULONG Milliseconds,
	IN PKTIMER_ROUTINE TimerRoutine,
	IN ULONG_PTR Parameter
	)
/*++

功能描述：
	初始化内核计时器结构体。

参数：
	Timer -- 内核计时器结构体指针。
	Milliseconds -- 计时时间间隔，单位毫秒。计时间隔最小KTIMER_MINIMUM，最大
		KTIMER_MAXIMUM。
	TimerRoutine -- 计时时间到达后要调用的回调函数。回调函数在计时器中断中被调用。
	Parameter -- 要传递给回调函数的参数。

返回值：
	无。

--*/
{
	ASSERT(NULL != Timer);
	ASSERT(NULL != TimerRoutine);
	ASSERT(Milliseconds >= KTIMER_MINIMUM && Milliseconds <= KTIMER_MAXIMUM);

	if (Milliseconds > KTIMER_MAXIMUM) {
		Milliseconds = KTIMER_MAXIMUM;
	} else if (Milliseconds < KTIMER_MINIMUM) {
		Milliseconds = KTIMER_MINIMUM;
	}

	//
	// 将单位为毫秒的时间转换为系统内部的时钟周期数。
	// 初始化 PIT 时设置了时钟周期是 10ms。加 5 是为了四舍五入。
	//
	Timer->IntervalTicks = (Milliseconds + 5) / 10;
	
	Timer->TimerRoutine = TimerRoutine;
	Timer->Parameter = Parameter;
}

VOID
KeRegisterTimer(
	IN PKTIMER Timer
	)
/*++

功能描述：
	注册已初始化的内核计时器。仅仅将计时器插入计时器链表的头部。

参数：
	Timer -- 内核计时器结构体指针。

返回值：
	无。

--*/
{
	BOOL IntState;
#ifdef _DEBUG
	PLIST_ENTRY ListEntry;
#endif

	IntState = KeEnableInterrupts(FALSE);

#ifdef _DEBUG
	//
	// 确保不会重复注册同一个计时器。
	//
	for (ListEntry = KiTimerListHead.Next; ListEntry != &KiTimerListHead; ListEntry = ListEntry->Next) {
		ASSERT(ListEntry != &Timer->TimerListEntry);
	}
#endif

	//
	// 初始化计数器并插入链表头。
	//
	Timer->ElapsedTicks = 0;
	ListInsertHead(&KiTimerListHead, &Timer->TimerListEntry);

	KeEnableInterrupts(IntState);
}

VOID
KeUnregisterTimer(
	IN PKTIMER Timer
	)
/*++

功能描述：
	注销内核计时器。仅仅将计时器从计时器链表中移除。

参数：
	Timer -- 内核计时器结构体指针。

返回值：
	无。

--*/
{
	BOOL IntState;
#ifdef _DEBUG
	PLIST_ENTRY ListEntry;
#endif

	IntState = KeEnableInterrupts(FALSE);

#ifdef _DEBUG
	//
	// 确保是已注册的计时器。
	//
	for (ListEntry = KiTimerListHead.Next; ListEntry != &KiTimerListHead; ListEntry = ListEntry->Next) {
		if (ListEntry == &Timer->TimerListEntry) {
			break;
		}
	}

	ASSERT(ListEntry != &KiTimerListHead);
#endif

	//
	// 当一个计时器的回调函数在执行时，有可能会注销下一个要处理的计时器，
	// 所以需要一个全局的 KiNextTimerListEntry 指向下一个要处理的计时器。
	//
	if(KiNextTimerListEntry == &Timer->TimerListEntry) {
		KiNextTimerListEntry = KiNextTimerListEntry->Next;
	}
	
	ListRemoveEntry(&Timer->TimerListEntry);

	KeEnableInterrupts(IntState);
}
