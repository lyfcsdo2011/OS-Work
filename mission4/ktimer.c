/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: ktimer.c

����: �ں˶�ʱ����ʵ�֡�



*******************************************************************************/

#include "ki.h"

//
// ��ʱ������ͷ�����
//
static LIST_ENTRY KiTimerListHead = {&KiTimerListHead, &KiTimerListHead};

//
// ��һ��Ҫ����ļ�ʱ����������ָ�롣
//
static volatile PLIST_ENTRY KiNextTimerListEntry = NULL;

volatile long Tick = 0;

//
// ʱ��Ƭ��ת���Ⱥ���ԭ�͡�
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

����������
	��ʱ��������PIT: Programmable Interval Timer�����жϷ������

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	PKTIMER Timer;

	++Tick;
	//
	// ������ʱ������
	//
	KiNextTimerListEntry = KiTimerListHead.Next;
	while (KiNextTimerListEntry != &KiTimerListHead) {

		Timer = CONTAINING_RECORD(KiNextTimerListEntry, KTIMER, TimerListEntry);

		//
		// ����ڵ��ûص�ǰ��������ָ����ƣ���Ϊ�ص�������ִ��ʱ���ܻ�ע���˼�ʱ����
		//
		KiNextTimerListEntry = KiNextTimerListEntry->Next;

		//
		// ���Ӽ�ʱ���Ѿ�ʹ�õ�ʱ��
		//
		Timer->ElapsedTicks++;

		//
		// ���ʱ�䵽�������¿�ʼ��ʱ�����ü�ʱ���ص�������
		// ע�⣺һ��Ҫ�����¿�ʼ��ʱ��Ȼ���ٵ��ü�ʱ���ص�������
		// ��Ϊ�ص�������ִ��ʱ���ܻ�ע����ʱ����
		//
		if (Timer->IntervalTicks == Timer->ElapsedTicks) {

			Timer->ElapsedTicks = 0;
			Timer->TimerRoutine(Timer->Parameter);
		}
	}
	KiNextTimerListEntry = NULL;

	//
	// ʱ��Ƭ��ת���ȡ�
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

����������
	��ʼ���ں˼�ʱ���ṹ�塣

������
	Timer -- �ں˼�ʱ���ṹ��ָ�롣
	Milliseconds -- ��ʱʱ��������λ���롣��ʱ�����СKTIMER_MINIMUM�����
		KTIMER_MAXIMUM��
	TimerRoutine -- ��ʱʱ�䵽���Ҫ���õĻص��������ص������ڼ�ʱ���ж��б����á�
	Parameter -- Ҫ���ݸ��ص������Ĳ�����

����ֵ��
	�ޡ�

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
	// ����λΪ�����ʱ��ת��Ϊϵͳ�ڲ���ʱ����������
	// ��ʼ�� PIT ʱ������ʱ�������� 10ms���� 5 ��Ϊ���������롣
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

����������
	ע���ѳ�ʼ�����ں˼�ʱ������������ʱ�������ʱ�������ͷ����

������
	Timer -- �ں˼�ʱ���ṹ��ָ�롣

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
#ifdef _DEBUG
	PLIST_ENTRY ListEntry;
#endif

	IntState = KeEnableInterrupts(FALSE);

#ifdef _DEBUG
	//
	// ȷ�������ظ�ע��ͬһ����ʱ����
	//
	for (ListEntry = KiTimerListHead.Next; ListEntry != &KiTimerListHead; ListEntry = ListEntry->Next) {
		ASSERT(ListEntry != &Timer->TimerListEntry);
	}
#endif

	//
	// ��ʼ������������������ͷ��
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

����������
	ע���ں˼�ʱ������������ʱ���Ӽ�ʱ���������Ƴ���

������
	Timer -- �ں˼�ʱ���ṹ��ָ�롣

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
#ifdef _DEBUG
	PLIST_ENTRY ListEntry;
#endif

	IntState = KeEnableInterrupts(FALSE);

#ifdef _DEBUG
	//
	// ȷ������ע��ļ�ʱ����
	//
	for (ListEntry = KiTimerListHead.Next; ListEntry != &KiTimerListHead; ListEntry = ListEntry->Next) {
		if (ListEntry == &Timer->TimerListEntry) {
			break;
		}
	}

	ASSERT(ListEntry != &KiTimerListHead);
#endif

	//
	// ��һ����ʱ���Ļص�������ִ��ʱ���п��ܻ�ע����һ��Ҫ����ļ�ʱ����
	// ������Ҫһ��ȫ�ֵ� KiNextTimerListEntry ָ����һ��Ҫ����ļ�ʱ����
	//
	if(KiNextTimerListEntry == &Timer->TimerListEntry) {
		KiNextTimerListEntry = KiNextTimerListEntry->Next;
	}
	
	ListRemoveEntry(&Timer->TimerListEntry);

	KeEnableInterrupts(IntState);
}
