/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: sysproc.c

����: ϵͳ���̺���������̨�̺߳���������������̨�����Ӧ������



*******************************************************************************/

#include "ke.h"
#include "ob.h"
#include "io.h"
#include "psp.h"
#include "mi.h"
#include "iop.h"
#include "obp.h"
#include "fat12.h"


PRIVATE
VOID
ConsoleCmdVersionNumber(
	IN HANDLE StdHandle
	);

PRIVATE
VOID
ConsoleCmdMemoryMap(
	IN HANDLE StdHandle
	);
	
PRIVATE
VOID
ConsoleCmdDiskSchedule(
	IN HANDLE StdHandle
	);
	
PRIVATE
VOID
ConsoleCmdRoundRobin(
	IN HANDLE StdHandle
	);
	
PRIVATE
VOID
ConsoleCmdLoop(
	IN HANDLE StdHandle
	);
	
PRIVATE
VOID
ConsoleCmdSuspendThread(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	);
	
PRIVATE
VOID
ConsoleCmdResumeThread(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	);
	
PRIVATE
VOID
ConsoleCmdPhysicalMemory(
	IN HANDLE StdHandle
	);
	
PRIVATE
VOID
ConsoleCmdVM(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	);

PRIVATE
VOID
ConsoleCmdProcAndThread(
	IN HANDLE StdHandle
	);

PRIVATE
VOID
ConsoleCmdScanDisk(
	IN HANDLE StdHandle
	);

PRIVATE
VOID
ConsoleCmdDir(
	IN HANDLE StdHandle
	);

ULONG
KiInitializationThread(
	IN PVOID Parameter
	);

ULONG
KiShellThread(
	IN PVOID Parameter
	);

ULONG
KiSystemProcessRoutine(
	IN PVOID Parameter
	)
{
	STATUS Status;
	HANDLE ThreadHandle;
	ULONG i;

	//
	// ������ʼ���̣߳��ɳ�ʼ���߳���ִ�еڶ�����ʼ����
	//
	Status = PsCreateThread( 0,
							 KiInitializationThread,
							 NULL,
							 FALSE,
							 &ThreadHandle,
							 NULL );

	if (!EOS_SUCCESS(Status)) {
		ASSERT(FALSE);
		KeBugCheck("Failed to create initialization thread!");
	}

	ObCloseHandle(ThreadHandle);

	//
	// ����ǰ�߳����ȼ�������ͣ���ǰ�߳���Ϊ�����߳̽������ѭ����
	//
	PsSetThreadPriority(CURRENT_THREAD_HANDLE, 0);

	for(;;) {
		i++;
	}
}

ULONG
KiInitializationThread(
	IN PVOID Parameter
	)
{
	STATUS Status;
	ULONG ConsoleIndex;
	HANDLE ThreadHandle;

	//
	// ִ�еڶ�����ʼ�������Ե��û�������ǰ�̵߳ĺ��������Դ�������ģ���ڲ���ϵ
	// ͳ�߳�(�����ڴ����ģ���0ҳ��ʼ���߳�)��
	//
	IoInitializeSystem2();
	MmInitializeSystem2();
	ObInitializeSystem2();
	PsInitializeSystem2();

	//
	// Ϊ 4 ������̨������һ���̡߳�
	//
	for(ConsoleIndex = 0; ConsoleIndex < 4; ConsoleIndex++) {

		Status = PsCreateThread( 0,
								 KiShellThread,
								 (PVOID)ConsoleIndex,
								 0,
								 &ThreadHandle,
								 NULL );

		if (EOS_SUCCESS(Status)) {
			ObCloseHandle(ThreadHandle);
		} else {
			break;
		}
	}

	return 0;
}

ULONG
KiShellThread(
	IN PVOID Parameter
	)
{
	STATUS Status;
	HANDLE StdHandle;
	HANDLE InputHandle;
	CHAR Line[0x400];
	CHAR Image[MAX_PATH];
	PCHAR Arg;
	STARTUPINFO StartInfo;
	PROCESS_INFORMATION ProcInfo;
	ULONG ExitCode;

	//
	// ���̲߳�����Ϊ��Ŵ򿪿���̨��Ϊ��׼������������
	//
	Status = IoOpenConsole((ULONG)Parameter, &StdHandle);
	ASSERT(EOS_SUCCESS(Status));
	fprintf(StdHandle, "Welcome to EOS shell\n");

	//
	// ��0������̨���Դ��ļ�a:\autorun.txt��Ϊ��������
	//
	if (0 == (ULONG)Parameter) {

		Status = IoCreateFile( "A:\\autorun.txt",
							   GENERIC_READ,
							   0,
							   OPEN_EXISTING,
							   0,
							   &InputHandle );

		if (!EOS_SUCCESS(Status)) {
			InputHandle = StdHandle;
		}

	} else {

		InputHandle = StdHandle;
	}

	for (;;){

		fprintf(StdHandle, ">");

		//
		// ��ȡ�����С����fgets()����NULL����˵����ȡ�ļ�autorun.txt��������ʱ
		// Ӧ��������̨���Ϊ������������
		//
		while (NULL == fgets(InputHandle, Line)) {
			ASSERT(0 == (ULONG)Parameter && InputHandle != StdHandle);
			InputHandle = StdHandle;
		}

		//
		// �����ȡ����Autorun.txt������������С�
		//
		if (InputHandle != StdHandle) {
			fprintf(StdHandle, "Autorun %s\n", Line);
		}

		//
		// �������С�
		//
		if (0 == strlen(Line)) {
			continue;
		}

		//
		// �ҵ����еĵ�һ���ո��ڿո񴦽��нضϣ��ո�ǰ�ǿ�ִ���ļ�����������
		// ���ݸ�����Ĳ����ַ�����
		//
		for (Arg = Line; *Arg != '\0' && *Arg != ' ' && *Arg != '\t'; Arg++);

		if (' ' == *Arg || '\t' == *Arg) {
			*Arg++ = '\0';
		}

		//
		// �ڴ���ӿ���̨����
		//
		if (0 == stricmp(Line, "ver")) {
			ConsoleCmdVersionNumber(StdHandle);
			continue;
		} else if (0 == stricmp(Line, "mm")) {
			ConsoleCmdMemoryMap(StdHandle);
			continue;
		} else if (0 == stricmp(Line, "ds")) {
			ConsoleCmdDiskSchedule(StdHandle);
			continue;
		} else if (0 == stricmp(Line, "rr")) {
			ConsoleCmdRoundRobin(StdHandle);
			continue;
		} else if (0 == stricmp(Line, "loop")) {
			ConsoleCmdLoop(StdHandle);
			continue;
		}  else if (0 == stricmp(Line, "suspend")) {
			ConsoleCmdSuspendThread(StdHandle, Arg);
			continue;
		} else if (0 == stricmp(Line, "resume")) {
			ConsoleCmdResumeThread(StdHandle, Arg);
			continue;
		} else if (0 == stricmp(Line, "pm")) {
			ConsoleCmdPhysicalMemory(StdHandle);
			continue;
		} else if (0 == stricmp(Line, "vm")) {
			ConsoleCmdVM(StdHandle, Arg);
			continue;
		} else if (0 == stricmp(Line, "pt")) {
			ConsoleCmdProcAndThread(StdHandle);
			continue;
		} else if (0 == stricmp(Line, "sd")) {
			ConsoleCmdScanDisk(StdHandle);
			continue;
		} else if (0 == stricmp(Line, "dir")) {
			ConsoleCmdDir(StdHandle);
			continue;
		}

		//
		// ����ִ���ļ�����Line�п�����Image�С�������������ļ������Զ����ļ���
		// ǰ�����a�̸�Ŀ¼��·�������û�и�����.exe����չ�����Զ���ӡ�
		//
		if (strnicmp(Line, "a:\\", 3) != 0 && strnicmp(Line, "a:/", 3) != 0) {
			sprintf(Image, "a:\\%s", Line);
		} else {
			strcpy(Image, Line);
		}

		if (strnicmp(Line + strlen(Line) - 4, ".exe", 4) != 0) {
			strcpy(Image + strlen(Image), ".exe");
		}

		StartInfo.StdInput = StdHandle;
		StartInfo.StdOutput = StdHandle;
		StartInfo.StdError = StdHandle;

		Status = PsCreateProcess( Image,
								  Arg,
								  0,
								  &StartInfo,
								  &ProcInfo );

		if (!EOS_SUCCESS(Status)) {
			fprintf(StdHandle, "Error:Create process failed with status code 0x%x.\n", Status);
			continue;
		}

		//
		// �ȴ����̽�����Ȼ��رս��̺����̵߳ľ����
		//
		ObWaitForObject(ProcInfo.ProcessHandle, INFINITE);
		PsGetExitCodeProcess(ProcInfo.ProcessHandle, &ExitCode);
		ObCloseHandle(ProcInfo.ProcessHandle);
		ObCloseHandle(ProcInfo.ThreadHandle);

		fprintf(StdHandle, "\n%s exit with 0x%.8X.\n", Line, ExitCode);
	}
}

//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� ver ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdVersionNumber(
	IN HANDLE StdHandle
	)
/*++

����������
	��ӡ��� EOS ����ϵͳ�İ汾�š�����̨���ver����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	fprintf(StdHandle, "\nEngintime EOS [Version Number 1.2]\n\n");
}

//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� mm ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdMemoryMap(
	IN HANDLE StdHandle
	)
/*++

����������
	ת��ϵͳ���̵Ķ���ҳ��ӳ����Ϣ������̨���mm����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;

	ULONG PfnOfPageDirectory;
	ULONG PageTotal = 0;
	ULONG IndexOfDirEntry;
	ULONG IndexOfTableEntry;
	PMMPTE_HARDWARE pPde;
	PMMPTE_HARDWARE pPte;
	ULONG_PTR PageTableBase;
	ULONG_PTR VirtualBase;

	const char* OutputFormat = NULL;

	ASSERT(PspCurrentProcess == PspSystemProcess);

	IntState = KeEnableInterrupts(FALSE);	// ���ж�

	//
	// ���ҳĿ¼��ҳ���
	//
	OutputFormat = "\nCR3->0x%X\n";
	PfnOfPageDirectory = (ULONG)(PspSystemProcess->Pas->PfnOfPageDirectory);
	fprintf(StdHandle, OutputFormat, PfnOfPageDirectory);

	//
	// ����ҳĿ¼�е� PDE
	//
	for(IndexOfDirEntry = 0; IndexOfDirEntry < PTE_PER_TABLE; IndexOfDirEntry++)
	{
		pPde = (PMMPTE_HARDWARE)((ULONG_PTR)PDE_BASE + IndexOfDirEntry * PTE_SIZE);

		//
		// ������Ч�� PDE
		//
		if(!pPde->Valid)
			continue;

		//
		// ��� PDE ��Ϣ����ʽ���£�
		// PDE: ��� (ӳ��� 4M �����ַ�Ļ�ַ)->��ӳ��ҳ���ҳ���
		//
		OutputFormat = "PDE: 0x%X (0x%X)->0x%X\n";
		VirtualBase = (IndexOfDirEntry << PDI_SHIFT);
		fprintf(StdHandle, OutputFormat, IndexOfDirEntry, VirtualBase, pPde->PageFrameNumber);

		//
		// ���� PDE �ı�ż�����ӳ���ҳ�����������ַ�Ļ�ַ
		//
		PageTableBase = (ULONG_PTR)PTE_BASE + IndexOfDirEntry * PAGE_SIZE;

		//
		// ����ҳ���е� PTE
		//
		for(IndexOfTableEntry = 0; IndexOfTableEntry < PTE_PER_TABLE; IndexOfTableEntry++)
		{
			pPte = (PMMPTE_HARDWARE)(PageTableBase + IndexOfTableEntry * PTE_SIZE);

			//
			// ������Ч�� PTE
			//
			if(!pPte->Valid)
				continue;

			//
			// ��� PTE ��Ϣ����ʽ���£�
			// PTE: ��� (ӳ��� 4K �����ַ�Ļ�ַ)->��ӳ������ҳ��ҳ���
			//
			OutputFormat = "\t\tPTE: 0x%X (0x%X)->0x%X\n";
			VirtualBase = (IndexOfDirEntry << PDI_SHIFT) | (IndexOfTableEntry << PTI_SHIFT);
			fprintf(StdHandle, OutputFormat, IndexOfTableEntry, VirtualBase, pPte->PageFrameNumber);

			//
			// ͳ��ռ�õ�����ҳ��
			//
			PageTotal++;
		}
	}

	//
	// ���ռ�õ�����ҳ�����������ڴ���
	//
	OutputFormat = "\nPhysical Page Total: %d\n";
	fprintf(StdHandle, OutputFormat, PageTotal);
	OutputFormat = "Physical Memory Total: %d\n\n";
	fprintf(StdHandle, OutputFormat, PageTotal * PAGE_SIZE);

	KeEnableInterrupts(IntState);	// ���ж�
}

//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� ds ��صĴ��롣
//

PRIVATE
ULONG
AccessCylinderThread(
	IN PVOID Param
	)
/*++

����������
	�̺߳���������������ָ���Ĵŵ���

������
	Param -- �̲߳������������߳�Ҫ���ʵĴŵ��š�

����ֵ��
	���� 0 ��ʾ�߳�ִ�гɹ���

--*/
{
	BYTE TempBuffer[4];
	PDEVICE_OBJECT FloppyDevice;
	ULONG Cylinder = (ULONG)Param;	// �̲߳����������߳�Ҫ���ʵĴŵ���
	
	//
	// �õ������������豸����
	//
	FloppyDevice = (PDEVICE_OBJECT)ObpLookupObjectByName(IopDeviceObjectType, "FLOPPY0");
	
	//
	// ��ָ���ŵ��ĵ�һ��������ȡһ���ֽڵ����ݡ�
	// ��Ҫ�����ô�ͷ�ƶ���ָ���Ĵŵ������Բ������Ķ�ȡ�����ݡ�
	// ����������߳����ڷ��ʴ��̣����߳̾ͻᱻ������
	//
	IopReadWriteSector(FloppyDevice, Cylinder * 2 * 18,	0, TempBuffer, 1, TRUE);

	return 0;
}

PRIVATE
VOID
NewThreadAccessCylinder(
	IN HANDLE StdHandle,
	IN ULONG Cylinder
	)
/*++

����������
	����һ������ָ���ŵ����̣߳��������Ϣ��

������
	StdHandle -- ��׼���롢��������
	Cylinder -- �½��߳�Ҫ���ʵĴŵ��š�

����ֵ��
	�ޡ�

--*/
{
	HANDLE ThreadHandle;
	ULONG ThreadID;
	
	//
	// �½�һ���̣߳������̷߳���ָ���Ĵŵ���
	// ע�⣬�߳�Ҫ���ʵĴŵ�����ͨ���̲߳��������̺߳����ġ�
	//
	ThreadHandle = (HANDLE)CreateThread(0, AccessCylinderThread,
										(PVOID)Cylinder, 0, &ThreadID);
	PsSetThreadPriority(ThreadHandle, 31);	// �����½��߳����ȼ�����֤�䱻�������С�
	CloseHandle(ThreadHandle);

	//
	// ����߳���Ϣ��
	// ��ʽ��TID: �߳�ID Cylinder: ���ʴŵ���
	//	
	fprintf(StdHandle, "TID: %d Cylinder: %d\n", ThreadID, Cylinder);
}

PREQUEST
IopDiskSchedule(
	VOID
	);

PRIVATE
VOID
ConsoleCmdDiskSchedule(
	IN HANDLE StdHandle
	)
/*++

����������
	���Դ��̵����㷨������̨���ds����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	PREQUEST pNextRequest;
	extern BOOL IsDeviceBusy;
	
	//
	// �õ�ǰ�̷߳���һ�� 10 �ŵ���Ҳ�������˴�ͷ�ĳ�ʼλ�á�
	//
#ifdef _DEBUG
	ThreadSeq = 0;
#endif
	
	ULONG StartCylinder = 10;
	AccessCylinderThread((PVOID)StartCylinder);
	fprintf(StdHandle, "Start Cylinder: %d\n", StartCylinder);

	//
	// �������豸����Ϊæ��
	//
	IsDeviceBusy = TRUE;
	
	//
	// ����������ʲ�ͬ�ŵ����̡߳������豸æ����Щ�̵߳�����
	// ���ᱻ������������У�ֱ�������̵����㷨ѡ�к�Żᱻ����
	//
	NewThreadAccessCylinder(StdHandle,  8);
	NewThreadAccessCylinder(StdHandle, 21);
	NewThreadAccessCylinder(StdHandle,  9);
	NewThreadAccessCylinder(StdHandle, 78);
	NewThreadAccessCylinder(StdHandle,  0);
	NewThreadAccessCylinder(StdHandle, 41);
	NewThreadAccessCylinder(StdHandle, 10);
	NewThreadAccessCylinder(StdHandle, 67);
	NewThreadAccessCylinder(StdHandle, 12);
	NewThreadAccessCylinder(StdHandle, 10);
	
	//
	// �������̵����㷨�����յ��Ȳ������δ���������������е�����
	//
	pNextRequest = IopDiskSchedule();
	PsSetEvent(&pNextRequest->Event);
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� rr ��صĴ��롣
//

//
// �̲߳����ṹ��
//
typedef struct _THREAD_PARAMETER {
	SHORT Y;				// �߳��������������
	HANDLE StdHandle;		// �߳�����������ݵı�׼���
}THREAD_PARAMETER, *PTHREAD_PARAMETER;

PRIVATE
ULONG
ThreadFunction(
	PVOID Param
	)
/*++

����������
	�̺߳�����

������
	Param -- �̲߳�������һ��ָ�� THREAD_PARAMETER �ṹ�������ָ�롣

����ֵ��
	���� 0 ��ʾ�߳�ִ�гɹ���

--*/
{
	ULONG i;
	UCHAR Priority;
	COORD CursorPosition;
	PTHREAD_PARAMETER pThreadParameter = (PTHREAD_PARAMETER)Param;

	// �����̲߳����������������ʾ��λ�á�
	CursorPosition.X = 0;
	CursorPosition.Y = pThreadParameter->Y;

	// ���̲߳���ָ������ѭ����ʾ�߳�ִ�е�״̬����ѭ����ͨ�������жϻ�����ʿ���̨��
	// ��ʽ��Thread ��� (���ȼ�): ִ�м���
	for (i = 0; ; i++) {
		__asm("cli");
		PsGetThreadPriority(CURRENT_THREAD_HANDLE, &Priority);
		SetConsoleCursorPosition(pThreadParameter->StdHandle, CursorPosition);
		fprintf(pThreadParameter->StdHandle, "Thread %d (ID:%d, Priority:%d): %u ",
			pThreadParameter->Y, ObGetObjectId(PspCurrentThread),Priority, i);
		__asm("sti");
	}
	
	return 0;
}

PRIVATE
VOID
ConsoleCmdRoundRobin(
	IN HANDLE StdHandle
	)
/*++

����������
	����ʱ��Ƭ��ת���ȡ�����̨���rr����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	ULONG i;
	COORD CursorPosition;
	HANDLE ThreadHandleArray[10];
	THREAD_PARAMETER ThreadParameterArray[10];

	// ����������Ļ�����ݡ�
	for (i = 0; i < 24; i++) {
		fprintf(StdHandle, "\n");
	}

	// �½� 10 �����ȼ�Ϊ 8 ���̡߳��ر��жϴӶ���֤�½����̲߳���ִ�С�
	__asm("cli");
	for (i = 0; i < 10; i++) {
	
		ThreadParameterArray[i].Y = i;
		ThreadParameterArray[i].StdHandle = StdHandle;

		ThreadHandleArray[i] = (HANDLE)CreateThread(
			0, ThreadFunction, (PVOID)&ThreadParameterArray[i], 0, NULL);

		// �����̵߳����ȼ���
		PsSetThreadPriority(ThreadHandleArray[i], 8);
	}
	__asm("sti");
	
	// ��ǰ�̵߳ȴ�һ��ʱ�䡣���ڵ�ǰ�߳����ȼ� 24 �����½��̵߳����ȼ� 8��
	// ����ֻ���ڵ�ǰ�߳̽��롰������״̬���½����̲߳���ִ�С�
	Sleep(40 * 1000);
	
	// ��ǰ�̱߳����Ѻ󣬻���ռ��������ǿ�ƽ��������½����̡߳�
	for (i = 0; i < 10; i++) {
		TerminateThread(ThreadHandleArray[i], 0);
		CloseHandle(ThreadHandleArray[i]);
	}
	
	// �����ַ�����Ļ�������λ�á�
	CursorPosition.X = 0;
	CursorPosition.Y = 23;
	SetConsoleCursorPosition(StdHandle, CursorPosition);
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� loop ��صĴ��롣
//

PRIVATE
ULONG
LoopThreadFunction(
	PVOID Param
	)
/*++

����������
	��ѭ���̺߳�����

������
	Param -- δʹ�á�

����ֵ��
	���� 0 ��ʾ�߳�ִ�гɹ���

--*/
{
	ULONG i;
	ULONG ThreadID = GetCurrentThreadId();
	COORD CursorPosition;
	HANDLE StdHandle = (HANDLE)Param;

	// ����������Ļ�����ݡ�
	for (i = 0; i < 24; i++) {
		fprintf(StdHandle, "\n");
	}

	// �����߳����������ʾ��λ��
	CursorPosition.X = 0;
	CursorPosition.Y = 0;

	// ��ѭ����
	// ��ʽ��Thread ID �߳�ID : ִ�м���
	for (i=0;;i++) {
		SetConsoleCursorPosition(StdHandle, CursorPosition);
		fprintf(StdHandle, "Loop thread ID %d : %u ", ThreadID, i);
	}
	
	return 0;
}

PRIVATE
VOID
ConsoleCmdLoop(
	IN HANDLE StdHandle
	)
/*++

����������
	����̨���loop����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	HANDLE ThreadHandle;
	COORD CursorPosition;
	
	// �����̡߳�
	ThreadHandle = (HANDLE)CreateThread(
		0, LoopThreadFunction, (PVOID)StdHandle, 0, NULL);

	// �����̵߳����ȼ���
	PsSetThreadPriority(ThreadHandle, 8);
	
	// �ȴ��߳̽�����
	WaitForSingleObject(ThreadHandle, INFINITE);
	
	// �ر��߳̾����
	CloseHandle(ThreadHandle);
	
	// �����ַ�����Ļ�������λ�á�
	CursorPosition.X = 0;
	CursorPosition.Y = 23;
	SetConsoleCursorPosition(StdHandle, CursorPosition);
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� suspend �� resume ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdSuspendThread(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	)
/*++

����������
	������ ID ָ�����̡߳�����̨���suspend threadid����

������
	StdHandle -- ��׼���롢��������
	Arg -- ��������ַ�������Ҫ��������̵߳� ID��

����ֵ��
	�ޡ�

--*/
{
	ULONG ThreadID;
	HANDLE hThread;
	
	//
	// ����������ַ����л���߳� ID��
	//
	ThreadID = atoi(Arg);
	if(0 == ThreadID) {
		fprintf(StdHandle, "Please input a valid thread ID.\n");
		return;
	}
	
	//
	// ���߳� ID ����߳̾��
	//
	hThread = (HANDLE)OpenThread(ThreadID);
	if (NULL == hThread) {
		fprintf(StdHandle, "%d is an invalid thread ID.\n", ThreadID);
		return;
	}
		
	//
	// �����߳�
	//
	if (SuspendThread(hThread))
		fprintf(StdHandle, "Suspend thread(%d) success.\n", ThreadID);
	else
		fprintf(StdHandle, "Suspend thread(%d) fail.\n", ThreadID);
	
	//
	// �ر��߳̾��
	//
	CloseHandle(hThread);
}
	
PRIVATE
VOID
ConsoleCmdResumeThread(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	)
/*++

����������
	�ָ��� ID ָ�����̡߳�����̨���resume threadid����

������
	StdHandle -- ��׼���롢��������
	Arg -- ��������ַ�������Ҫ���ָ����̵߳� ID��

����ֵ��
	�ޡ�

--*/
{
	ULONG ThreadID;
	HANDLE hThread;
	
	//
	// ����������ַ����л���߳� ID��
	//
	ThreadID = atoi(Arg);
	if(0 == ThreadID) {
		fprintf(StdHandle, "Please input a valid thread ID.\n");
		return;
	}
	
	//
	// ���߳� ID ����߳̾��
	//
	hThread = (HANDLE)OpenThread(ThreadID);
	if (NULL == hThread) {
		fprintf(StdHandle, "%d is an invalid thread ID.\n", ThreadID);
		return;
	}
		
	//
	// �ָ��߳�
	//
	if (ResumeThread(hThread))
		fprintf(StdHandle, "Resume thread(%d) success.\n", ThreadID);
	else
		fprintf(StdHandle, "Resume thread(%d) fail.\n", ThreadID);
	
	//
	// �ر��߳̾��
	//
	CloseHandle(hThread);
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� vm ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdVM(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	)
/*++

����������
	ͳ������ɽ��� ID ָ���Ľ��̵������ַ����������Ϣ������̨���vm processid����

������
	StdHandle -- ��׼���롢��������
	Arg -- ��������ַ��������̵� ID��

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	ULONG ProcID;
	PPROCESS pProcCtrlBlock;
	PMMVAD_LIST pVadList;
	PLIST_ENTRY pListEntry;
	PMMVAD pVad;
	ULONG Index, TotalVpnCount, AllocatedVpnCount, FreeVpnCount, VpnCount;
	STATUS Status;
	
	//
	// ����������ַ����л�ý��� ID��
	//
	ProcID = atoi(Arg);
	if(0 == ProcID) {
		fprintf(StdHandle, "Please input a valid process ID.\n");
		return;
	}
	
	//
	// �ɽ��� ID ��ý��̿��ƿ�
	//
	Status = ObRefObjectById(ProcID, PspProcessType, (PVOID*)&pProcCtrlBlock);
	if (!EOS_SUCCESS(Status)) {
		fprintf(StdHandle, "%d is an invalid process ID.\n", ProcID);
		return;
	}
	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �����̿��ƿ��� VAD �����ָ�뱣���������������ʹ��
	//
	pVadList = &pProcCtrlBlock->Pas->VadList;
	
	//
	// ��� VAD �����м�¼����ʼҳ��ţ�����ҳ���
	//
	fprintf(StdHandle, "Total Vpn from %d to %d. (0x%X - 0x%X)\n\n",
		pVadList->StartingVpn, pVadList->EndVpn,
		pVadList->StartingVpn * PAGE_SIZE, (pVadList->EndVpn + 1) * PAGE_SIZE - 1);
	
	//
	// ���� VAD ����������� VAD ����ʼҳ��ţ�����ҳ��źͰ���������ҳ������
	//
	Index = AllocatedVpnCount = 0;
	for(pListEntry = pVadList->VadListHead.Next;
		pListEntry != &pVadList->VadListHead;
		pListEntry = pListEntry->Next) {
	
		Index++;
		pVad = CONTAINING_RECORD(pListEntry, MMVAD, VadListEntry);
		
		VpnCount = pVad->EndVpn - pVad->StartingVpn + 1;
		fprintf(StdHandle, "%d# Vad Include %d Vpn From %d to %d. (0x%X - 0x%X)\n",
			Index, VpnCount, pVad->StartingVpn, pVad->EndVpn,
			pVad->StartingVpn * PAGE_SIZE, (pVad->EndVpn + 1) * PAGE_SIZE - 1);
		
		AllocatedVpnCount += VpnCount;
	}
	
	//
	// ͳ������ҳ���������ѷ��������ҳ���δ���������ҳ��
	//
	TotalVpnCount = pVadList->EndVpn - pVadList->StartingVpn + 1;
	fprintf(StdHandle, "\nTotal Vpn Count: %d.\n", TotalVpnCount);
	fprintf(StdHandle, "Allocated Vpn Count: %d.\n", AllocatedVpnCount);
	FreeVpnCount = TotalVpnCount - AllocatedVpnCount;
	fprintf(StdHandle, "Free Vpn Count: %d.\n", FreeVpnCount);
	
	KeEnableInterrupts(IntState);	// ���ж�
	
	ObDerefObject(pProcCtrlBlock);
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� pm ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdPhysicalMemory(
	IN HANDLE StdHandle
	)
/*++

����������
	ͳ���������洢������Ϣ������̨���pm����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �������ҳ�����������ڴ����������ֽ�Ϊ��λ��
	//
	fprintf(StdHandle, "Page Count: %d.\n", MiTotalPageFrameCount);
	fprintf(StdHandle, "Memory Count: %d * %d = %d Byte.\n",
		MiTotalPageFrameCount, PAGE_SIZE,
		MiTotalPageFrameCount * PAGE_SIZE);
	
	//
	// �����ҳ�����Ϳ���ҳ����
	//
	fprintf(StdHandle, "\nZeroed Page Count: %d.\n", MiZeroedPageCount);
	fprintf(StdHandle, "Free Page Count: %d.\n", MiFreePageCount);
	
	//
	// �����ʹ�õ�����ҳ����
	//
	fprintf(StdHandle, "\nUsed Page Count: %d.\n", MiTotalPageFrameCount - MiZeroedPageCount - MiFreePageCount);
	
	KeEnableInterrupts(IntState);	// ���ж�
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� pt ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdProcAndThread(
	IN HANDLE StdHandle
	)
/*++

����������
	ͳ��������̺��߳���Ϣ������̨���pt����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PPROCESS pProc;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �������еĽ��̶������ÿ�����̵���Ϣ
	//
	fprintf(StdHandle, "******** Process List (%d Process) ********\n", PspProcessType->ObjectCount);
	fprintf(StdHandle, "ID | System? | Priority | ThreadCount | PrimaryThreadID | ImageName\n");
	
	for(pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// ��������ǽ��̿��ƿ�
		//
		pProc = (PPROCESS)&pObjectHeader->Body;
		
		//
		// ������̵���Ϣ
		//
		fprintf(StdHandle, "%d      %s         %d           %d             %d               %s\n",
			pObjectHeader->Id, pProc->System ? "Y" : "N", pProc->Priority,
			ListGetCount(&pProc->ThreadListHead), ObGetObjectId(pProc->PrimaryThread),
			(NULL == pProc->ImageName) ? "N\\A" : pProc->ImageName);
	}
	
	//
	// �������е��̶߳������ÿ���̵߳���Ϣ
	//
	fprintf(StdHandle, "\n******** Thread List (%d Thread) ********\n", PspThreadType->ObjectCount);
	fprintf(StdHandle, "ID | System? | Priority  |  State  |  ParentProcessID | StartAddress\n");
	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// ����������߳̿��ƿ�
		//
		pThread = (PTHREAD)&pObjectHeader->Body;
		
		//
		// ���߳�״̬ת��Ϊ�ַ���
		//
		switch(pThread->State) {
		
		case Zero:
			ThreadState = "Zero      ";
			break;
		case Ready:
			ThreadState = "Ready     ";
			break;
		case Running:
			ThreadState = "Running   ";
			break;
		case Waiting:
			ThreadState = "Waiting   ";
			break;
		case Terminated:
			ThreadState = "Terminated";
			break;;
		default:
			ThreadState = "Undefined ";
		}
		
		//
		// ����̵߳���Ϣ
		//
		fprintf(StdHandle, "%d      %s         %d         %s       %d           0x%X\n",
			pObjectHeader->Id, pThread->Process->System ? "Y" : "N", pThread->Priority,
			ThreadState, ObGetObjectId(pThread->Process), pThread->StartAddr);
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� dir ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdDir(
	IN HANDLE StdHandle
	)
/*++

����������
	������̸�Ŀ¼���ļ�����Ϣ������̨���dir����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	PDEVICE_OBJECT FatDevice;
	PVCB pVcb;
	PVOID pBuffer;
	SIZE_T BufferSize;
	PDIRENT pDirEntry;
	CHAR FileName[13];
	
	ULONG i, RootDirSectors;
	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �õ� FAT12 �ļ�ϵͳ�豸����Ȼ��õ�����ƿ� VCB
	//
	FatDevice = (PDEVICE_OBJECT)ObpLookupObjectByName(IopDeviceObjectType, "A:");
	pVcb = (PVCB)FatDevice->DeviceExtension;
	
	//
	// ����һ�������ڴ���Ϊ��������Ȼ��������Ŀ¼�������̶��뻺������
	//
	pBuffer = NULL;		// ��ָ���������ĵ�ַ����ϵͳ�����������ĵ�ַ��
	BufferSize = pVcb->RootDirSize;	// ����Ļ�������С���Ŀ¼����С��ͬ��
	MmAllocateVirtualMemory(&pBuffer, &BufferSize, MEM_RESERVE | MEM_COMMIT, TRUE);
	
	RootDirSectors = pVcb->RootDirSize / pVcb->Bpb.BytesPerSector;	// �����Ŀ¼��ռ�õ���������
	for(i=0; i<RootDirSectors; i++) {
		
		// ����Ŀ¼��ռ�õ��������뻺����
		IopReadWriteSector( pVcb->DiskDevice,
							pVcb->FirstRootDirSector + i,
							0,
							(PCHAR)pBuffer + pVcb->Bpb.BytesPerSector * i,
							pVcb->Bpb.BytesPerSector,
							TRUE);
	}
	
	//
	// ɨ�軺�����еĸ�Ŀ¼������Ŀ¼�е��ļ����ļ�����Ϣ
	//
	fprintf(StdHandle, "Name        |   Size(Byte) |    Last Write Time\n");
	for(i=0; i<pVcb->Bpb.RootEntries; i++) {
	
		pDirEntry = (PDIRENT)(pBuffer + 32 * i);
		
		//
		// ����δʹ�õ�Ŀ¼��ͱ�ɾ����Ŀ¼��
		//
		if(0x0 == pDirEntry->Name[0]
			|| (CHAR)0xE5 == pDirEntry->Name[0])
			continue;
		
		FatConvertDirNameToFileName(pDirEntry->Name, FileName);
		
		fprintf(StdHandle, "%s        %d         %d-%d-%d %d:%d:%d\n",
			FileName, pDirEntry->FileSize, 1980 + pDirEntry->LastWriteDate.Year,
			pDirEntry->LastWriteDate.Month, pDirEntry->LastWriteDate.Day,
			pDirEntry->LastWriteTime.Hour, pDirEntry->LastWriteTime.Minute,
			pDirEntry->LastWriteTime.DoubleSeconds);
	}
	
	//
	// �ͷŻ�����
	//
	BufferSize = 0;	// ��������С����Ϊ 0����ʾ�ͷ�ȫ��������
	MmFreeVirtualMemory(&pBuffer, &BufferSize, MEM_RELEASE, TRUE);
	
	KeEnableInterrupts(IntState);	// ���ж�
}


//////////////////////////////////////////////////////////////////////////
//
// �����ǺͿ���̨���� sd ��صĴ��롣
//

PRIVATE
VOID
ConsoleCmdScanDisk(
	IN HANDLE StdHandle
	)
/*++

����������
	ɨ�����̣�����������Ϣ������̨���sd����

������
	StdHandle -- ��׼���롢��������

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	PDEVICE_OBJECT FatDevice;
	PVCB pVcb;
	ULONG i, FreeClusterCount, UsedClusterCount;
	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �õ� FAT12 �ļ�ϵͳ�豸����Ȼ��õ�����ƿ� VCB
	//
	FatDevice = (PDEVICE_OBJECT)ObpLookupObjectByName(IopDeviceObjectType, "A:");
	pVcb = (PVCB)FatDevice->DeviceExtension;
	
	//
	// ������ƿ��л���� BIOS Parameter Block (BPB) ���Լ�����ƿ��е�������Ҫ��Ϣ���
	//
	fprintf(StdHandle, "******** BIOS Parameter Block (BPB) ********\n");
	fprintf(StdHandle, "Bytes Per Sector   : %d\n", pVcb->Bpb.BytesPerSector);
	fprintf(StdHandle, "Sectors Per Cluster: %d\n", pVcb->Bpb.SectorsPerCluster);
	fprintf(StdHandle, "Reserved Sectors   : %d\n", pVcb->Bpb.ReservedSectors);
	fprintf(StdHandle, "Fats               : %d\n", pVcb->Bpb.Fats);
	fprintf(StdHandle, "Root Entries       : %d\n", pVcb->Bpb.RootEntries);
	fprintf(StdHandle, "Sectors            : %d\n", pVcb->Bpb.Sectors);
	fprintf(StdHandle, "Media              : 0x%X\n", pVcb->Bpb.Media);
	fprintf(StdHandle, "Sectors Per Fat    : %d\n", pVcb->Bpb.SectorsPerFat);
	fprintf(StdHandle, "Sectors Per Track  : %d\n", pVcb->Bpb.SectorsPerTrack);
	fprintf(StdHandle, "Heads              : %d\n", pVcb->Bpb.Heads);
	fprintf(StdHandle, "Hidden Sectors     : %d\n", pVcb->Bpb.HiddenSectors);
	fprintf(StdHandle, "Large Sectors      : %d\n", pVcb->Bpb.LargeSectors);
	fprintf(StdHandle, "******** BIOS Parameter Block (BPB) ********\n\n");

	fprintf(StdHandle, "First Sector of Root Directroy: %d\n", pVcb->FirstRootDirSector);
	fprintf(StdHandle, "Size of Root Directroy        : %d\n", pVcb->RootDirSize);
	fprintf(StdHandle, "First Sector of Data Area     : %d\n", pVcb->FirstDataSector);
	fprintf(StdHandle, "Number Of Clusters            : %d\n\n", pVcb->NumberOfClusters);
	
	//
	// ɨ�� FAT ��ͳ�ƿ��дص����������������̿ռ��ʹ�����
	//
	FreeClusterCount = 0;
	for (i = 2; i < pVcb->NumberOfClusters + 2; i++) {
		if (0 == FatGetFatEntryValue(pVcb, i))
			FreeClusterCount++;
	}
	UsedClusterCount = pVcb->NumberOfClusters - FreeClusterCount;
	fprintf(StdHandle, "Free Cluster Count: %d (%d Byte)\n", FreeClusterCount, FreeClusterCount*pVcb->Bpb.SectorsPerCluster*pVcb->Bpb.BytesPerSector);
	fprintf(StdHandle, "Used Cluster Count: %d (%d Byte)\n", UsedClusterCount, UsedClusterCount*pVcb->Bpb.SectorsPerCluster*pVcb->Bpb.BytesPerSector);
	
	KeEnableInterrupts(IntState);	// ���ж�
}

#ifdef _DEBUG

typedef struct _PROCESSINFO {
	INT Id;
	INT Priority;
	INT PrimaryThreadID;
	INT ThreadCount;
	PSTR ImageName;
	INT SystemProc;
} PROCESSINFO, *PPROCESSINFO;

#define MAXPROCESSCOUNT 20
PROCESSINFO ProcArray[MAXPROCESSCOUNT];
INT PNum = 0, TNum = 0;

typedef struct _THREADINFO {
	INT Id;
	INT Priority;
	INT SystemThread;
	INT ParentProcessID;
	LIST_ENTRY StateListEntry;			// ����״̬���е�������
	INT State;
	PSTR StartAddress;
	PTHREAD_START_ROUTINE StartAddr;
} THREADINFO, *PTHREADINFO;

#define MAXTHREADCOUNT 100
THREADINFO ThreadArray[MAXTHREADCOUNT];

PRIVATE VOID GetProcAndThread( )
/*++

����������
	��ȡ���̺��߳���Ϣ��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PPROCESS pProc;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �������еĽ��̶���
	//
	PNum = 0;
	TNum = 0;
	for(pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PNum < MAXPROCESSCOUNT;
		pListEntry = pListEntry->Next, PNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
			
		//
		// ��������ǽ��̿��ƿ�
		//
		pProc = (PPROCESS)&pObjectHeader->Body;
		
		ProcArray[PNum].Id = pObjectHeader->Id;
		ProcArray[PNum].Priority = pProc->Priority;
		ProcArray[PNum].PrimaryThreadID = (pProc->PrimaryThread == 0) ? 0 : ObGetObjectId(pProc->PrimaryThread);
		ProcArray[PNum].ThreadCount = (pProc->PrimaryThread == 0) ? 0: ListGetCount(&pProc->ThreadListHead);
		ProcArray[PNum].ImageName = (pProc->PrimaryThread == 0) ? "N\\A" :((NULL == pProc->ImageName) ? "N\\A" : pProc->ImageName);
		ProcArray[PNum].SystemProc = pProc->System;
	}
	
	//
	// �������е��̶߳���
	//
	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead && TNum < MAXTHREADCOUNT;
		pListEntry = pListEntry->Next, TNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// ����������߳̿��ƿ�
		//
		pThread = (PTHREAD)&pObjectHeader->Body;
			
		ThreadArray[TNum].Id = pObjectHeader->Id;
		ThreadArray[TNum].Priority = pThread->Priority;
		ThreadArray[TNum].SystemThread = pThread->Process->System;
		ThreadArray[TNum].ParentProcessID = ObGetObjectId(pThread->Process);
		ThreadArray[TNum].State = pThread->State;
		ThreadArray[TNum].StartAddress = (PSTR)(pThread->StartAddr);
		ThreadArray[TNum].StateListEntry = pThread->StateListEntry;
		ThreadArray[TNum].StartAddr = pThread->StartAddr;
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

PTHREAD PspThreadArr[MAXTHREADCOUNT];

PRIVATE VOID GetThreadLink( )
/*++

����������
	��ȡ�߳�����

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	

	for(TNum = 0, pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead && TNum < MAXTHREADCOUNT;
		pListEntry = pListEntry->Next,TNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// ����������߳̿��ƿ�
		//
		pThread = (PTHREAD)&pObjectHeader->Body;	
		
		PspThreadArr[TNum] = pThread;		
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

PPROCESS PspProcessArr[MAXPROCESSCOUNT];

PRIVATE VOID GetProcessLink( )
/*++

����������
	��ȡ��������

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PPROCESS pProcess;
	PLIST_ENTRY pListEntry;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(PNum = 0, pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PNum < MAXPROCESSCOUNT;
		pListEntry = pListEntry->Next,PNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// ����������߳̿��ƿ�
		//
		pProcess = (PPROCESS)&pObjectHeader->Body;	
		
		PspProcessArr[PNum] = pProcess;		
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#define READYMAXTHREADCOUNT 20
#define PRIORITYCOUNT 32
typedef struct _PRIORITYREADYQUEUE
{
	INT Priority;
	INT Count;
	PTHREAD PThreadArr[READYMAXTHREADCOUNT];
}PRIORITYREADYQUEUE;

PRIORITYREADYQUEUE PriorityReadyQueue[PRIORITYCOUNT];

VOID GetPriorityReadyQueue()
/*++

����������
	�õ���ͬ���ȼ��ľ������С�

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry;
	INT RTNum = 0;
	INT PriNum = 0;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(PriNum = 0; PriNum < 32; PriNum++)
	{
		PriorityReadyQueue[PriNum].Priority = PriNum;
		if(ListIsEmpty(&PspReadyListHeads[PriNum])) {
			PriorityReadyQueue[PriNum].Count = 0;
			continue;
		}

		for(RTNum = 0, pListEntry = PspReadyListHeads[PriNum].Next;
		pListEntry != &PspReadyListHeads[PriNum] && RTNum < READYMAXTHREADCOUNT;
		pListEntry = pListEntry->Next, RTNum++) {
		
			//
			// ��ö���ͷ��ָ��
			//
			pThread = CONTAINING_RECORD(pListEntry, THREAD, StateListEntry);
			
			//
			// ����������߳̿��ƿ�
			//	
			PriorityReadyQueue[PriNum].PThreadArr[RTNum] = pThread;	
		}
		PriorityReadyQueue[PriNum].Count = RTNum; 
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#define MAXTHREADOBJCOUNT 100
INT ThreadObjIdArr[MAXTHREADOBJCOUNT];	// �̶߳���Id����
INT ThreadObjCount = 0;

PRIVATE VOID GetAllThreadObjId( )
/*++

����������
	��ȡ�����̶߳���ID��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
		
	//
	// �������е��̶߳���
	//
	
	for(ThreadObjCount = 0, pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead && ThreadObjCount < MAXTHREADOBJCOUNT;
		pListEntry = pListEntry->Next, ThreadObjCount++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		ThreadObjIdArr[ThreadObjCount] = pObjectHeader->Id;
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

INT MaxTid = 0;
PRIVATE VOID GetThreadObjMaxId( )
/*++

����������
	��ȡ�����̶߳���ID��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PLIST_ENTRY pListEntry;	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
		
	//
	// �������е��̶߳���
	//	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		if(MaxTid < pObjectHeader->Id)
		{
			MaxTid = pObjectHeader->Id;
		}	
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#define WAITTHREADMAXTHREADCOUNT 10

typedef	struct _THREADDETAILINFO {
	INT Id;
	INT Priority;
	INT SystemThread;
	INT ParentProcessID;
	INT State;
	PSTR StartAddress;
	PTHREAD_START_ROUTINE StartAddr;
	
	PPROCESS Process;					// �߳���������ָ��
	LIST_ENTRY ThreadListEntry;			// ���̵��߳�������

	ULONG RemainderTicks;				// ʣ��ʱ��Ƭ������ʱ��Ƭ��ת����
	STATUS WaitStatus;					// �����ȴ��Ľ��״̬
	KTIMER WaitTimer;					// �������޵ȴ����ѵļ�ʱ��
	LIST_ENTRY StateListEntry;			// ����״̬���е�������
	LIST_ENTRY WaitListHead;			// �ȴ����У����еȴ��߳̽������̶߳��ڴ˶��еȴ���
	INT WaitThreadCount;
	INT WaitThreadIdArr[WAITTHREADMAXTHREADCOUNT];
	PTHREAD PWaitThreadArr[WAITTHREADMAXTHREADCOUNT];
	
	//
	// Ϊ�˽ṹ�򵥣�EOSû�ж��ں˽��и��뱣���������̶߳��������ں�״̬������Ŀ
	// ǰ�߳�û���û��ռ��ջ��
	//
	PVOID KernelStack;					// �߳�λ���ں˿ռ��ջ
	CONTEXT KernelContext;				// �߳�ִ�����ں�״̬�������Ļ���״̬

	//
	// �̱߳������������̵ĵ�ַ�ռ���ִ���û����룬�������κν��̵ĵ�ַ�ռ���ִ��
	// �ں˴��룬��Ϊ�ں˴���λ�����н��̵�ַ�ռ乲���ϵͳ��ַ�ռ��С�
	//
	PMMPAS AttachedPas;					// �߳���ִ���ں˴���ʱ�󶨽��̵�ַ�ռ䡣

	PVOID Parameter;					// ���ݸ���ں����Ĳ���

	ULONG LastError;					// �߳����һ�εĴ�����
	ULONG ExitCode;						// �̵߳��˳���
} THREADDETAILINFO;

THREADDETAILINFO SelThreadInfo;


PRIVATE VOID GetSelThreadDetail(INT ThreadObjId)
/*++

����������
	��ȡѡȡ�̵߳���ϸ��Ϣ��

������
	�̶߳���Id

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PTHREAD pThread, pWThread;
	PLIST_ENTRY pListEntry;	
	PLIST_ENTRY pWListEntry;
	const char* ThreadState = NULL;
	INT WaitThreadNum;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
		
	//
	// �������е��̶߳��󣬵õ�ÿ���̵߳���Ϣ
	//	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		if(pObjectHeader->Id == ThreadObjId)
		{
			//
			// ����������߳̿��ƿ�
			//
			pThread = (PTHREAD)&pObjectHeader->Body;
					
			SelThreadInfo.Id = pObjectHeader->Id;
			SelThreadInfo.Priority = pThread->Priority;
			SelThreadInfo.SystemThread = pThread->Process->System;
			SelThreadInfo.ParentProcessID = ObGetObjectId(pThread->Process);
			SelThreadInfo.State = pThread->State;
			SelThreadInfo.StartAddress = (PSTR)(pThread->StartAddr);
			SelThreadInfo.StartAddr = pThread->StartAddr;
			SelThreadInfo.RemainderTicks = pThread->RemainderTicks;
			SelThreadInfo.LastError = pThread->LastError ;					
			SelThreadInfo.ExitCode = pThread->ExitCode;		
			SelThreadInfo.KernelContext	= pThread->KernelContext;		
			SelThreadInfo.WaitTimer = pThread->WaitTimer;	
			SelThreadInfo.AttachedPas = pThread->AttachedPas;
			SelThreadInfo.WaitListHead = pThread->WaitListHead;
			SelThreadInfo.StateListEntry = pThread->StateListEntry;
		
			WaitThreadNum = 0;
			if(pThread->WaitListHead.Prev != NULL && pThread->WaitListHead.Next != NULL)
			{
				for(WaitThreadNum = 0, pWListEntry = pThread->WaitListHead.Next;
				pWListEntry != &pThread->WaitListHead && WaitThreadNum < WAITTHREADMAXTHREADCOUNT;
				pWListEntry = pWListEntry->Next, ++WaitThreadNum)
				{
					pWThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
					SelThreadInfo.PWaitThreadArr[WaitThreadNum] = pWThread;
					SelThreadInfo.WaitThreadIdArr[WaitThreadNum] = ObGetObjectId(pWThread);
				}
			}	
			
			SelThreadInfo.WaitThreadCount = WaitThreadNum;
			
			break;
		}	
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#define MAXPROCOBJCOUNT 100

INT ProcObjIdArr[MAXPROCOBJCOUNT];	// ���̶���Id����
INT PObjCount = 0;

PRIVATE VOID GetAllProcObjId( )
/*++

����������
	��ȡ���н��̶���ID��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PLIST_ENTRY pListEntry;	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �������еĽ��̶��󣬵õ����н���ID
	//
	
	for(PObjCount = 0, pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PObjCount < MAXPROCOBJCOUNT;
		pListEntry = pListEntry->Next, ++PObjCount) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		 ProcObjIdArr[PObjCount] = pObjectHeader->Id;	
	}	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#define WAITPROCESSMAXTHREADCOUNT 20
//
// ���̶���ṹ�� (PCB)��
//
typedef struct _PROCESSDETAILINFO {
	INT Id;
	INT Priority;
	INT PrimaryThreadID;
	INT ThreadCount;
	PSTR ImageName;
	INT SystemProc;
	
	PMMPAS Pas;							// ���̵�ַ�ռ� 
	PHANDLE_TABLE ObjectTable;			// ���̵��ں˶�������
	LIST_ENTRY ThreadListHead;			// �߳�����ͷ
	PTHREAD PrimaryThread;				// ���߳�ָ��
	LIST_ENTRY WaitListHead;			// �ȴ����У����еȴ����̽������̶߳��ڴ˶��еȴ���
	INT WaitThreadCount;
	INT WaitThreadIdArr[WAITPROCESSMAXTHREADCOUNT];
	PTHREAD PWaitThreadArr[WAITPROCESSMAXTHREADCOUNT];

	PSTR CmdLine;						// �����в���
	PVOID ImageBase;					// ��ִ��ӳ��ļ��ػ�ַ
	PPROCESS_START_ROUTINE ImageEntry;	// ��ִ��ӳ�����ڵ�ַ

	HANDLE StdInput;
	HANDLE StdOutput;
	HANDLE StdError;

	ULONG ExitCode;						// �����˳���
} PROCESSDETAILINFO;

PROCESSDETAILINFO SelProcInfo;

PRIVATE VOID GetSelProcDetail(INT ProcObjId)
/*++

����������
	��ȡ��ѡ����̵���ϸ��Ϣ��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PPROCESS pProc;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry, pWListEntry;
	INT WaitThreadNum;
	PNum = 0;
	
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �������еĽ��̶���
	//
	
	for(pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PNum < MAXPROCESSCOUNT;
		pListEntry = pListEntry->Next, PNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		if(ProcObjId == pObjectHeader->Id)
		{
			//
			// ��������ǽ��̿��ƿ�
			//
			pProc = (PPROCESS)&pObjectHeader->Body;
			
			SelProcInfo.Id = pObjectHeader->Id;
			SelProcInfo.Priority = pProc->Priority;
			SelProcInfo.PrimaryThreadID = (pProc->PrimaryThread == 0) ? 0 : ObGetObjectId(pProc->PrimaryThread);
			SelProcInfo.ThreadCount = (pProc->PrimaryThread == 0) ? 0: ListGetCount(&pProc->ThreadListHead);
			SelProcInfo.ImageName = (pProc->PrimaryThread == 0) ? "N\\A" :((NULL == pProc->ImageName) ? "N\\A" : pProc->ImageName);
			SelProcInfo.SystemProc = pProc->System;
			SelProcInfo.Pas = pProc->Pas;
			SelProcInfo.ObjectTable = pProc->ObjectTable;
			SelProcInfo.ImageEntry = pProc->ImageEntry;
			SelProcInfo.WaitListHead = pProc->WaitListHead;
	
			WaitThreadNum = 0;
			if(pProc->WaitListHead.Prev != NULL && pProc->WaitListHead.Next != NULL)
			{
				for(WaitThreadNum = 0, pWListEntry = pProc->WaitListHead.Next;
				pWListEntry != &pProc->WaitListHead && WaitThreadNum < WAITPROCESSMAXTHREADCOUNT;
				pWListEntry = pWListEntry->Next, ++WaitThreadNum)
				{
					pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
					SelProcInfo.PWaitThreadArr[WaitThreadNum] = pThread;
					SelProcInfo.WaitThreadIdArr[WaitThreadNum] = ObGetObjectId(pThread);
				}
			}
				
			SelProcInfo.WaitThreadCount = WaitThreadNum;

			break;
		}	
	}	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#define MAXVADCOUNT 30
MMVAD pVadArr[MAXVADCOUNT];
INT VadCount = 0;
ULONG TotalVpnStart, TotalVpnEnd;
ULONG TotalVpnCount, AllocatedVpnCount, FreeVpnCount;

PRIVATE
VOID
GetVpn(
	ULONG ProcID
	)
/*++

����������
	ͳ������ɽ��� ID ָ���Ľ��̵������ַ����������Ϣ��

������
	ProcID�����̵� ID��

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	PPROCESS pProcCtrlBlock;
	PMMVAD_LIST pVadList;
	PLIST_ENTRY pListEntry;
	PMMVAD pVad;
	ULONG VpnCount;
	STATUS Status;
		
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �ɽ��� ID ��ý��̿��ƿ�
	//
	Status = ObRefObjectById(ProcID, PspProcessType, (PVOID*)&pProcCtrlBlock);
	
	//
	// �����̿��ƿ��� VAD �����ָ�뱣���������������ʹ��
	//
	pVadList = &pProcCtrlBlock->Pas->VadList;
	TotalVpnStart = pVadList->StartingVpn;
	TotalVpnEnd = pVadList->EndVpn;
	
	//
	// ���� VAD ����������� VAD ����ʼҳ��ţ�����ҳ��źͰ���������ҳ������
	//
	AllocatedVpnCount = 0;
	for(VadCount = 0, pListEntry = pVadList->VadListHead.Next;
		pListEntry != &pVadList->VadListHead && VadCount < MAXVADCOUNT;
		pListEntry = pListEntry->Next, VadCount++) {
		
		pVad = CONTAINING_RECORD(pListEntry, MMVAD, VadListEntry);
		
		VpnCount = pVad->EndVpn - pVad->StartingVpn + 1;
		pVadArr[VadCount].EndVpn = pVad->EndVpn;
		pVadArr[VadCount].StartingVpn = pVad->StartingVpn;
		AllocatedVpnCount += VpnCount;
	}
	
	
	//
	// ͳ������ҳ���������ѷ��������ҳ���δ���������ҳ��
	//
	TotalVpnCount = pVadList->EndVpn - pVadList->StartingVpn + 1;
	FreeVpnCount = TotalVpnCount - AllocatedVpnCount;
	
	ObDerefObject(pProcCtrlBlock);
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;	
}

/*
������һ��ҳ��Ļ���������СΪ 8 * 1024 �ֽڣ����������洢ҳĿ¼��ҳ���е����ݡ�
����ֻ�洢��Ч��ҳ����Ӷ����Դ��ӿ��ȡҳ����ٶȡ�
*/
struct Page_Table_Entry
{
	ULONG Index;
	ULONG Pte;
};

struct Page_Table_Entry Page_Table_Buffer[1024];
ULONG Page_Entry_Count;

VOID Snapshot_Page_Table(ULONG* Page_Table_Base)
{
	ULONG i, pte;
	BOOL IntState;
		
	Page_Entry_Count = 0;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(i = 0; i < PTE_PER_TABLE; i++)
	{
		pte = Page_Table_Base[i];
		
		/* ������Ч�� PTE */
		if(!(pte & 1))
			continue;
			
		Page_Table_Buffer[Page_Entry_Count].Index = i;
		Page_Table_Buffer[Page_Entry_Count].Pte = pte;
		Page_Entry_Count++;
	}	
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#define MAXSEMCOUNT 10
#define MAXSEMWAITCOUNT 10
typedef struct _SEMAINFO
{
	SEMAPHORE Sem;
	INT WaitObj[MAXSEMWAITCOUNT];
	INT Count;	
}SEMAINFO;

SEMAINFO SemInfoArr[MAXSEMCOUNT];

INT SemNum = 0;
INT WaitSemThreadNum = 0;
PRIVATE VOID GetSem( )
/*++

����������
	��ȡ��¼���ź�����Ϣ��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PSEMAPHORE pSem;
	PLIST_ENTRY pListEntry;
	SemNum = 0;
	WaitSemThreadNum = 0;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(pListEntry = PspSemaphoreType->ObjectListHead.Next;
		pListEntry != &PspSemaphoreType->ObjectListHead && SemNum < MAXSEMCOUNT;
		pListEntry = pListEntry->Next, SemNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
			
		//
		// ��¼���ź���
		//
		pSem = (PSEMAPHORE)&pObjectHeader->Body;
		
		SEMAPHORE Sem;
		
		Sem.Count = pSem->Count;
		Sem.MaximumCount = pSem->MaximumCount;
		Sem.WaitListHead = pSem->WaitListHead;
		
		PTHREAD pThread;
		SemInfoArr[SemNum].Sem = Sem;
		
		// �ȴ��߳�
		PLIST_ENTRY pWListEntry;

		for(WaitSemThreadNum = 0,pWListEntry = pSem->WaitListHead.Next;
		pWListEntry != &pSem->WaitListHead && WaitSemThreadNum < MAXSEMWAITCOUNT;
		pWListEntry = pWListEntry->Next, WaitSemThreadNum++)
		{
			//
			// ��ö���ͷ��ָ��
			//
			pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
			
			SemInfoArr[SemNum].WaitObj[WaitSemThreadNum] = ObGetObjectId(pThread);
		}
		SemInfoArr[SemNum].Count = WaitSemThreadNum;
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;	
}

#define MAXMUTEXCOUNT 10
#define MAXMUTEXWAITCOUNT 10

typedef struct _MUTEXINFO
{
	MUTEX Mutex;
	INT Tid;
	INT WaitObj[MAXMUTEXWAITCOUNT];
	INT Count;	
}MUTEXINFO;
MUTEXINFO MutexInfoArr[MAXMUTEXCOUNT];

INT MutexNum = 0;
INT WaitMutexThreadNum = 0;

PRIVATE VOID GetMutex( )
/*++

����������
	��ȡ�����ź�����Ϣ��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PMUTEX pMutex;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(MutexNum = 0, pListEntry = PspMutexType->ObjectListHead.Next;
		pListEntry != &PspMutexType->ObjectListHead && MutexNum < MAXMUTEXCOUNT;
		pListEntry = pListEntry->Next,MutexNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
			
		//
		// ��¼���ź���
		//
		pMutex = (PMUTEX)&pObjectHeader->Body;
		

		MutexInfoArr[MutexNum].Mutex.OwnerThread = pMutex->OwnerThread;
		MutexInfoArr[MutexNum].Mutex.RecursionCount = pMutex->RecursionCount;
		if(pMutex->OwnerThread != NULL)
		{
			MutexInfoArr[MutexNum].Tid = ObGetObjectId(pMutex->OwnerThread);
		}
		else
		{
			MutexInfoArr[MutexNum].Tid = -1;
		}
		
		PTHREAD pThread;
		// �ȴ��߳�
		PLIST_ENTRY pWListEntry;

		for(WaitMutexThreadNum = 0, pWListEntry = pMutex->WaitListHead.Next;
		pWListEntry != &pMutex->WaitListHead && WaitMutexThreadNum < MAXMUTEXWAITCOUNT;
		pWListEntry = pWListEntry->Next, WaitMutexThreadNum++)
		{
			//
			// ��ö���ͷ��ָ��
			//
			pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
			
			MutexInfoArr[MutexNum].WaitObj[WaitMutexThreadNum] = ObGetObjectId(pThread);
		}
		MutexInfoArr[MutexNum].Count = WaitMutexThreadNum;
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;	
}

ULONG SectionNum = 0;

struct Page_Section_Entry
{
	ULONG begin;
	ULONG end;
	ULONG state;
	ULONG count;
};

#define MAX_SECTION_COUNT 32
struct Page_Section_Entry page_section_table[MAX_SECTION_COUNT];

VOID GetPfn()
{	
	ULONG i = 0; 
	ULONG j = 0;
	INT count = 0;
	BOOL IntState;
	
	SectionNum = 0;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(i = 0; i < MiTotalPageFrameCount; i++)
	{
		count = 1;
		page_section_table[SectionNum].begin = i;	
		page_section_table[SectionNum].state = MiGetPfnDatabaseEntry(i)->PageState;
		if(MiGetPfnDatabaseEntry(i)->PageState == BUSY_PAGE)
		{	
			for(j = i + 1; j < MiTotalPageFrameCount; j++)
			{
				if(MiGetPfnDatabaseEntry(j)->PageState != BUSY_PAGE)
				{
					break;	
				}
				else
				{
					count++;
				}
			}	
		}
		else if(MiGetPfnDatabaseEntry(i)->PageState == FREE_PAGE)
		{
			for(j = i + 1; j < MiTotalPageFrameCount; j++)
			{
				if(MiGetPfnDatabaseEntry(j)->PageState != FREE_PAGE)
				{
					break;	
				}
				else
				{
					count++;
				}
			}
		}
		else if(MiGetPfnDatabaseEntry(i)->PageState == ZEROED_PAGE)
		{
			for(j = i + 1; j < MiTotalPageFrameCount; j++)
			{
				if(MiGetPfnDatabaseEntry(j)->PageState != ZEROED_PAGE)
				{
					break;	
				}
				else
				{
					count++;
				}
			}
		}
		page_section_table[SectionNum].end = j - 1;
		page_section_table[SectionNum].count = count;
		i = j - 1;
		SectionNum++;
		if(SectionNum >= MAX_SECTION_COUNT)
		{
			break;
		}
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

//typedef struct _PROCESSINFO {
//	INT Id;
//	INT Priority;
//	INT PrimaryThreadID;
//	INT ThreadCount;
//	PSTR ImageName;
//	INT SystemProc;
//} PROCESSINFO, *PPROCESSINFO;

#define MAXDEVICECOUNT 10
//PROCESSINFO ProcArray[MAXDEVICECOUNT];
INT DNum = 0;

typedef struct _DEVICE_OBJECT_INFO
{
	INT Id;
	PSTR Name;
	PDEVICE_OBJECT PDevice;
	
}DEVICE_OBJECT_INFO;

DEVICE_OBJECT_INFO DeviceArr[MAXDEVICECOUNT];

PRIVATE VOID GetDevice( )
/*++

����������
	��ȡ���̺��߳���Ϣ��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PDEVICE_OBJECT pDevice;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	//
	// �������еĽ��̶���
	//
	DNum = 0;
	for(pListEntry = IopDeviceObjectType->ObjectListHead.Next;
		pListEntry != &IopDeviceObjectType->ObjectListHead && DNum < MAXDEVICECOUNT;
		pListEntry = pListEntry->Next, DNum++) {
		
		//
		// ��ö���ͷ��ָ��
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		DeviceArr[DNum].Id = pObjectHeader->Id;
		DeviceArr[DNum].Name = pObjectHeader->Name;	
		//
		// ��������ǽ��̿��ƿ�
		//
		pDevice = (PDEVICE_OBJECT)&pObjectHeader->Body;
		DeviceArr[DNum].PDevice = pDevice;
	}
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}


#endif
