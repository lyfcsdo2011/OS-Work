/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: event.c

����: ����ͬ������֮�¼���ʵ�֡�



*******************************************************************************/

#include "psp.h"

VOID
PsInitializeEvent(
	IN PEVENT Event,
	IN BOOL ManualReset,
	IN BOOL InitialState
	)
/*++

����������
	��ʼ���¼��ṹ�塣

������
	Event -- �¼��ṹ��ָ�롣
	ManualReset -- �Ƿ��ʼ��Ϊ�ֶ��¼���TRUE Ϊ�ֶ���FALSE Ϊ�Զ���
	InitialState -- �¼���ʼ����״̬��TRUE Ϊ��Ч��FALSE Ϊ��Ч��

����ֵ��
	�ޡ�

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

����������
	�����ȴ��¼�ֱ���¼���Ϊ Signaled ״̬������ȴ������Զ��¼����ɹ��ȴ���Ḵλ
	�¼�Ϊ Nonsignaled ״̬��

������
	Event -- �¼��ṹ��ָ�롣
	Milliseconds -- �ȴ�ʱ�����ޣ�����ȴ���ʱ�򷵻�STATUS_TIMEOUT�����ΪINFINIT
		�����õȴ�ֱ���ȴ��ɹ���

����ֵ��
	STATUS_SUCCESS��ʾ�ȴ��ɹ���STATUS_TIMEOUT��ʾ�ȴ���ʱ��

--*/
{
	STATUS Status;
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	//
	// ����¼����� Signaled ״̬��ô���̷��سɹ�������ǰ�߳������ȴ��¼���
	//
	if (Event->IsSignaled) {

		//
		// ������Զ��¼�����ô�ɹ��ȴ���ͬʱ��Ҫ��λ�¼���
		//
		if (!Event->IsManual) {
			Event->IsSignaled = FALSE;
		}

		Status = STATUS_SUCCESS;

	} else {

		//
		// ����FCFSԭ���������¼��ȴ����еĶ�β��
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

����������
	ʹ�¼���Ϊ Singnaled ״̬������¼��ĵȴ����������߳����ڵȴ����ȴ��߳̽������ѡ�

������
	Event -- �¼��ṹ��ָ�롣

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState = KeEnableInterrupts(FALSE);

	//
	// ����¼����� Nonsignaled ״̬�����޸��¼�Ϊ Signaled ״̬��
	//
	if (!Event->IsSignaled) {

		Event->IsSignaled = TRUE;

		while (Event->IsSignaled && !ListIsEmpty(&Event->WaitListHead)) {

			//
			// �¼����� Signaled ״̬�ҵȴ����зǿգ����� FCFS ��ԭ���ѵȴ����еĶ����̡߳�
			//
			PspWakeThread(&Event->WaitListHead, STATUS_SUCCESS);

			//
			// ������Զ��¼�����ô��֪ͨһ���̺߳��¼�������λ����Ϊ Nonsignaled ״̬����
			//
			if (!Event->IsManual) {
				Event->IsSignaled = FALSE;
			}
		}

		//
		// �������̱߳����ѣ�ִ���̵߳��ȡ�
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

����������
	��λ�¼�״̬Ϊ Nonsignaled ״̬��

������
	Event -- �¼��ṹ��ָ�롣

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState = KeEnableInterrupts(FALSE);

	Event->IsSignaled = FALSE;

	KeEnableInterrupts(IntState);
}

//////////////////////////////////////////////////////////////////////////
//
// �����Ǻ��¼�����������صĴ��롣
//

//
// �¼���������ָ�롣
//
POBJECT_TYPE PspEventType;

VOID
PspOnCreateEventObject(
	IN PVOID EventObject,
	IN ULONG_PTR CreateParam
	)
/*++

����������
	�¼�����Ĺ��캯������ObCreateObject���á�

������
	EventObject -- �´������¼������ָ�롣
	CreateParam -- ���������λ0��־�Ƿ�Ϊ�ֶ��¼���λ1��־��ʼ״̬��

����ֵ��
	�ޡ�

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

����������
	�����¼��������͡�

������
	�ޡ�

����ֵ��
	�ޡ�

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

����������
	�����¼�����

������
	ManualReset -- �Ƿ��ʼ��Ϊ�ֶ��¼���
	InitialState -- �¼���ʼ����״̬��
	EventName -- �����ַ���ָ�룬���ΪNULL�򴴽�һ���������󣬷����Դ��Ѵ��ڵ���
		����������������󲻴����򴴽�һ���µ���������
	EventHandle -- ����´�����򿪵��¼���������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

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
	// �����¼�����
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
	// Ϊ�¼����󴴽������
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

����������
	ʹ�¼������Ϊ Singnaled ״̬������¼��ĵȴ����������߳����ڵȴ����ȴ��߳̽������ѡ�

������
	Handle -- �¼�����ľ����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PEVENT Event;

	//
	// ���¼��������õ�����ָ�롣
	//
	Status = ObRefObjectByHandle(Handle, PspEventType, (PVOID*)&Event);

	if (EOS_SUCCESS(Status)) {

		PsSetEvent(Event);

		//
		// �ر��¼�����ָ�롣
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

����������
	��λ�¼�����״̬Ϊ Nonsignaled ״̬��

������
	Handle -- �¼�����ľ����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PEVENT Event;

	//
	// ���¼��������õ�����ָ�롣
	//
	Status = ObRefObjectByHandle(Handle, PspEventType, (PVOID*)&Event);

	if (EOS_SUCCESS(Status)) {

		PsResetEvent(Event);

		//
		// �رն���ָ�롣
		//
		ObDerefObject(Event);
	}

	return Status;
}
