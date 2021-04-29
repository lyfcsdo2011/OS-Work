/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: delete.c

����: �̡߳����̽����Լ���غ�����ʵ�֡�



*******************************************************************************/

#include "psp.h"

extern LIST_ENTRY PspTerminatedListHead;

STATUS
PsTerminateProcess(
	IN HANDLE Handle,
	IN ULONG ExitCode
	)
/*++

����������
	����ָ���Ľ��̣������ڽ��̻����ж��е��á�

������
	Handle -- ���������̵ľ����
	ExitCode -- ���̽����롣

����ֵ��
	����ɹ��򷵻�STATUSS_SUCESS��

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
		// �����ǰ���̱�����������Ĵ����û�л���ִ���ˡ�
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

����������
	����ָ�����̡߳�

������
	Handle -- �������̵߳ľ����
	ExitCode -- �߳̽����롣

����ֵ��
	����ɹ��򷵻�STATUSS_SUCESS��

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
		// �����ǰ�̻߳�ǰ���̱�����������Ĵ����û�л�����ִ���ˡ�
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

����������
	�˳���ǰ���̡�

������
	ExitCode -- �˳��롣

����ֵ��
	�ޡ�

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

����������
	�˳���ǰ�̡߳�

������
	ExitCode -- ���̽����롣

����ֵ��
	�ޡ�

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
	// ���̵��̱߳����Ѿ�ȫ�����������߸�����û�д����̡߳�
	//
	ASSERT(NULL == Process->PrimaryThread);

	//
	// ���ܶ�ͬһ�����̶����ظ��������������
	//
	ASSERT(NULL != Process->ObjectTable && NULL != Process->Pas);

	//
	// �ͷž�����ر������Ѵ򿪾������
	//
	ObFreeHandleTable(Process->ObjectTable);
	Process->ObjectTable = NULL;

	//
	// ��ǰ�̸߳��ŵ����������̵ĵ�ַ�ռ���ִ�У���ִ�����������
	//
	PspThreadAttachProcess(Process);

	//
	// ��������û���ַ�ռ��е������ڴ档
	//
	MmCleanVirtualMemory();

	//
	// ��ǰ�̷߳��ص��Լ��������̵ĵ�ַ�ռ��м���ִ�С�
	// ע�⣺�����ǰ�߳����ڱ������������ŵ�ϵͳ���̵�ַ�ռ��м���ִ�С�
	//
	if (PspCurrentProcess == Process) {
		PspThreadAttachProcess(PspSystemProcess);
	} else {
		PspThreadAttachProcess(PspCurrentProcess);
	}

	//
	// ɾ�����̵�ַ�ռ䡣
	//
	MmDeleteProcessAddressSpace(Process->Pas);
	Process->Pas = NULL;

	//
	// �����Ѿ�������ɾ�������ٱ������Լ������á�
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

����������
	����ָ�����̡�

������
	Process -- ���̶���ָ�롣
	ExitCode -- ���̽����롣

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	PTHREAD Thread;

	IntState = KeEnableInterrupts(FALSE);

	if (NULL != Process->PrimaryThread) {

		//
		// ���ý��̽�����־�����߳�ָ��ΪNULL���ͽ����롣
		//
		Process->PrimaryThread = NULL;
		Process->ExitCode = ExitCode;

		//
		// ���ѵȴ����̽����������̡߳�
		//
		while (!ListIsEmpty(&Process->WaitListHead)) {
			PspWakeThread(&Process->WaitListHead, STATUS_SUCCESS);
		}

		//
		// ���������ڵ������̡߳�
		// ע�⣺���������ڽ���ÿ���̺߳�����ִ���̵߳��ȣ������̶߳�����������ִ
		// ��һ�ε��ȼ��ɡ�
		//
		while (!ListIsEmpty(&Process->ThreadListHead)) {
			Thread = CONTAINING_RECORD(Process->ThreadListHead.Next, THREAD, ThreadListEntry);
			PspTerminateThread(Thread, ExitCode, TRUE);
		}

		//
		// ɾ�����̻�����
		//
		PspDeleteProcessEnvironment(Process);

		//
		// ִ���̵߳��ȡ�
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

����������
	����ָ�����̡߳�

������
	Thread -- Ŀ���̶߳���ָ�롣
	ExitCode -- �߳̽����롣
	IsTerminatingProcess -- ����ִ�н������̲�����

����ֵ��
	�ޡ�

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
			// �������߳������ڽ��̵����̣߳������߳����ڵ��������̡�
			//
			PspTerminateProcess(Thread->Process, ExitCode);

		} else {

			//
			// ���ѵȴ��߳̽����������̡߳�
			//
			while (!ListIsEmpty(&Thread->WaitListHead)) {
				PspWakeThread(&Thread->WaitListHead, STATUS_SUCCESS);
			}

			//
			// �߳�����Ŀǰ����״̬��ת�����״̬��
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
			// �����߳̽����벢���̴߳ӽ��̵��߳��������Ƴ���
			//
			Thread->ExitCode = ExitCode;
			ListRemoveEntry(&Thread->ThreadListEntry);

			//
			// �ͷ��̵߳��ں�ģʽջ��
			// ע�⣺�����ǰ�߳����ڽ����Լ��������ͷ��߳�����ʹ�õ��ں�ջ��
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
			// �����ɾ���̲߳���ϵͳ�̣߳������ͷ��̵߳��û�ģʽջ��Ŀǰ������
			// �̶�ִ�����ں�ģʽջ�У�û���û�ģʽջ�����Դ˲�����
			//
			/*if (!Thread->Process->System) {

				//
				// �û�ģʽջ�ڽ��̵��û���ַ�ռ��У���ǰ�߳���Ҫ���ŵ���������
				// ���������̵ĵ�ַ�ռ���ִ�С�
				//
				PspThreadAttachProcess(Thread->Process);

				//
				// �ͷű�ɾ���̵߳��û�ģʽջ��
				//
				
				//
				// ��ǰ�߳����·��ص��Լ��������̵ĵ�ַ�ռ���ִ�С�
				//
				PspThreadAttachProcess(PspCurrentProcess);
			}*/

			//
			// �߳̽������̲߳��ٱ������Լ������á�
			//
			ObDerefObject(Thread);	

			//
			// ע�⣺���ڽ�������ʱ����Ҫִ���̵߳��ȣ���Ϊ�������̺�������ִ���̵߳��ȡ�
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

����������
	�ȴ����̽�����

������
	ProcessObject -- ���̶���ָ�롣
	Milliseconds -- �ȴ�ʱ�����ޣ�����ȴ���ʱ�򷵻�STATUS_TIMEOUT�����ΪINFINIT
		�����õȴ�ֱ���ȴ��ɹ���

����ֵ��
	STATUS_SUCCESS -- �ȴ��ɹ���
	STATUS_TIMEOUT -- �ȴ���ʱ��

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	//
	// ��������Ѿ����������̷��أ������ڽ��̵ĵȴ������еȴ�ֱ�����̽�����
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

����������
	�ȴ��߳̽�����

������
	ThreadObject -- �̶߳���ָ�롣
	Milliseconds -- �ȴ�ʱ�����ޣ�����ȴ���ʱ�򷵻�STATUS_TIMEOUT�����ΪINFINIT
		�����õȴ�ֱ���ȴ��ɹ���

����ֵ��
	STATUS_SUCCESS -- �ȴ��ɹ���
	STATUS_TIMEOUT -- �ȴ���ʱ��

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	ASSERT(Zero != ((PTHREAD)ThreadObject)->State);

	//
	// ����������߳��Ѿ����������̷��أ������ڱ������̵߳ĵȴ������еȴ���
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

����������
	�̶߳����������������ɾ���̶߳���ʱ�����á�

������
	ThreadObject -- �̶߳����ָ�롣

����ֵ��
	�ޡ�

--*/
{
	ASSERT( Zero == ((PTHREAD)ThreadObject)->State ||
			Terminated == ((PTHREAD)ThreadObject)->State );

	//
	// �����ڽ���״̬���̶߳���ӽ���״̬�������Ƴ���
	//
	if (Terminated == ((PTHREAD)ThreadObject)->State) {
		ListRemoveEntry(&((PTHREAD)ThreadObject)->StateListEntry);
	}
}
