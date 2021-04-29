/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: semaphore.c

����: ����ͬ������֮��¼���ź�����ʵ�֡�



*******************************************************************************/

#include "psp.h"

VOID
PsInitializeSemaphore(
	IN PSEMAPHORE Semaphore,
	IN LONG InitialCount,
	IN LONG MaximumCount
	)
/*++

����������
	��ʼ���ź����ṹ�塣

������
	Semaphore -- Ҫ��ʼ�����ź����ṹ��ָ�롣
	InitialCount -- �ź����ĳ�ʼֵ������С�� 0 �Ҳ��ܴ��� MaximumCount��
	MaximumCount -- �ź��������ֵ��������� 0��

����ֵ��
	�ޡ�

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

����������
	�ź����� Wait ������P ��������

������
	Semaphore -- Wait �������ź�������
	Milliseconds -- �ȴ���ʱ���ޣ���λ���롣

����ֵ��
	STATUS_SUCCESS��
	�����޸��ź���ʹ֧֮�ֳ�ʱ���ѹ��ܺ�����ȴ���ʱ��Ӧ�÷��� STATUS_TIMEOUT��

--*/
{
	BOOL IntState;

	ASSERT(KeGetIntNesting() == 0); // �жϻ����²��ܵ��ô˺�����

	IntState = KeEnableInterrupts(FALSE); // ��ʼԭ�Ӳ�������ֹ�жϡ�

	//
	// Ŀǰ��ʵ���˱�׼��¼���ź�������֧�ֳ�ʱ���ѹ��ܣ����� PspWait ����
	// �ĵڶ���������ֵֻ���� INFINITE��
	//
	Semaphore->Count--;
	if (Semaphore->Count < 0) {
		PspWait(&Semaphore->WaitListHead, INFINITE);
	}

	KeEnableInterrupts(IntState); // ԭ�Ӳ�����ɣ��ָ��жϡ�

	return STATUS_SUCCESS;
}

STATUS
PsReleaseSemaphore(
	IN PSEMAPHORE Semaphore,
	IN LONG ReleaseCount,
	OUT PLONG PreviousCount
	)
/*++

����������
	�ź����� Signal ������V ��������

������
	Semaphore -- Wait �������ź�������
	ReleaseCount -- �ź����������ӵ���������ǰֻ��Ϊ 1�������޸��ź���ʹ֧֮��
					��ʱ���ѹ��ܺ󣬴˲�����ֵ�ܹ����ڵ��� 1��
	PreviousCount -- �����ź�������������֮ǰ��ֵ��

����ֵ��
	����ɹ��ͷ��ź��������� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE); // ��ʼԭ�Ӳ�������ֹ�жϡ�

	if (Semaphore->Count + ReleaseCount > Semaphore->MaximumCount) {

		Status = STATUS_SEMAPHORE_LIMIT_EXCEEDED;

	} else {

		//
		// ��¼��ǰ���ź�����ֵ��
		//
		if (NULL != PreviousCount) {
			*PreviousCount = Semaphore->Count;
		}

		//
		// Ŀǰ��ʵ���˱�׼��¼���ź�����ÿִ��һ���ź������ͷŲ���
		// ֻ��ʹ�ź�����ֵ���� 1��
		//
		Semaphore->Count++;
		if (Semaphore->Count <= 0) {
			PspWakeThread(&Semaphore->WaitListHead, STATUS_SUCCESS);
		}

		//
		// �������̱߳����ѣ�ִ���̵߳��ȡ�
		//
		PspThreadSchedule();

		Status = STATUS_SUCCESS;
	}

	KeEnableInterrupts(IntState); // ԭ�Ӳ�����ɣ��ָ��жϡ�

	return Status;
}

//////////////////////////////////////////////////////////////////////////
//
// �����Ǻ��ź�������������صĴ��롣
//

//
// �ź�����������ָ�롣
//
POBJECT_TYPE PspSemaphoreType = NULL;

//
// ���ڳ�ʼ�� semaphore �ṹ��Ĳ����ṹ�塣
//
typedef struct _SEM_CREATE_PARAM{
	LONG InitialCount;
	LONG MaximumCount;
}SEM_CREATE_PARAM, *PSEM_CREATE_PARAM;

//
// semaphore ����Ĺ��캯�����ڴ����� semaphore ����ʱ�����á�
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
// semaphore �������͵ĳ�ʼ��������
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
// semaphore ����Ĺ��캯����
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
	// �����ź�������
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
// semaphore ����� signal ����������
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

	// �� semaphore ����õ� semaphore �����ָ�롣
	Status = ObRefObjectByHandle(Handle, PspSemaphoreType, (PVOID*)&Semaphore);

	if (EOS_SUCCESS(Status)) {
		Status = PsReleaseSemaphore(Semaphore, ReleaseCount, PreviousCount);
		ObDerefObject(Semaphore);
	}

	return Status;
}

