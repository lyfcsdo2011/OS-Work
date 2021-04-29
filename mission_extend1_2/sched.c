/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: sched.c

����: �̵߳��ȵ�ʵ�֡������߳�״̬��ת����



*******************************************************************************/

#include "psp.h"

//
// 32������ͷ��ɵ����飬�ֱ��Ӧ��0~31��32�����ȼ��ľ������С�
// �±�Ϊn�������Ӧ���ȼ�Ϊn�ľ������С�
//
LIST_ENTRY PspReadyListHeads[32];

//
// 32λ����λͼ��
// ���λͼ�ĵ�nλΪ1����������ȼ�Ϊn�ľ������зǿա�
//
volatile ULONG PspReadyBitmap = 0;

//
// ˯�ߵȴ��̶߳��С�
// �̵߳���Sleep������������н��еȴ���
//
LIST_ENTRY PspSleepingListHead;

//
// �ѽ����̶߳��С�
//
LIST_ENTRY PspTerminatedListHead;

//
// ��ǰ�����̡߳�
//
volatile PTHREAD PspCurrentThread = NULL;


VOID
PspInitSchedulingQueue(
	VOID
	)
/*++

����������
	��ʼ���̵߳���Ҫʹ�õĸ��ֶ��С�

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	INT i;

	//
	// ��ʼ��32�����ȼ���Ӧ��32���������С�
	//
	for(i = 0; i < 32; i++)
		ListInitializeHead(&PspReadyListHeads[i]);

	//
	// ��ʼ��˯�ߵȴ����С�
	//
	ListInitializeHead(&PspSleepingListHead);

	//
	// ��ʼ���ѽ����̶߳��С�
	//
	ListInitializeHead(&PspTerminatedListHead);
}

//
// �õ���ǰ�̶߳���ָ�롣
//
PVOID
PsGetCurrentThreadObject(
	VOID
	)
{
	return PspCurrentThread;
}

//
// �õ���ǰ���̶���ָ�롣
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

����������
	ʹ Zero ״̬��������״̬���߳�ת�����״̬��

������
	Thread -- �߳�ָ�롣

����ֵ��
	�ޡ�

--*/
{
	ASSERT(NULL != Thread);
	ASSERT(Zero == Thread->State || Running == Thread->State);

	//
	// ���̲߳��������ȼ���Ӧ�ľ������еĶ�β�������þ���λͼ�ж�Ӧ��λ��
	// ����̵߳�״̬�޸�Ϊ����״̬��
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

����������
	ȡ���̵߳ľ���״̬��ʹ�߳�ת�� Zero ״̬��

������
	Thread - ��ǰ���ھ���״̬���̵߳�ָ�롣

����ֵ��
	�ޡ�

--*/
{
	ASSERT(NULL != Thread && Ready == Thread->State);

	//
	// ���̴߳����ڵľ���������ȡ��������߳����ȼ���Ӧ�ľ������б�Ϊ�գ�
	// ���������λͼ�ж�Ӧ��λ��
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

����������
	�ȴ���ʱ��ʱ���ص����������ѵȴ���ʱ���̡߳�

������
	Param -- �ȴ���ʱ���̵߳�ָ�루��Ҫ����ǿ��ת������

����ֵ��
	��

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

����������
	��ǰ�̰߳���FCFS��ԭ�����ָ���ĵȴ����еĶ�β���߳������ȴ�ֱ���ȴ���ʱ��
	��PspWakeThread�����á�

������
	WaitListHead -- ������ĵȴ����е�ָ�롣
	Milliseconds -- ���޵ȴ�ʱ��(��λms)������ȴ�ʱ�䳬������ϵͳ�Զ����Ѳ�
		����STATUS_TIMEOUT�����Ϊ0������������STATUS_TIMEOUT�����ΪINFINIT��
		�����õȴ�ֱ��PspWakeThread�����á�

����ֵ��
	����̵߳ȴ���ʱ�򷵻�STATUS_TIMEOUT�����򷵻�PspWakeThread�ĵڶ�������
	WaitStatus��

--*/
{
	ASSERT(0 == KeGetIntNesting());
	ASSERT(Running == PspCurrentThread->State);
	ASSERT(0 != PspReadyBitmap);

	if(0 == Milliseconds) {
		return STATUS_TIMEOUT;
	}

	//
	// ����ǰ�̲߳���ȴ����еĶ�β���޸��߳�״̬��ΪWaiting��
	//
	ListInsertTail(WaitListHead, &PspCurrentThread->StateListEntry);
	PspCurrentThread->State = Waiting;
	
#ifdef _DEBUG
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_WAIT, Tick);
#endif

	//
	// ����������õȴ�����ע��һ�����ڳ�ʱ�����̵߳ĵȴ���ʱ����
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
	// ��ǰ�߳̽���ȴ�״̬����Ҫ�ó�����������Ȩ�ȴ�����ִ���̵߳��ȡ�
	//
	PspThreadSchedule();

	//
	// zzZ...
	//

	//
	// �̱߳����Ѽ���ִ�У����صȴ����״̬�롣
	//
	return PspCurrentThread->WaitStatus;
}

VOID
PspUnwaitThread(
	IN PTHREAD Thread
	)
/*++

����������
	ʹ���ڵȴ�״̬���߳�����ȴ����в�ת�� Zero ״̬��

������
	Thread -- Ŀ���̶߳���ָ�롣

����ֵ��
	�ޡ�

--*/
{
	ASSERT(Waiting == Thread->State);

	//
	// ���̴߳����ڵȴ��������Ƴ����޸�״̬��ΪZero��
	//
	ListRemoveEntry(&Thread->StateListEntry);
	Thread->State = Zero;

	//
	// ����߳�ע���˵ȴ���ʱ������ע���ȴ���ʱ����
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

����������
	����ָ���ȴ����еĶ����̡߳�

������
	WaitListHead -- �ȴ�����ָ�롣
	WaitStatus -- �������̴߳�PspWait���صķ���ֵ��

����ֵ��
	����ȴ�����Ϊ���򷵻�NULL�����򷵻ر������̵߳�ָ�롣

--*/
{
	PTHREAD Thread;

	if (!ListIsEmpty(WaitListHead)) {

		//
		// ���ѵȴ����еĶ����̡߳�
		//
		Thread = CONTAINING_RECORD(WaitListHead->Next, THREAD, StateListEntry);
		PspUnwaitThread(Thread);
		PspReadyThread(Thread);

		//
		// �����̴߳�PspWait���صķ���ֵ��
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

����������
	ʱ��Ƭ��ת���Ⱥ���������ʱ�������жϷ������ KiIsrTimer ���á�

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	//
	// �ڴ���Ӵ��룬ʵ��ʱ��Ƭ��ת�����㷨��
	//
	
	return;
}

PCONTEXT
PspSelectNextThread(
	VOID
	)
/*++

����������
	�̵߳��Ⱥ�������������жϷ������ִ����ɺ󣬲��������̷��ر��ж����е���
	�̣����ǵ����������ѡ��һ�����ʵ��̼߳������У����жϵ��߳̿��ܼ������У���

������
	�ޡ�

����ֵ��
	Ӧִ���̵߳�CPU������ָ�롣

--*/
{
	ULONG HighestPriority;
	SIZE_T StackSize;

	//
	// ɨ�����λͼ����õ�ǰ������ȼ���ע�⣺����λͼ����Ϊ�ա�
	//
	BitScanReverse(&HighestPriority, PspReadyBitmap);

	if (NULL != PspCurrentThread && Running == PspCurrentThread->State) {

		if (0 != PspReadyBitmap && HighestPriority > PspCurrentThread->Priority) {

			//
			// ������ڱȵ�ǰ�����߳����ȼ����ߵľ����̣߳���ǰ�߳�Ӧ�����ȡ�
			// ��Ϊ��ǰ�߳��Դ�������״̬�����Ա������ȼ��߳����Ⱥ�Ӧ������
			// ���ȼ���Ӧ�ľ������еĶ��ס�ע�⣬���ܵ��� PspReadyThread��
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
			// ��ǰ�̼߳������С�
			// ע�⣺�жϳ���ִ��ʱ���ܻ����˵�ǰ�̰߳����еĵ�ַ�ռ䡣
			//
			MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);
			return &PspCurrentThread->KernelContext;
		}

	} else if(0 == PspReadyBitmap) {

		//
		// ���ж������̴߳��ڷ�����״̬���������һ�������еľ����̡߳�
		//
		ASSERT(FALSE);
		KeBugCheck("No ready thread to run!");
	}

	if (NULL != PspCurrentThread) {

		//
		// �����ǰ�߳̽������Լ����������ͷ��̵߳��ں�ջ����Ϊ�߳���ִ��ʱ����
		// �ͷ��Լ�����ռ�õ�ջ��
		//
		if (Terminated == PspCurrentThread->State) {

			StackSize = 0;

			MmFreeVirtualMemory( &PspCurrentThread->KernelStack,
								 &StackSize,
								 MEM_RELEASE,
								 TRUE );
		}

		//
		// ȡ��ָ�� PspCurrentThread ���̶߳�������á�
		//
		ObDerefObject(PspCurrentThread);
	}

	//
	// ѡ�����ȼ���ߵķǿվ������еĶ����߳���Ϊ��ǰ�����̡߳�
	//
	PspCurrentThread = CONTAINING_RECORD(PspReadyListHeads[HighestPriority].Next, THREAD, StateListEntry);
	ObRefObject(PspCurrentThread);

	PspUnreadyThread(PspCurrentThread);
	PspCurrentThread->State = Running;
	
#ifdef _DEBUG
	RECORD_TASK_STATE(ObGetObjectId(PspCurrentThread) , TS_RUNNING, Tick);
#endif

	//
	// �����̰߳����еĵ�ַ�ռ䡣
	//
	MmSwapProcessAddressSpace(PspCurrentThread->AttachedPas);

	//
	// �����̵߳������Ļ����飬�ָ��߳����С�
	//
	return &PspCurrentThread->KernelContext;
}

VOID
PspThreadSchedule(
	VOID
	)
/*++

����������
	ִ���̵߳��ȡ�

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	ULONG HighestPriority;

	//
	// ע�⣬�����ǰ���ڴ����жϣ��ж�Ƕ����Ȳ�Ϊ 0����ʲôҲ������
	// ��Ϊ���жϷ���ʱϵͳ���Զ�ִ���̵߳��ȡ�
	//
	if (KeGetIntNesting() == 0) {

		if (Running != PspCurrentThread->State) {

			//
			// ��ǰ�߳��Ѿ����ڷ�����״̬��ִ���̵߳��ȡ�
			//
			KeThreadSchedule();

		} else if (0 != PspReadyBitmap) {

			//
			// ɨ�����λͼ��������ڱȵ�ǰ�߳����ȼ��ߵľ����߳���ִ���̵߳��ȡ�
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

����������
	��ǰ�߳�ֹͣ����ָ����ʱ������

������
	Milliseconds -- ֹͣ���е�ʱ��������λ���롣
					ע�⣬�����ֵΪ INFINITE����ǰ�߳̽���Զֹͣ���С�

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;

	ASSERT(KeGetIntNesting() == 0);

	IntState = KeEnableInterrupts(FALSE);

	if (0 == Milliseconds) {

		//
		// �������ͬ���ȼ��ľ����̣߳���ǰ�߳̽������״̬��ִ���̵߳��ȡ�
		//
		if (BIT_TEST(PspReadyBitmap, PspCurrentThread->Priority)) {
			PspReadyThread(PspCurrentThread);
			PspThreadSchedule();
		}

	} else {

		//
		// ��ǰ�߳���˯�߶����еȴ���ʱ��
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

����������
	����ָ���߳����ȼ���ע�⣺Ŀ���߳����ȼ��ı����ܻᴥ���̵߳��ȡ�

������
	Handle -- Ŀ���߳̾����
	Priority -- �����õ������ȼ���

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS������˵��������Ч��

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
		// �����߳��ڸı����ȼ�ʱ���ڲ�ͬ���ȼ���Ӧ�ľ�������֮��Ǩ�ơ�
		//
		if (Ready == Thread->State) {

			//
			// �߳����뵱ǰ���ȼ���Ӧ�ľ������С�
			//
			PspUnreadyThread(Thread);

			Thread->Priority = Priority;

			//
			// �̲߳��������ȼ���Ӧ�ľ������еĶ�β��
			//
			PspReadyThread(Thread);

		} else {

			Thread->Priority = Priority;
		}

		//
		// ���ȼ��ı����Ҫִ���̵߳��ȡ�
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

����������
	��ȡָ���̵߳����ȼ���

������
	Handle -- Ŀ���߳̾����
	Priority -- �����̵߳����ȼ���

����ֵ��
	����ɹ��򷵻� STATUS_SUCCESS��

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
	// ��ȡ�̵߳����ȼ���
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

����������
	��ǰ�̸߳�����ָ�����̵ĵ�ַ�ռ���ִ�С�

������
	Process -- ���̶���ָ�롣

����ֵ��
	�ޡ�

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
������һ�����Ա����ڼ�¼����״̬ת���Ĺ켣��
*/															
struct task_state_entry
{
	long pid;
	long new_state;
	long Tick;
	char fun_name[64];
	int line_num;
};

#define MAX_TASK_TRANS 1000		// ֱ�ӽ���ֵ�޸ĵĸ���һЩ���Ϳ��Լ�¼�������״̬�任�Ĺ켣����Ҫ�趨��̫�󣬷���ᵼ�»��ƹ켣�������ڴ治�㡣
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
