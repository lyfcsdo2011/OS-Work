/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: create.c

描述: 进程、线程的创建。



*******************************************************************************/

#include "psp.h"

PPROCESS PspSystemProcess = NULL;

VOID
PsCreateSystemProcess(
	IN PTHREAD_START_ROUTINE StartAddr
	)
/*++

功能描述：
	创建系统进程。

参数：
	StartAddr -- 系统进程入口地址。

返回值：
	无。

--*/
{
	STATUS Status;
	PTHREAD Thread;

	//
	// 系统进程唯一，只能被创建一次。
	//
	ASSERT(NULL == PspSystemProcess);

	//
	// 创建进程环境。
	//
	Status = PspCreateProcessEnvironment(24, NULL, NULL, &PspSystemProcess);

	ASSERT(EOS_SUCCESS(Status));

	//
	// 创建进程的主线程。
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

功能描述：
	创建一个应用进程。

参数：
	ImageName -- 进程的可执行文件的全路径名称，最长MAX_PATH，不能为空。
	CmdLine -- 命令行参数，最长1024个字符，可为NULL。
	CreateFlags -- 创建参数，目前尚无参数可选。
	StartupInfo -- 新建进程启动参数结构体，包含了新进程要使用的标准输入输出对象句柄。
	ProcInfo -- 指针，指向用于保存进程创建结果信息的结构体。

返回值：
	如果创建成功则返回STATUS_SUCCESS。

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
		// 在父（当前）进程的句柄表中为即将创建的子进程以及子进程的主线程对象分配两个句柄。
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
		// 根据指定的标准输入输出对象句柄得到对象的指针，并检查对象是否为
		// 有效的可读写对象。
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
		// 创建一个进程环境（进程的控制块以及进程的地址空间和句柄表）。
		//
		Status = PspCreateProcessEnvironment(8, ImageName, CmdLine, &ProcessObject);

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// 在新建进程的句柄表中为标准输入输出对象创建句柄。
		// 因为新建进程的句柄表目前还是空的，所以创建操作肯定不会失败。
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
		// 加载可执行映像（程序的指令和数据）到新建进程的用户地址空间中。
		//
		Status = PspLoadProcessImage( ProcessObject,
									  ProcessObject->ImageName,
									  &ProcessObject->ImageBase,
									  (PVOID*)&ProcessObject->ImageEntry );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// 创建新进程的主线程，所有进程的主线程都从函数PspProcessStartup开始执行。
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
		// 设置父进程中的句柄值，使之指向新建子进程对象及其主线程对象，这样父进
		// 程就可以通过句柄访问和控制子进程及其主线程了。
		//
		Status = ObSetHandleValueEx(PspCurrentProcess->ObjectTable, ProcessHandle, ProcessObject);
		ASSERT(EOS_SUCCESS(Status));

		Status = ObSetHandleValueEx(PspCurrentProcess->ObjectTable, ThreadHandle, ThreadObject);
		ASSERT(EOS_SUCCESS(Status));

		//
		// 设置返回结果（子进程及其主线程的句柄和ID）。
		//
		ProcInfo->ProcessHandle = ProcessHandle;
		ProcInfo->ThreadHandle = ThreadHandle;
		ProcInfo->ProcessId = ObGetObjectId(ProcessObject);
		ProcInfo->ThreadId = ObGetObjectId(ThreadObject);

		Status = STATUS_SUCCESS;

	} while (0);

	//
	// 如果创建过程失败，则对已进行的操作进行回滚。
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

功能描述：
	在当前进程内创建一个新线程。

参数：
	StackSize -- 用户模式线程栈的大小，如果当前进程是系统进程则忽略之。目前所有
		线程都执行在内核栈中，参数暂时无用。
	StartAddr -- 线程开始执行的函数的指针。
	ThreadParam -- 传递给线程函数的参数。
	CreateFlags -- 创建参数，目前尚无参数可选。
	ThreadHandle -- 指针，指向用于保存线程句柄的变量。
	ThreadId -- 指针，指向用于保存线程ID的变量，可选。

返回值：
	如果创建线程成功，则返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	HANDLE Handle;
	PTHREAD ThreadObject;

	if (NULL == StartAddr) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 先为即将创建的线程对象分配一个句柄。
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
	// 如果创建线程失败则释放之前创建的句柄并返回。
	//
	if (!EOS_SUCCESS(Status)) {
		ObFreeHandleEx(PspCurrentProcess->ObjectTable, Handle);
		return Status;
	}

	//
	// 设置句柄的值，使句柄指向新创建的线程对象。
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

功能描述：
	创建一个进程环境，包括进程的控制块、地址空间和句柄表。

参数：
	Priority -- 进程的基本优先级，进程内创建的线程将默认继承这个优先级。
	ImageName -- 进程的可执行文件的全路径名称，最长MAX_PATH，不能为空。
	CmdLine -- 命令行参数，最长1024个字符，可为NULL。
	Process -- 用于输出进程对象的指针

返回值：
	如果创建成功则返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	BOOL IntState;
	SIZE_T ImageNameSize;
	SIZE_T CmdLineSize;
	PPROCESS NewProcess;

	ASSERT(Priority <= 31);

	//
	// 计算可执行文件名称和命令行参数字符串占用的内存。
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
		// ImageName 为 NULL 则忽略 CmdLine。
		//
		ImageNameSize = 0;
		CmdLineSize = 0;
	}

	//
	// 开始原子操作，禁止中断。
	//
	IntState = KeEnableInterrupts(FALSE);

	do {

		//
		// 创建一个进程对象。
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
		// 如果是系统进程则直接使用系统进程地址空间，否则新创建一个进程地址空间。
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
		// 为进程分配一个句柄表。
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
		// 初始化进程控制块。
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
		// 如果可执行映像和命令行参数字符串非空则拷贝之。
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
		// 在被调用PspDeleteProcessEnvironment之前，进程留一份对自己的引用。
		//
		ObRefObject(NewProcess);

		//
		// 设置返回值。
		//
		*Process = NewProcess;
		Status = STATUS_SUCCESS;

	} while (0);

	KeEnableInterrupts(IntState);	// 原子操作完成，恢复中断。

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

功能描述：
	在指定进程内创建一个线程。

参数：
	Process -- 进程对象指针，新创建的线程将属于这个进程。
	StackSize -- 用户模式线程栈大小，目前线程都执行在内核态，参数暂时无用。
	StartAddr -- 线程开始执行的函数的指针。
	ThreadParam -- 传递给线程函数的参数。
	CreateFlags -- 创建参数，目前尚无参数可选。
	Thread -- 用于输出新线程对象的指针。

返回值：
	如果创建线程成功，则返回 STATUS_SUCCESS。

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
		// 创建线程对象
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
		// 为线程分配执行在内核态时所需的内核栈。
		// 内核栈位于系统地址空间中，目前大小固定为8KB。
		//
		NewThread->KernelStack = NULL;
		KernelStackSize = KERNEL_STACK_SIZE;

		Status = MmAllocateVirtualMemory( &NewThread->KernelStack,
										  &KernelStackSize,
										  MEM_RESERVE | MEM_COMMIT,
										  TRUE );

		if (!EOS_SUCCESS(Status)) {
			ObDerefObject(NewThread); // 释放已创建的线程对象。
			break;
		}

		//
		// 如果是用户线程，还需在用户地址空间中为线程分配用户模式栈。
		// 目前所有线程都一直执行在内核模式栈中，没有用户模式栈，忽略此操作。
		//
		/*if (!Process->System) {
		}*/

		//
		// 初始化线程控制块。
		//
		NewThread->Process = Process;
		NewThread->Priority = Process->Priority;	// 线程继承进程的优先级。
		NewThread->StartAddr = StartAddr;			// 线程的开始执行地址。
		NewThread->Parameter = ThreadParam;			// 线程函数的参数。
		NewThread->State = Zero;
		NewThread->RemainderTicks = TICKS_OF_TIME_SLICE;
		NewThread->WaitStatus = 0;
		NewThread->LastError = 0;
		NewThread->ExitCode = 0;
		ListInitializeHead(&NewThread->WaitListHead);
		PspInitializeThreadContext(NewThread);		// 初始化线程的处理器上下文。
		NewThread->AttachedPas = Process->Pas;		// 线程在所在进程的地址空间中执行。

		//
		// 将线程插入所在进程的线程链表。
		//
		ListInsertTail(&Process->ThreadListHead, &NewThread->ThreadListEntry);

		//
		// 使线程进入就绪状态。
		//
		PspReadyThread(NewThread);
		
		
		//
		// 线程在结束前保留一份对自己的引用。
		//
		ObRefObject(NewThread);

		//
		// 有线程进入就绪状态，执行线程调度。
		//
		PspThreadSchedule();

		//
		// 设置返回值。
		//
		*Thread = NewThread;
		Status = STATUS_SUCCESS;

	} while (0);

	KeEnableInterrupts(IntState);

	return Status;
}
