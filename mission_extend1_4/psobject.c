/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: psobject.c

����: �ͽ��̡��̶߳�����ص�һЩ������



*******************************************************************************/

#include "psp.h"

//
// ���̡��̶߳��������ָ�롣
//
POBJECT_TYPE PspProcessType;
POBJECT_TYPE PspThreadType;

STATUS
PspOnWaitForProcessObject(
	IN PVOID ProcessObject,
	IN ULONG Milliseconds
	);

STATUS
PspOnWaitForThreadObject(
	IN PVOID ThreadObject,
	IN ULONG Milliseconds
	);

VOID
PspOnDeleteThreadObject(
	IN PVOID ThreadObject
	);

VOID
PspCreateProcessObjectType(
	VOID
	)
/*++

����������
	�������̺��̶߳������͡�

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	STATUS Status;
	OBJECT_TYPE_INITIALIZER Initializer;

	//
	// �������̶������͡�
	//
	Initializer.Create = NULL;
	Initializer.Delete = NULL;
	Initializer.Wait = PspOnWaitForProcessObject;
	Initializer.Read = NULL;
	Initializer.Write = NULL;

	Status = ObCreateObjectType("PROCESS", &Initializer, &PspProcessType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create process object type!");
	}

	//
	// �����̶߳������͡�
	//
	Initializer.Delete = PspOnDeleteThreadObject;
	Initializer.Wait = PspOnWaitForThreadObject;

	Status = ObCreateObjectType("THREAD", &Initializer, &PspThreadType );

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create thread object type!");
	}
}

STATUS
PsGetObjectTable(
	IN HANDLE ProcessHandle,
	OUT PVOID *ObjectTable
	)
/*++

����������
	�õ�ָ�����̵��ں˶�������

������
	ProcessHandle -- ���̾����
	ObjectTable -- ָ�룬ָ�����ڱ�����ָ��ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PPROCESS Process;

	if (CURRENT_PROCESS_HANDLE == ProcessHandle) {

		*ObjectTable = PspCurrentProcess->ObjectTable;
		Status = STATUS_SUCCESS;

	} else {

		Status = ObRefObjectByHandleEx( PspCurrentProcess->ObjectTable,
										ProcessHandle,
										PspProcessType,
										(PVOID*)&Process );

		if (EOS_SUCCESS(Status)) {

			if (NULL != Process->ObjectTable) {
				*ObjectTable = Process->ObjectTable;
			} else {
				Status = STATUS_PROCESS_IS_TERMINATING;
			}
		}
	}

	return Status;
}

STATUS
PsGetStdHandle(
	IN ULONG StdHandle,
	OUT PHANDLE Handle
	)
/*++

����������
	�õ���ǰ���̵ı�׼������������

������
	StdHandle -- ��׼��������������������ֵ������STD_INPUT_HANDLE��
		STD_OUTPUT_HANDLE��STD_ERROR_HANDLE��֮һ��

	Handle -- ָ�룬ָ�����ڱ������ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	if (STD_INPUT_HANDLE == StdHandle) {
		*Handle = PspCurrentProcess->StdInput;
	} else if (STD_OUTPUT_HANDLE == StdHandle) {
		*Handle = PspCurrentProcess->StdOutput;
	} else if (STD_ERROR_HANDLE == StdHandle) {
		*Handle = PspCurrentProcess->StdError;
	} else {
		return STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}

//
// �õ���ǰ���̵�ӳ���ļ����ƺ������в�����
//
VOID
PsGetImageNameAndCmdLine(
	OUT PCHAR ImageNameBuffer,
	OUT PCHAR CmdLineBuffer
	)
{
	ASSERT(PspCurrentProcess != PspSystemProcess);

	if (NULL != ImageNameBuffer) {
		strcpy(ImageNameBuffer, PspCurrentProcess->ImageName);
	}

	if (NULL != CmdLineBuffer) {
		if (NULL != PspCurrentProcess->CmdLine) {
			strcpy(CmdLineBuffer, PspCurrentProcess->CmdLine);
		}
		else {
			*CmdLineBuffer = 0;
		}
	}
}

//
// �õ���ǰ�̵߳Ĵ����롣
//
ULONG
PsGetLastError(
	VOID
	)
{
	return PspCurrentThread->LastError;
}

//
// ���õ�ǰ�̵߳Ĵ����롣
//
VOID
PsSetLastError(
	IN ULONG ErrCode
	)
{
	PspCurrentThread->LastError = ErrCode;
}

STATUS
PsGetExitCodeProcess(
	IN HANDLE ProcessHandle,
	OUT PULONG ExitCode
	)
/*++

����������
	�õ�ָ�����̵��˳��롣

������
	ProcessHandle -- ���̾����
	ExitCode -- ָ�룬ָ�����ڱ����˳���ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCESS��

--*/
{
	STATUS Status;
	PPROCESS ProcessObject;

	Status = ObRefObjectByHandle( ProcessHandle,
								  PspProcessType,
								  (PVOID*)&ProcessObject );

	if (EOS_SUCCESS(Status)) {
		*ExitCode = ProcessObject->ExitCode;
		
		ObDerefObject(ProcessObject);
	}

	return Status;
}

STATUS
PsGetExitCodeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG ExitCode
	)
/*++

����������
	�õ�ָ���̵߳��˳��롣

������
	ThreadHandle -- �߳̾����
	ExitCode -- ָ�룬ָ�����ڱ����˳���ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCESS��

--*/
{
	STATUS Status;
	PPROCESS ThreadObject;

	Status = ObRefObjectByHandle( ThreadHandle,
								  PspProcessType,
								  (PVOID*)&ThreadObject );

	if (EOS_SUCCESS(Status)) {
		*ExitCode = ThreadObject->ExitCode;
		
		ObDerefObject(ThreadObject);
	}

	return Status;
}

STATUS
PsOpenProcess(
	IN ULONG ProcessId,
	OUT PHANDLE ProcessHandle
	)
/*++

����������
	ͨ������Id�򿪽��̶���ľ����

������
	ProcessId -- �����򿪵Ľ��̶����Id��
	ProcessHandle -- ָ�룬ָ�����ڱ�����̾���ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PVOID ProcessObject;

	Status = ObRefObjectById(ProcessId, PspProcessType, &ProcessObject);

	if (!EOS_SUCCESS(Status)) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = ObCreateHandle(ProcessObject, ProcessHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(ProcessObject);
	}

	return Status;
}

STATUS
PsOpenThread(
	IN ULONG ThreadId,
	OUT PHANDLE ThreadHandle
	)
/*++

����������
	ͨ���߳�Id���̶߳���ľ����

������
	ThreadId -- �����򿪵��̶߳����Id��
	ThreadHandle -- ָ�룬ָ�����ڱ����߳̾���ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PVOID ThreadObject;

	Status = ObRefObjectById(ThreadId, PspThreadType, &ThreadObject);

	if (!EOS_SUCCESS(Status)) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = ObCreateHandle(ThreadObject, ThreadHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(ThreadObject);
	}

	return Status;
}
