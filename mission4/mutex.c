/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: mutex.c

����: ����ͬ������֮�����ź�����ʵ�֡�



*******************************************************************************/

#include "psp.h"


VOID
PsInitializeMutex(
	IN PMUTEX Mutex,
	IN BOOL InitialOwner
	)
/*++

����������
	��ʼ�������ź����ṹ�塣

������
	Mutex -- �����ź����ṹ���ָ�롣
	InitialOwner -- �����ź����ĳ�ʼӵ�����̵߳�ָ�롣

����ֵ��
	�ޡ�

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

����������
	�����ȴ������ź�����P��������

������
	Mutex -- �ź����ṹ���ָ�롣
	Milliseconds -- �ȴ�ʱ�����ޣ���λ���롣����ȴ���ʱ�򷵻� STATUS_TIMEOUT��
		���Ϊ INFINIT �����õȴ�ֱ���ɹ���

����ֵ��
	STATUS_SUCCESS -- �ȴ��ɹ���
	STATUS_TIMEOUT -- �ȴ���ʱ��

--*/
{
	STATUS Status;
	BOOL IntState = KeEnableInterrupts(FALSE);

	ASSERT(KeGetIntNesting() == 0);

	//
	// ��� MUTEX �ǿ��еģ���������ӵ����Ϊ��ǰ�̲߳�������ݹ������Ϊ 1��
	// �����ǰ�̱߳����� MUTEX ��ӵ���ߣ������� MUTEX �ĵݹ��������
	// ��� MUTEX �������߳�ռ�У��򽫵�ǰ�̼߳��뵽 MUTEX �ĵȴ������С�
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

����������
	�ͷŻ����ź�����ע�⣺�ͷŻ����ź������̱߳����ǻ����ź�����ӵ�����̡߳�

������
	Mutex -- �����ź����ṹ���ָ�롣

����ֵ��
	STATUS_SUCCESS -- Release�����ɹ���
	STATUS_MUTEX_NOT_OWNED -- ��ǰ�����̷߳ǻ����ź�����ӵ���ߡ�

--*/
{
	STATUS Status;
	BOOL IntState = KeEnableInterrupts(FALSE);

	ASSERT(KeGetIntNesting() == 0);

	if (PspCurrentThread == Mutex->OwnerThread) {

		//
		// ��С�ݹ�������������Ϊ0���ͷŻ����ź�����
		//
		if (0 == --Mutex->RecursionCount) {

			//
			// ����һ���ȴ���û����ź������̣߳�����֮����Ϊ�ź�����ӵ�����̡߳�
			//
			Mutex->OwnerThread = PspWakeThread(&Mutex->WaitListHead, STATUS_SUCCESS);

			if(NULL != Mutex->OwnerThread) {

				Mutex->RecursionCount = 1;

				//
				// ���̱߳����ѣ�ִ���̵߳��ȡ�
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
// �����Ǻͻ����ź�������������صĴ��롣
//

//
// �����ź�����������ָ�롣
//
POBJECT_TYPE PspMutexType = NULL;

VOID
PspCreateMutexObjectType(
	VOID
	)
/*++

����������
	���������ź����������͡�

������
	�ޡ�

����ֵ��
	�ޡ�

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

����������
	���������ź�������

������
	InitialOwner -- ���ΪTRUE���ʼ����ǰ�����߳�Ϊ�´��������ź��������ӵ���ߡ�
	MutexName -- �����ַ���ָ�룬���ΪNULL�򴴽�һ���������󣬷����Դ��Ѵ��ڵ���
		����������������󲻴����򴴽�һ���µ���������
	MutexHandle -- ����´�����򿪵Ļ����ź�����������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PVOID MutexObject;

	//
	// ����MUTEX����
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

����������
	�ͷŻ����ź�������

������
	Handle -- �����ź�������ľ����

����ֵ��
	STATUS_SUCCESS -- Release�����ɹ���
	STATUS_INVALID_HANDLE -- �������һ����Ч�Ļ����ź�����������
	STATUS_MUTEX_NOT_OWNED -- ��ǰ�����̷߳ǻ����ź��������ӵ���ߡ�

--*/
{
	STATUS Status;
	PMUTEX Mutex;

	//
	// ���¼��������õ�����ָ�롣
	//
	Status = ObRefObjectByHandle(Handle, PspMutexType, (PVOID*)&Mutex);

	if (EOS_SUCCESS(Status)) {

		PsReleaseMutex(Mutex);

		//
		// �رն���ָ�롣
		//
		ObDerefObject(Mutex);
	}

	return Status;
}
