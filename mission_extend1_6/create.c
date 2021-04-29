/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: create.c

����: ���̡��̵߳Ĵ�����



*******************************************************************************/

#include "psp.h"

PPROCESS PspSystemProcess = NULL;

VOID
PsCreateSystemProcess(
	IN PTHREAD_START_ROUTINE StartAddr
	)
/*++

����������
	����ϵͳ���̡�

������
	StartAddr -- ϵͳ������ڵ�ַ��

����ֵ��
	�ޡ�

--*/
{
	STATUS Status;
	PTHREAD Thread;

	//
	// ϵͳ����Ψһ��ֻ�ܱ�����һ�Ρ�
	//
	ASSERT(NULL == PspSystemProcess);

	//
	// �������̻�����
	//
	Status = PspCreateProcessEnvironment(24, NULL, NULL, &PspSystemProcess);

	ASSERT(EOS_SUCCESS(Status));

	//
	// �������̵����̡߳�
	//
	Status = PspCreateThread( PspSystemProcess,
							  0,
							  StartAddr,
							  NULL,
							  0,
							  &Thread );

	ASSERT(EOS_SUCCESS(Status));

	PspSystemProcess->PrimaryThread = Thread;
}

STATUS
PsCreateProcess(
	IN PCSTR ImageName,
	IN PCSTR CmdLine OPTIONAL,
	IN ULONG CreateFlags,
	IN PSTARTUPINFO StartupInfo,
	OUT PPROCESS_INFORMATION ProcInfo
	)
/*++

����������
	����һ��Ӧ�ý��̡�

������
	ImageName -- ���̵Ŀ�ִ���ļ���ȫ·�����ƣ��MAX_PATH������Ϊ�ա�
	CmdLine -- �����в������1024���ַ�����ΪNULL��
	CreateFlags -- ����������Ŀǰ���޲�����ѡ��
	StartupInfo -- �½��������������ṹ�壬�������½���Ҫʹ�õı�׼���������������
	ProcInfo -- ָ�룬ָ�����ڱ�����̴��������Ϣ�Ľṹ�塣

����ֵ��
	��������ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PVOID StdInputObject = NULL;
	PVOID StdOutputObject = NULL;
	PVOID StdErrorObject = NULL;
	PPROCESS ProcessObject = NULL;
	PTHREAD ThreadObject =NULL;
	HANDLE ProcessHandle = NULL;
	HANDLE ThreadHandle = NULL;

	if (NULL == ImageName || NULL == StartupInfo) {
		return STATUS_INVALID_PARAMETER;
	}

	do {

		//
		// �ڸ�����ǰ�����̵ľ������Ϊ�����������ӽ����Լ��ӽ��̵����̶߳���������������
		//
		Status = ObAllocateHandleEx(PspCurrentProcess->ObjectTable, &ProcessHandle);
		if (!EOS_SUCCESS(Status)) {
			break;
		}

		Status = ObAllocateHandleEx(PspCurrentProcess->ObjectTable, &ThreadHandle);
		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// ����ָ���ı�׼��������������õ������ָ�룬���������Ƿ�Ϊ
		// ��Ч�Ŀɶ�д����
		//
		Status = ObRefObjectByHandleEx( PspCurrentProcess->ObjectTable,
										StartupInfo->StdInput,
										NULL,
										&StdInputObject);
		if (!EOS_SUCCESS(Status)) {
			break;
		}
		if ((ObGetAccessMask(StdInputObject) & OB_READ_ACCESS) == 0) {
			Status = STATUS_INVALID_HANDLE;
			break;
		}

		Status = ObRefObjectByHandleEx( PspCurrentProcess->ObjectTable,
										StartupInfo->StdOutput,
										NULL,
										&StdOutputObject);
		if (!EOS_SUCCESS(Status)) {
			break;
		}
		if ((ObGetAccessMask(StdOutputObject) & OB_WRITE_ACCESS) == 0) {
			Status = STATUS_INVALID_HANDLE;
			break;
		}

		Status = ObRefObjectByHandleEx( PspCurrentProcess->ObjectTable,
										StartupInfo->StdError,
										NULL,
										&StdErrorObject);
		if (!EOS_SUCCESS(Status)) {
			break;
		}
		if ((ObGetAccessMask(StdErrorObject) & OB_WRITE_ACCESS) == 0) {
			Status = STATUS_INVALID_HANDLE;
			break;
		}

		//
		// ����һ�����̻��������̵Ŀ��ƿ��Լ����̵ĵ�ַ�ռ�;������
		//
		Status = PspCreateProcessEnvironment(8, ImageName, CmdLine, &ProcessObject);

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// ���½����̵ľ������Ϊ��׼����������󴴽������
		// ��Ϊ�½����̵ľ����Ŀǰ���ǿյģ����Դ��������϶�����ʧ�ܡ�
		//
		Status = ObCreateHandleEx( ProcessObject->ObjectTable,
								   StdInputObject,
								   &ProcessObject->StdInput);
		ASSERT(EOS_SUCCESS(Status));

		Status = ObCreateHandleEx( ProcessObject->ObjectTable,
								   StdOutputObject,
								   &ProcessObject->StdOutput);
		ASSERT(EOS_SUCCESS(Status));

		Status = ObCreateHandleEx( ProcessObject->ObjectTable,
								   StdErrorObject,
								   &ProcessObject->StdError);
		ASSERT(EOS_SUCCESS(Status));

		StdInputObject = NULL;
		StdOutputObject = NULL;
		StdErrorObject = NULL;

		//
		// ���ؿ�ִ��ӳ�񣨳����ָ������ݣ����½����̵��û���ַ�ռ��С�
		//
		Status = PspLoadProcessImage( ProcessObject,
									  ProcessObject->ImageName,
									  &ProcessObject->ImageBase,
									  (PVOID*)&ProcessObject->ImageEntry );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// �����½��̵����̣߳����н��̵����̶߳��Ӻ���PspProcessStartup��ʼִ�С�
		//
		Status = PspCreateThread( ProcessObject,
								  0,
								  PspProcessStartup,
								  NULL,
								  CreateFlags,
								  &ThreadObject );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		ProcessObject->PrimaryThread = ThreadObject;

		//
		// ���ø������еľ��ֵ��ʹָ֮���½��ӽ��̶��������̶߳�����������
		// �̾Ϳ���ͨ��������ʺͿ����ӽ��̼������߳��ˡ�
		//
		Status = ObSetHandleValueEx(PspCurrentProcess->ObjectTable, ProcessHandle, ProcessObject);
		ASSERT(EOS_SUCCESS(Status));

		Status = ObSetHandleValueEx(PspCurrentProcess->ObjectTable, ThreadHandle, ThreadObject);
		ASSERT(EOS_SUCCESS(Status));

		//
		// ���÷��ؽ�����ӽ��̼������̵߳ľ����ID����
		//
		ProcInfo->ProcessHandle = ProcessHandle;
		ProcInfo->ThreadHandle = ThreadHandle;
		ProcInfo->ProcessId = ObGetObjectId(ProcessObject);
		ProcInfo->ThreadId = ObGetObjectId(ThreadObject);

		Status = STATUS_SUCCESS;

	} while (0);

	//
	// �����������ʧ�ܣ�����ѽ��еĲ������лع���
	//
	if (!EOS_SUCCESS(Status)) {

		if (ProcessHandle != NULL) {
			ObFreeHandleEx(PspCurrentProcess->ObjectTable, ProcessHandle);
		}
		if (ThreadHandle != NULL) {
			ObFreeHandleEx(PspCurrentProcess->ObjectTable, ThreadHandle);
		}
		if (StdInputObject != NULL) {
			ObDerefObject(StdInputObject);
		}
		if (StdOutputObject != NULL) {
			ObDerefObject(StdOutputObject);
		}
		if (StdErrorObject != NULL) {
			ObDerefObject(StdErrorObject);
		}
		if (ProcessObject != NULL) {
			PspDeleteProcessEnvironment(ProcessObject);
		}
	}

	return Status;
}

STATUS
PsCreateThread(
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PHANDLE ThreadHandle,
	OUT PULONG ThreadId OPTIONAL
	)
/*++

����������
	�ڵ�ǰ�����ڴ���һ�����̡߳�

������
	StackSize -- �û�ģʽ�߳�ջ�Ĵ�С�������ǰ������ϵͳ���������֮��Ŀǰ����
		�̶߳�ִ�����ں�ջ�У�������ʱ���á�
	StartAddr -- �߳̿�ʼִ�еĺ�����ָ�롣
	ThreadParam -- ���ݸ��̺߳����Ĳ�����
	CreateFlags -- ����������Ŀǰ���޲�����ѡ��
	ThreadHandle -- ָ�룬ָ�����ڱ����߳̾���ı�����
	ThreadId -- ָ�룬ָ�����ڱ����߳�ID�ı�������ѡ��

����ֵ��
	��������̳߳ɹ����򷵻� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	HANDLE Handle;
	PTHREAD ThreadObject;

	if (NULL == StartAddr) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ��Ϊ�����������̶߳������һ�������
	//
	Status = ObAllocateHandleEx(PspCurrentProcess->ObjectTable, &Handle);

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	Status = PspCreateThread( PspCurrentProcess,
							  StackSize,
							  StartAddr,
							  ThreadParam,
							  CreateFlags,
							  &ThreadObject );

	//
	// ��������߳�ʧ�����ͷ�֮ǰ�����ľ�������ء�
	//
	if (!EOS_SUCCESS(Status)) {
		ObFreeHandleEx(PspCurrentProcess->ObjectTable, Handle);
		return Status;
	}

	//
	// ���þ����ֵ��ʹ���ָ���´������̶߳���
	//
	Status = ObSetHandleValueEx(PspCurrentProcess->ObjectTable, Handle, ThreadObject);
	ASSERT(EOS_SUCCESS(Status));

	*ThreadHandle = Handle;
	if (ThreadId != NULL) {
		*ThreadId = ObGetObjectId(ThreadObject);
	}

	return STATUS_SUCCESS;
}

STATUS
PspCreateProcessEnvironment(
	IN UCHAR Priority,
	IN PCSTR ImageName,
	IN PCSTR CmdLine,
	OUT PPROCESS *Process
	)
/*++

����������
	����һ�����̻������������̵Ŀ��ƿ顢��ַ�ռ�;����

������
	Priority -- ���̵Ļ������ȼ��������ڴ������߳̽�Ĭ�ϼ̳�������ȼ���
	ImageName -- ���̵Ŀ�ִ���ļ���ȫ·�����ƣ��MAX_PATH������Ϊ�ա�
	CmdLine -- �����в������1024���ַ�����ΪNULL��
	Process -- ����������̶����ָ��

����ֵ��
	��������ɹ��򷵻� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	SIZE_T ImageNameSize;
	SIZE_T CmdLineSize;
	PPROCESS NewProcess;

	ASSERT(Priority <= 31);

	//
	// �����ִ���ļ����ƺ������в����ַ���ռ�õ��ڴ档
	//
	if (NULL != ImageName) {

		ImageNameSize = strlen(ImageName) + 1;

		if (ImageNameSize > MAX_PATH) {
			return STATUS_PATH_TOO_LONG;
		}

		if (NULL != CmdLine) {

			CmdLineSize = strlen(CmdLine) + 1;

			if (CmdLineSize > 1024) {
				return STATUS_INVALID_COMMAND_LINE;
			}

		} else {

			CmdLineSize = 0;
		}

	} else {

		//
		// ImageName Ϊ NULL ����� CmdLine��
		//
		ImageNameSize = 0;
		CmdLineSize = 0;
	}

	//
	// ��ʼԭ�Ӳ�������ֹ�жϡ�
	//
	IntState = KeEnableInterrupts(FALSE);

	do {

		//
		// ����һ�����̶���
		//
		Status = ObCreateObject( PspProcessType,
								 NULL,
								 sizeof(PROCESS) + ImageNameSize + CmdLineSize,
								 0,
								 (PVOID*)&NewProcess );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// �����ϵͳ������ֱ��ʹ��ϵͳ���̵�ַ�ռ䣬�����´���һ�����̵�ַ�ռ䡣
		//
		if (NULL == ImageName) {

			NewProcess->Pas = MmGetSystemProcessAddressSpace();

		} else {

			NewProcess->Pas = MmCreateProcessAddressSpace();

			if (NULL == NewProcess->Pas) {

				ObDerefObject(NewProcess);

				Status = STATUS_NO_MEMORY;

				break;
			}
		}

		//
		// Ϊ���̷���һ�������
		//
		NewProcess->ObjectTable = ObAllocateHandleTable();

		if (NULL == NewProcess->ObjectTable) {

			if (NULL != ImageName) {
				MmDeleteProcessAddressSpace(NewProcess->Pas);
			}

			ObDerefObject(NewProcess);

			Status = STATUS_NO_MEMORY;

			break;
		}

		//
		// ��ʼ�����̿��ƿ顣
		//
		NewProcess->System = (ImageName == NULL);
		NewProcess->Priority = Priority;
		NewProcess->PrimaryThread = NULL;
		NewProcess->StdInput = NULL;
		NewProcess->StdOutput = NULL;
		NewProcess->StdError = NULL;
		NewProcess->ExitCode = 0;
		ListInitializeHead(&NewProcess->ThreadListHead);
		ListInitializeHead(&NewProcess->WaitListHead);

		//
		// �����ִ��ӳ��������в����ַ����ǿ��򿽱�֮��
		//
		if (NULL != ImageName) {

			NewProcess->ImageName = (PCHAR)NewProcess + sizeof(PROCESS);
			strcpy(NewProcess->ImageName, ImageName);

			if (NULL != CmdLine) {
				NewProcess->CmdLine = NewProcess->ImageName + ImageNameSize;
				strcpy(NewProcess->CmdLine, CmdLine);
			} else {
				NewProcess->CmdLine = NULL;
			}

		} else {

			NewProcess->ImageName = NULL;
			NewProcess->CmdLine = NULL;
		}

		//
		// �ڱ�����PspDeleteProcessEnvironment֮ǰ��������һ�ݶ��Լ������á�
		//
		ObRefObject(NewProcess);

		//
		// ���÷���ֵ��
		//
		*Process = NewProcess;
		Status = STATUS_SUCCESS;

	} while (0);

	KeEnableInterrupts(IntState);	// ԭ�Ӳ�����ɣ��ָ��жϡ�

	return Status;
}

STATUS
PspCreateThread(
	IN PPROCESS Process,
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PTHREAD *Thread
	)
/*++

����������
	��ָ�������ڴ���һ���̡߳�

������
	Process -- ���̶���ָ�룬�´������߳̽�����������̡�
	StackSize -- �û�ģʽ�߳�ջ��С��Ŀǰ�̶߳�ִ�����ں�̬��������ʱ���á�
	StartAddr -- �߳̿�ʼִ�еĺ�����ָ�롣
	ThreadParam -- ���ݸ��̺߳����Ĳ�����
	CreateFlags -- ����������Ŀǰ���޲�����ѡ��
	Thread -- ����������̶߳����ָ�롣

����ֵ��
	��������̳߳ɹ����򷵻� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	PTHREAD NewThread;
	SIZE_T KernelStackSize;

	ASSERT(NULL != Process && NULL != StartAddr && NULL != Thread);

	IntState = KeEnableInterrupts(FALSE);

	do {

		//
		// �����̶߳���
		//
		Status = ObCreateObject( PspThreadType,
								 NULL,
								 sizeof(THREAD),
								 0,
								 (PVOID*)&NewThread );

		if (!EOS_SUCCESS(Status)) {
			break;
		}
	
#ifdef _DEBUG
		MaxTid = ObGetObjectId(NewThread);
		RECORD_TASK_STATE(ObGetObjectId(NewThread) , TS_CREATE, Tick);
#endif		
		
		//
		// Ϊ�̷߳���ִ�����ں�̬ʱ������ں�ջ��
		// �ں�ջλ��ϵͳ��ַ�ռ��У�Ŀǰ��С�̶�Ϊ8KB��
		//
		NewThread->KernelStack = NULL;
		KernelStackSize = KERNEL_STACK_SIZE;

		Status = MmAllocateVirtualMemory( &NewThread->KernelStack,
										  &KernelStackSize,
										  MEM_RESERVE | MEM_COMMIT,
										  TRUE );

		if (!EOS_SUCCESS(Status)) {
			ObDerefObject(NewThread); // �ͷ��Ѵ������̶߳���
			break;
		}

		//
		// ������û��̣߳��������û���ַ�ռ���Ϊ�̷߳����û�ģʽջ��
		// Ŀǰ�����̶߳�һֱִ�����ں�ģʽջ�У�û���û�ģʽջ�����Դ˲�����
		//
		/*if (!Process->System) {
		}*/

		//
		// ��ʼ���߳̿��ƿ顣
		//
		NewThread->Process = Process;
		NewThread->Priority = Process->Priority;	// �̼̳߳н��̵����ȼ���
		NewThread->StartAddr = StartAddr;			// �̵߳Ŀ�ʼִ�е�ַ��
		NewThread->Parameter = ThreadParam;			// �̺߳����Ĳ�����
		NewThread->State = Zero;
		NewThread->RemainderTicks = TICKS_OF_TIME_SLICE;
		NewThread->WaitStatus = 0;
		NewThread->LastError = 0;
		NewThread->ExitCode = 0;
		ListInitializeHead(&NewThread->WaitListHead);
		PspInitializeThreadContext(NewThread);		// ��ʼ���̵߳Ĵ����������ġ�
		NewThread->AttachedPas = Process->Pas;		// �߳������ڽ��̵ĵ�ַ�ռ���ִ�С�

		//
		// ���̲߳������ڽ��̵��߳�����
		//
		ListInsertTail(&Process->ThreadListHead, &NewThread->ThreadListEntry);

		//
		// ʹ�߳̽������״̬��
		//
		PspReadyThread(NewThread);
		
		
		//
		// �߳��ڽ���ǰ����һ�ݶ��Լ������á�
		//
		ObRefObject(NewThread);

		//
		// ���߳̽������״̬��ִ���̵߳��ȡ�
		//
		PspThreadSchedule();

		//
		// ���÷���ֵ��
		//
		*Thread = NewThread;
		Status = STATUS_SUCCESS;

	} while (0);

	KeEnableInterrupts(IntState);

	return Status;
}
