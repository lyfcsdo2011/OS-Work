/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: sysproc.c

描述: 系统进程函数，控制台线程函数，及各个控制台命令对应函数。



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
	// 创建初始化线程，由初始化线程来执行第二步初始化。
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
	// 将当前线程优先级降至最低，当前线程做为空闲线程进入空闲循环。
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
	// 执行第二步初始化，可以调用会阻塞当前线程的函数，可以创建各个模块内部的系
	// 统线程(例如内存管理模块的0页初始化线程)。
	//
	IoInitializeSystem2();
	MmInitializeSystem2();
	ObInitializeSystem2();
	PsInitializeSystem2();

	//
	// 为 4 个控制台各创建一个线程。
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
	// 以线程参数作为序号打开控制台作为标准输入输出句柄，
	//
	Status = IoOpenConsole((ULONG)Parameter, &StdHandle);
	ASSERT(EOS_SUCCESS(Status));
	fprintf(StdHandle, "Welcome to EOS shell\n");

	//
	// 第0个控制台尝试打开文件a:\autorun.txt作为输入句柄。
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
		// 读取输入行。如果fgets()返回NULL，则说明读取文件autorun.txt结束，此时
		// 应更换控制台句柄为输入句柄继续。
		//
		while (NULL == fgets(InputHandle, Line)) {
			ASSERT(0 == (ULONG)Parameter && InputHandle != StdHandle);
			InputHandle = StdHandle;
		}

		//
		// 如果读取的是Autorun.txt的内容则回显行。
		//
		if (InputHandle != StdHandle) {
			fprintf(StdHandle, "Autorun %s\n", Line);
		}

		//
		// 跳过空行。
		//
		if (0 == strlen(Line)) {
			continue;
		}

		//
		// 找到行中的第一个空格并在空格处将行截断，空格前是可执行文件名，后面是
		// 传递给程序的参数字符串。
		//
		for (Arg = Line; *Arg != '\0' && *Arg != ' ' && *Arg != '\t'; Arg++);

		if (' ' == *Arg || '\t' == *Arg) {
			*Arg++ = '\0';
		}

		//
		// 在此添加控制台命令
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
		// 将可执行文件名从Line中拷贝至Image中。如果仅给出了文件名则自动在文件名
		// 前面加上a盘根目录的路径，如果没有给出“.exe”扩展名则自动添加。
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
		// 等待进程结束，然后关闭进程和主线程的句柄。
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
// 下面是和控制台命令 ver 相关的代码。
//

PRIVATE
VOID
ConsoleCmdVersionNumber(
	IN HANDLE StdHandle
	)
/*++

功能描述：
	打印输出 EOS 操作系统的版本号。控制台命令“ver”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

--*/
{
	fprintf(StdHandle, "\nEngintime EOS [Version Number 1.2]\n\n");
}

//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 mm 相关的代码。
//

PRIVATE
VOID
ConsoleCmdMemoryMap(
	IN HANDLE StdHandle
	)
/*++

功能描述：
	转储系统进程的二级页表映射信息。控制台命令“mm”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

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

	IntState = KeEnableInterrupts(FALSE);	// 关中断

	//
	// 输出页目录的页框号
	//
	OutputFormat = "\nCR3->0x%X\n";
	PfnOfPageDirectory = (ULONG)(PspSystemProcess->Pas->PfnOfPageDirectory);
	fprintf(StdHandle, OutputFormat, PfnOfPageDirectory);

	//
	// 遍历页目录中的 PDE
	//
	for(IndexOfDirEntry = 0; IndexOfDirEntry < PTE_PER_TABLE; IndexOfDirEntry++)
	{
		pPde = (PMMPTE_HARDWARE)((ULONG_PTR)PDE_BASE + IndexOfDirEntry * PTE_SIZE);

		//
		// 跳过无效的 PDE
		//
		if(!pPde->Valid)
			continue;

		//
		// 输出 PDE 信息，格式如下：
		// PDE: 标号 (映射的 4M 虚拟地址的基址)->所映射页表的页框号
		//
		OutputFormat = "PDE: 0x%X (0x%X)->0x%X\n";
		VirtualBase = (IndexOfDirEntry << PDI_SHIFT);
		fprintf(StdHandle, OutputFormat, IndexOfDirEntry, VirtualBase, pPde->PageFrameNumber);

		//
		// 根据 PDE 的标号计算其映射的页表所在虚拟地址的基址
		//
		PageTableBase = (ULONG_PTR)PTE_BASE + IndexOfDirEntry * PAGE_SIZE;

		//
		// 遍历页表中的 PTE
		//
		for(IndexOfTableEntry = 0; IndexOfTableEntry < PTE_PER_TABLE; IndexOfTableEntry++)
		{
			pPte = (PMMPTE_HARDWARE)(PageTableBase + IndexOfTableEntry * PTE_SIZE);

			//
			// 跳过无效的 PTE
			//
			if(!pPte->Valid)
				continue;

			//
			// 输出 PTE 信息，格式如下：
			// PTE: 标号 (映射的 4K 虚拟地址的基址)->所映射物理页的页框号
			//
			OutputFormat = "\t\tPTE: 0x%X (0x%X)->0x%X\n";
			VirtualBase = (IndexOfDirEntry << PDI_SHIFT) | (IndexOfTableEntry << PTI_SHIFT);
			fprintf(StdHandle, OutputFormat, IndexOfTableEntry, VirtualBase, pPte->PageFrameNumber);

			//
			// 统计占用的物理页数
			//
			PageTotal++;
		}
	}

	//
	// 输出占用的物理页数，和物理内存数
	//
	OutputFormat = "\nPhysical Page Total: %d\n";
	fprintf(StdHandle, OutputFormat, PageTotal);
	OutputFormat = "Physical Memory Total: %d\n\n";
	fprintf(StdHandle, OutputFormat, PageTotal * PAGE_SIZE);

	KeEnableInterrupts(IntState);	// 开中断
}

//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 ds 相关的代码。
//

PRIVATE
ULONG
AccessCylinderThread(
	IN PVOID Param
	)
/*++

功能描述：
	线程函数。访问软盘上指定的磁道。

参数：
	Param -- 线程参数。保存了线程要访问的磁道号。

返回值：
	返回 0 表示线程执行成功。

--*/
{
	BYTE TempBuffer[4];
	PDEVICE_OBJECT FloppyDevice;
	ULONG Cylinder = (ULONG)Param;	// 线程参数保存了线程要访问的磁道号
	
	//
	// 得到软盘驱动器设备对象
	//
	FloppyDevice = (PDEVICE_OBJECT)ObpLookupObjectByName(IopDeviceObjectType, "FLOPPY0");
	
	//
	// 从指定磁道的第一个扇区读取一个字节的数据。
	// 重要的是让磁头移动到指定的磁道，所以并不关心读取的数据。
	// 如果有其它线程正在访问磁盘，则本线程就会被阻塞。
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

功能描述：
	创建一个访问指定磁道的线程，并输出信息。

参数：
	StdHandle -- 标准输入、输出句柄。
	Cylinder -- 新建线程要访问的磁道号。

返回值：
	无。

--*/
{
	HANDLE ThreadHandle;
	ULONG ThreadID;
	
	//
	// 新建一个线程，让新线程访问指定的磁道。
	// 注意，线程要访问的磁道号是通过线程参数传入线程函数的。
	//
	ThreadHandle = (HANDLE)CreateThread(0, AccessCylinderThread,
										(PVOID)Cylinder, 0, &ThreadID);
	PsSetThreadPriority(ThreadHandle, 31);	// 提升新建线程优先级，保证其被优先运行。
	CloseHandle(ThreadHandle);

	//
	// 输出线程信息。
	// 格式：TID: 线程ID Cylinder: 访问磁道号
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

功能描述：
	测试磁盘调度算法。控制台命令“ds”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

--*/
{
	PREQUEST pNextRequest;
	extern BOOL IsDeviceBusy;
	
	//
	// 让当前线程访问一下 10 磁道，也就设置了磁头的初始位置。
	//
#ifdef _DEBUG
	ThreadSeq = 0;
#endif
	
	ULONG StartCylinder = 10;
	AccessCylinderThread((PVOID)StartCylinder);
	fprintf(StdHandle, "Start Cylinder: %d\n", StartCylinder);

	//
	// 将磁盘设备设置为忙。
	//
	IsDeviceBusy = TRUE;
	
	//
	// 创建多个访问不同磁道的线程。由于设备忙，这些线程的请求
	// 都会被放入请求队列中，直到被磁盘调度算法选中后才会被处理。
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
	// 触发磁盘调度算法，按照调度策略依次处理请求队列中所有的请求。
	//
	pNextRequest = IopDiskSchedule();
	PsSetEvent(&pNextRequest->Event);
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 rr 相关的代码。
//

//
// 线程参数结构体
//
typedef struct _THREAD_PARAMETER {
	SHORT Y;				// 线程输出内容所在行
	HANDLE StdHandle;		// 线程用来输出内容的标准句柄
}THREAD_PARAMETER, *PTHREAD_PARAMETER;

PRIVATE
ULONG
ThreadFunction(
	PVOID Param
	)
/*++

功能描述：
	线程函数。

参数：
	Param -- 线程参数。是一个指向 THREAD_PARAMETER 结构体变量的指针。

返回值：
	返回 0 表示线程执行成功。

--*/
{
	ULONG i;
	UCHAR Priority;
	COORD CursorPosition;
	PTHREAD_PARAMETER pThreadParameter = (PTHREAD_PARAMETER)Param;

	// 根据线程参数设置输出内容显示的位置。
	CursorPosition.X = 0;
	CursorPosition.Y = pThreadParameter->Y;

	// 在线程参数指定的行循环显示线程执行的状态。死循环。通过开关中断互斥访问控制台。
	// 格式：Thread 序号 (优先级): 执行计数
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

功能描述：
	测试时间片轮转调度。控制台命令“rr”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

--*/
{
	ULONG i;
	COORD CursorPosition;
	HANDLE ThreadHandleArray[10];
	THREAD_PARAMETER ThreadParameterArray[10];

	// 清理整个屏幕的内容。
	for (i = 0; i < 24; i++) {
		fprintf(StdHandle, "\n");
	}

	// 新建 10 个优先级为 8 的线程。关闭中断从而保证新建的线程不会执行。
	__asm("cli");
	for (i = 0; i < 10; i++) {
	
		ThreadParameterArray[i].Y = i;
		ThreadParameterArray[i].StdHandle = StdHandle;

		ThreadHandleArray[i] = (HANDLE)CreateThread(
			0, ThreadFunction, (PVOID)&ThreadParameterArray[i], 0, NULL);

		// 设置线程的优先级。
		PsSetThreadPriority(ThreadHandleArray[i], 8);
	}
	__asm("sti");
	
	// 当前线程等待一段时间。由于当前线程优先级 24 高于新建线程的优先级 8，
	// 所以只有在当前线程进入“阻塞”状态后，新建的线程才能执行。
	Sleep(40 * 1000);
	
	// 当前线程被唤醒后，会抢占处理器。强制结束所有新建的线程。
	for (i = 0; i < 10; i++) {
		TerminateThread(ThreadHandleArray[i], 0);
		CloseHandle(ThreadHandleArray[i]);
	}
	
	// 设置字符在屏幕上输出的位置。
	CursorPosition.X = 0;
	CursorPosition.Y = 23;
	SetConsoleCursorPosition(StdHandle, CursorPosition);
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 loop 相关的代码。
//

PRIVATE
ULONG
LoopThreadFunction(
	PVOID Param
	)
/*++

功能描述：
	死循环线程函数。

参数：
	Param -- 未使用。

返回值：
	返回 0 表示线程执行成功。

--*/
{
	ULONG i;
	ULONG ThreadID = GetCurrentThreadId();
	COORD CursorPosition;
	HANDLE StdHandle = (HANDLE)Param;

	// 清理整个屏幕的内容。
	for (i = 0; i < 24; i++) {
		fprintf(StdHandle, "\n");
	}

	// 设置线程输出内容显示的位置
	CursorPosition.X = 0;
	CursorPosition.Y = 0;

	// 死循环。
	// 格式：Thread ID 线程ID : 执行计数
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

功能描述：
	控制台命令“loop”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

--*/
{
	HANDLE ThreadHandle;
	COORD CursorPosition;
	
	// 创建线程。
	ThreadHandle = (HANDLE)CreateThread(
		0, LoopThreadFunction, (PVOID)StdHandle, 0, NULL);

	// 设置线程的优先级。
	PsSetThreadPriority(ThreadHandle, 8);
	
	// 等待线程结束。
	WaitForSingleObject(ThreadHandle, INFINITE);
	
	// 关闭线程句柄。
	CloseHandle(ThreadHandle);
	
	// 设置字符在屏幕上输出的位置。
	CursorPosition.X = 0;
	CursorPosition.Y = 23;
	SetConsoleCursorPosition(StdHandle, CursorPosition);
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 suspend 和 resume 相关的代码。
//

PRIVATE
VOID
ConsoleCmdSuspendThread(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	)
/*++

功能描述：
	挂起由 ID 指定的线程。控制台命令“suspend threadid”。

参数：
	StdHandle -- 标准输入、输出句柄。
	Arg -- 命令参数字符串。需要被挂起的线程的 ID。

返回值：
	无。

--*/
{
	ULONG ThreadID;
	HANDLE hThread;
	
	//
	// 从命令参数字符串中获得线程 ID。
	//
	ThreadID = atoi(Arg);
	if(0 == ThreadID) {
		fprintf(StdHandle, "Please input a valid thread ID.\n");
		return;
	}
	
	//
	// 由线程 ID 获得线程句柄
	//
	hThread = (HANDLE)OpenThread(ThreadID);
	if (NULL == hThread) {
		fprintf(StdHandle, "%d is an invalid thread ID.\n", ThreadID);
		return;
	}
		
	//
	// 挂起线程
	//
	if (SuspendThread(hThread))
		fprintf(StdHandle, "Suspend thread(%d) success.\n", ThreadID);
	else
		fprintf(StdHandle, "Suspend thread(%d) fail.\n", ThreadID);
	
	//
	// 关闭线程句柄
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

功能描述：
	恢复由 ID 指定的线程。控制台命令“resume threadid”。

参数：
	StdHandle -- 标准输入、输出句柄。
	Arg -- 命令参数字符串。需要被恢复的线程的 ID。

返回值：
	无。

--*/
{
	ULONG ThreadID;
	HANDLE hThread;
	
	//
	// 从命令参数字符串中获得线程 ID。
	//
	ThreadID = atoi(Arg);
	if(0 == ThreadID) {
		fprintf(StdHandle, "Please input a valid thread ID.\n");
		return;
	}
	
	//
	// 由线程 ID 获得线程句柄
	//
	hThread = (HANDLE)OpenThread(ThreadID);
	if (NULL == hThread) {
		fprintf(StdHandle, "%d is an invalid thread ID.\n", ThreadID);
		return;
	}
		
	//
	// 恢复线程
	//
	if (ResumeThread(hThread))
		fprintf(StdHandle, "Resume thread(%d) success.\n", ThreadID);
	else
		fprintf(StdHandle, "Resume thread(%d) fail.\n", ThreadID);
	
	//
	// 关闭线程句柄
	//
	CloseHandle(hThread);
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 vm 相关的代码。
//

PRIVATE
VOID
ConsoleCmdVM(
	IN HANDLE StdHandle,
	IN PCSTR Arg
	)
/*++

功能描述：
	统计输出由进程 ID 指定的进程的虚拟地址描述符的信息。控制台命令“vm processid”。

参数：
	StdHandle -- 标准输入、输出句柄。
	Arg -- 命令参数字符串。进程的 ID。

返回值：
	无。

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
	// 从命令参数字符串中获得进程 ID。
	//
	ProcID = atoi(Arg);
	if(0 == ProcID) {
		fprintf(StdHandle, "Please input a valid process ID.\n");
		return;
	}
	
	//
	// 由进程 ID 获得进程控制块
	//
	Status = ObRefObjectById(ProcID, PspProcessType, (PVOID*)&pProcCtrlBlock);
	if (!EOS_SUCCESS(Status)) {
		fprintf(StdHandle, "%d is an invalid process ID.\n", ProcID);
		return;
	}
	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 将进程控制块中 VAD 链表的指针保存下来，方便后面使用
	//
	pVadList = &pProcCtrlBlock->Pas->VadList;
	
	//
	// 输出 VAD 链表中记录的起始页框号，结束页框号
	//
	fprintf(StdHandle, "Total Vpn from %d to %d. (0x%X - 0x%X)\n\n",
		pVadList->StartingVpn, pVadList->EndVpn,
		pVadList->StartingVpn * PAGE_SIZE, (pVadList->EndVpn + 1) * PAGE_SIZE - 1);
	
	//
	// 遍历 VAD 链表，输出所有 VAD 的起始页框号，结束页框号和包含的虚拟页框数量
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
	// 统计虚拟页框总数、已分配的虚拟页框和未分配的虚拟页框
	//
	TotalVpnCount = pVadList->EndVpn - pVadList->StartingVpn + 1;
	fprintf(StdHandle, "\nTotal Vpn Count: %d.\n", TotalVpnCount);
	fprintf(StdHandle, "Allocated Vpn Count: %d.\n", AllocatedVpnCount);
	FreeVpnCount = TotalVpnCount - AllocatedVpnCount;
	fprintf(StdHandle, "Free Vpn Count: %d.\n", FreeVpnCount);
	
	KeEnableInterrupts(IntState);	// 开中断
	
	ObDerefObject(pProcCtrlBlock);
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 pm 相关的代码。
//

PRIVATE
VOID
ConsoleCmdPhysicalMemory(
	IN HANDLE StdHandle
	)
/*++

功能描述：
	统计输出物理存储器的信息。控制台命令“pm”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

--*/
{
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 输出物理页数量和物理内存数量（以字节为单位）
	//
	fprintf(StdHandle, "Page Count: %d.\n", MiTotalPageFrameCount);
	fprintf(StdHandle, "Memory Count: %d * %d = %d Byte.\n",
		MiTotalPageFrameCount, PAGE_SIZE,
		MiTotalPageFrameCount * PAGE_SIZE);
	
	//
	// 输出零页数量和空闲页数量
	//
	fprintf(StdHandle, "\nZeroed Page Count: %d.\n", MiZeroedPageCount);
	fprintf(StdHandle, "Free Page Count: %d.\n", MiFreePageCount);
	
	//
	// 输出已使用的物理页数量
	//
	fprintf(StdHandle, "\nUsed Page Count: %d.\n", MiTotalPageFrameCount - MiZeroedPageCount - MiFreePageCount);
	
	KeEnableInterrupts(IntState);	// 开中断
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 pt 相关的代码。
//

PRIVATE
VOID
ConsoleCmdProcAndThread(
	IN HANDLE StdHandle
	)
/*++

功能描述：
	统计输出进程和线程信息。控制台命令“pt”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PPROCESS pProc;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 遍历所有的进程对象，输出每个进程的信息
	//
	fprintf(StdHandle, "******** Process List (%d Process) ********\n", PspProcessType->ObjectCount);
	fprintf(StdHandle, "ID | System? | Priority | ThreadCount | PrimaryThreadID | ImageName\n");
	
	for(pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// 对象体就是进程控制块
		//
		pProc = (PPROCESS)&pObjectHeader->Body;
		
		//
		// 输出进程的信息
		//
		fprintf(StdHandle, "%d      %s         %d           %d             %d               %s\n",
			pObjectHeader->Id, pProc->System ? "Y" : "N", pProc->Priority,
			ListGetCount(&pProc->ThreadListHead), ObGetObjectId(pProc->PrimaryThread),
			(NULL == pProc->ImageName) ? "N\\A" : pProc->ImageName);
	}
	
	//
	// 遍历所有的线程对象，输出每个线程的信息
	//
	fprintf(StdHandle, "\n******** Thread List (%d Thread) ********\n", PspThreadType->ObjectCount);
	fprintf(StdHandle, "ID | System? | Priority  |  State  |  ParentProcessID | StartAddress\n");
	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// 对象体就是线程控制块
		//
		pThread = (PTHREAD)&pObjectHeader->Body;
		
		//
		// 将线程状态转换为字符串
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
		// 输出线程的信息
		//
		fprintf(StdHandle, "%d      %s         %d         %s       %d           0x%X\n",
			pObjectHeader->Id, pThread->Process->System ? "Y" : "N", pThread->Priority,
			ThreadState, ObGetObjectId(pThread->Process), pThread->StartAddr);
	}
	
	KeEnableInterrupts(IntState);	// 开中断
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 dir 相关的代码。
//

PRIVATE
VOID
ConsoleCmdDir(
	IN HANDLE StdHandle
	)
/*++

功能描述：
	输出软盘根目录中文件的信息。控制台命令“dir”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

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
	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 得到 FAT12 文件系统设备对象，然后得到卷控制块 VCB
	//
	FatDevice = (PDEVICE_OBJECT)ObpLookupObjectByName(IopDeviceObjectType, "A:");
	pVcb = (PVCB)FatDevice->DeviceExtension;
	
	//
	// 分配一块虚拟内存做为缓冲区，然后将整个根目录区从软盘读入缓冲区。
	//
	pBuffer = NULL;		// 不指定缓冲区的地址。由系统决定缓冲区的地址。
	BufferSize = pVcb->RootDirSize;	// 申请的缓冲区大小与根目录区大小相同。
	MmAllocateVirtualMemory(&pBuffer, &BufferSize, MEM_RESERVE | MEM_COMMIT, TRUE);
	
	RootDirSectors = pVcb->RootDirSize / pVcb->Bpb.BytesPerSector;	// 计算根目录区占用的扇区数量
	for(i=0; i<RootDirSectors; i++) {
		
		// 将根目录区占用的扇区读入缓冲区
		IopReadWriteSector( pVcb->DiskDevice,
							pVcb->FirstRootDirSector + i,
							0,
							(PCHAR)pBuffer + pVcb->Bpb.BytesPerSector * i,
							pVcb->Bpb.BytesPerSector,
							TRUE);
	}
	
	//
	// 扫描缓冲区中的根目录项，输出根目录中的文件和文件夹信息
	//
	fprintf(StdHandle, "Name        |   Size(Byte) |    Last Write Time\n");
	for(i=0; i<pVcb->Bpb.RootEntries; i++) {
	
		pDirEntry = (PDIRENT)(pBuffer + 32 * i);
		
		//
		// 跳过未使用的目录项和被删除的目录项
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
	// 释放缓冲区
	//
	BufferSize = 0;	// 缓冲区大小设置为 0，表示释放全部缓冲区
	MmFreeVirtualMemory(&pBuffer, &BufferSize, MEM_RELEASE, TRUE);
	
	KeEnableInterrupts(IntState);	// 开中断
}


//////////////////////////////////////////////////////////////////////////
//
// 下面是和控制台命令 sd 相关的代码。
//

PRIVATE
VOID
ConsoleCmdScanDisk(
	IN HANDLE StdHandle
	)
/*++

功能描述：
	扫描软盘，并输出相关信息。控制台命令“sd”。

参数：
	StdHandle -- 标准输入、输出句柄。

返回值：
	无。

--*/
{
	BOOL IntState;
	PDEVICE_OBJECT FatDevice;
	PVCB pVcb;
	ULONG i, FreeClusterCount, UsedClusterCount;
	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 得到 FAT12 文件系统设备对象，然后得到卷控制块 VCB
	//
	FatDevice = (PDEVICE_OBJECT)ObpLookupObjectByName(IopDeviceObjectType, "A:");
	pVcb = (PVCB)FatDevice->DeviceExtension;
	
	//
	// 将卷控制块中缓存的 BIOS Parameter Block (BPB) ，以及卷控制块中的其它重要信息输出
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
	// 扫描 FAT 表，统计空闲簇的数量，并计算软盘空间的使用情况
	//
	FreeClusterCount = 0;
	for (i = 2; i < pVcb->NumberOfClusters + 2; i++) {
		if (0 == FatGetFatEntryValue(pVcb, i))
			FreeClusterCount++;
	}
	UsedClusterCount = pVcb->NumberOfClusters - FreeClusterCount;
	fprintf(StdHandle, "Free Cluster Count: %d (%d Byte)\n", FreeClusterCount, FreeClusterCount*pVcb->Bpb.SectorsPerCluster*pVcb->Bpb.BytesPerSector);
	fprintf(StdHandle, "Used Cluster Count: %d (%d Byte)\n", UsedClusterCount, UsedClusterCount*pVcb->Bpb.SectorsPerCluster*pVcb->Bpb.BytesPerSector);
	
	KeEnableInterrupts(IntState);	// 开中断
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
	LIST_ENTRY StateListEntry;			// 所在状态队列的链表项
	INT State;
	PSTR StartAddress;
	PTHREAD_START_ROUTINE StartAddr;
} THREADINFO, *PTHREADINFO;

#define MAXTHREADCOUNT 100
THREADINFO ThreadArray[MAXTHREADCOUNT];

PRIVATE VOID GetProcAndThread( )
/*++

功能描述：
	获取进程和线程信息。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PPROCESS pProc;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 遍历所有的进程对象
	//
	PNum = 0;
	TNum = 0;
	for(pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PNum < MAXPROCESSCOUNT;
		pListEntry = pListEntry->Next, PNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
			
		//
		// 对象体就是进程控制块
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
	// 遍历所有的线程对象
	//
	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead && TNum < MAXTHREADCOUNT;
		pListEntry = pListEntry->Next, TNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// 对象体就是线程控制块
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
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

PTHREAD PspThreadArr[MAXTHREADCOUNT];

PRIVATE VOID GetThreadLink( )
/*++

功能描述：
	获取线程链表。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PTHREAD pThread;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	

	for(TNum = 0, pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead && TNum < MAXTHREADCOUNT;
		pListEntry = pListEntry->Next,TNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// 对象体就是线程控制块
		//
		pThread = (PTHREAD)&pObjectHeader->Body;	
		
		PspThreadArr[TNum] = pThread;		
	}
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

PPROCESS PspProcessArr[MAXPROCESSCOUNT];

PRIVATE VOID GetProcessLink( )
/*++

功能描述：
	获取进程链表。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PPROCESS pProcess;
	PLIST_ENTRY pListEntry;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	for(PNum = 0, pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PNum < MAXPROCESSCOUNT;
		pListEntry = pListEntry->Next,PNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		//
		// 对象体就是线程控制块
		//
		pProcess = (PPROCESS)&pObjectHeader->Body;	
		
		PspProcessArr[PNum] = pProcess;		
	}
	
	KeEnableInterrupts(IntState);	// 开中断
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

功能描述：
	得到不同优先级的就绪队列。

参数：
	

返回值：
	无。

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
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
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
			// 获得对象头的指针
			//
			pThread = CONTAINING_RECORD(pListEntry, THREAD, StateListEntry);
			
			//
			// 对象体就是线程控制块
			//	
			PriorityReadyQueue[PriNum].PThreadArr[RTNum] = pThread;	
		}
		PriorityReadyQueue[PriNum].Count = RTNum; 
	}
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

#define MAXTHREADOBJCOUNT 100
INT ThreadObjIdArr[MAXTHREADOBJCOUNT];	// 线程对象Id数组
INT ThreadObjCount = 0;

PRIVATE VOID GetAllThreadObjId( )
/*++

功能描述：
	获取所有线程对象ID。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
		
	//
	// 遍历所有的线程对象
	//
	
	for(ThreadObjCount = 0, pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead && ThreadObjCount < MAXTHREADOBJCOUNT;
		pListEntry = pListEntry->Next, ThreadObjCount++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		ThreadObjIdArr[ThreadObjCount] = pObjectHeader->Id;
	}
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

INT MaxTid = 0;
PRIVATE VOID GetThreadObjMaxId( )
/*++

功能描述：
	获取所有线程对象ID。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PLIST_ENTRY pListEntry;	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
		
	//
	// 遍历所有的线程对象
	//	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		if(MaxTid < pObjectHeader->Id)
		{
			MaxTid = pObjectHeader->Id;
		}	
	}
	
	KeEnableInterrupts(IntState);	// 开中断
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
	
	PPROCESS Process;					// 线程所属进程指针
	LIST_ENTRY ThreadListEntry;			// 进程的线程链表项

	ULONG RemainderTicks;				// 剩余时间片，用于时间片轮转调度
	STATUS WaitStatus;					// 阻塞等待的结果状态
	KTIMER WaitTimer;					// 用于有限等待唤醒的计时器
	LIST_ENTRY StateListEntry;			// 所在状态队列的链表项
	LIST_ENTRY WaitListHead;			// 等待队列，所有等待线程结束的线程都在此队列等待。
	INT WaitThreadCount;
	INT WaitThreadIdArr[WAITTHREADMAXTHREADCOUNT];
	PTHREAD PWaitThreadArr[WAITTHREADMAXTHREADCOUNT];
	
	//
	// 为了结构简单，EOS没有对内核进行隔离保护，所有线程都运行在内核状态，所以目
	// 前线程没有用户空间的栈。
	//
	PVOID KernelStack;					// 线程位于内核空间的栈
	CONTEXT KernelContext;				// 线程执行在内核状态的上下文环境状态

	//
	// 线程必须在所属进程的地址空间中执行用户代码，但可在任何进程的地址空间中执行
	// 内核代码，因为内核代码位于所有进程地址空间共享的系统地址空间中。
	//
	PMMPAS AttachedPas;					// 线程在执行内核代码时绑定进程地址空间。

	PVOID Parameter;					// 传递给入口函数的参数

	ULONG LastError;					// 线程最近一次的错误码
	ULONG ExitCode;						// 线程的退出码
} THREADDETAILINFO;

THREADDETAILINFO SelThreadInfo;


PRIVATE VOID GetSelThreadDetail(INT ThreadObjId)
/*++

功能描述：
	获取选取线程的详细信息。

参数：
	线程对象Id

返回值：
	无。

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
	IntState = KeEnableInterrupts(FALSE);	// 关中断
		
	//
	// 遍历所有的线程对象，得到每个线程的信息
	//	
	for(pListEntry = PspThreadType->ObjectListHead.Next;
		pListEntry != &PspThreadType->ObjectListHead;
		pListEntry = pListEntry->Next) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		if(pObjectHeader->Id == ThreadObjId)
		{
			//
			// 对象体就是线程控制块
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
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

#define MAXPROCOBJCOUNT 100

INT ProcObjIdArr[MAXPROCOBJCOUNT];	// 进程对象Id数组
INT PObjCount = 0;

PRIVATE VOID GetAllProcObjId( )
/*++

功能描述：
	获取所有进程对象ID。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PLIST_ENTRY pListEntry;	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 遍历所有的进程对象，得到所有进程ID
	//
	
	for(PObjCount = 0, pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PObjCount < MAXPROCOBJCOUNT;
		pListEntry = pListEntry->Next, ++PObjCount) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		 ProcObjIdArr[PObjCount] = pObjectHeader->Id;	
	}	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

#define WAITPROCESSMAXTHREADCOUNT 20
//
// 进程对象结构体 (PCB)。
//
typedef struct _PROCESSDETAILINFO {
	INT Id;
	INT Priority;
	INT PrimaryThreadID;
	INT ThreadCount;
	PSTR ImageName;
	INT SystemProc;
	
	PMMPAS Pas;							// 进程地址空间 
	PHANDLE_TABLE ObjectTable;			// 进程的内核对象句柄表
	LIST_ENTRY ThreadListHead;			// 线程链表头
	PTHREAD PrimaryThread;				// 主线程指针
	LIST_ENTRY WaitListHead;			// 等待队列，所有等待进程结束的线程都在此队列等待。
	INT WaitThreadCount;
	INT WaitThreadIdArr[WAITPROCESSMAXTHREADCOUNT];
	PTHREAD PWaitThreadArr[WAITPROCESSMAXTHREADCOUNT];

	PSTR CmdLine;						// 命令行参数
	PVOID ImageBase;					// 可执行映像的加载基址
	PPROCESS_START_ROUTINE ImageEntry;	// 可执行映像的入口地址

	HANDLE StdInput;
	HANDLE StdOutput;
	HANDLE StdError;

	ULONG ExitCode;						// 进程退出码
} PROCESSDETAILINFO;

PROCESSDETAILINFO SelProcInfo;

PRIVATE VOID GetSelProcDetail(INT ProcObjId)
/*++

功能描述：
	获取已选择进程的详细信息。

参数：
	

返回值：
	无。

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
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 遍历所有的进程对象
	//
	
	for(pListEntry = PspProcessType->ObjectListHead.Next;
		pListEntry != &PspProcessType->ObjectListHead && PNum < MAXPROCESSCOUNT;
		pListEntry = pListEntry->Next, PNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		if(ProcObjId == pObjectHeader->Id)
		{
			//
			// 对象体就是进程控制块
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
	KeEnableInterrupts(IntState);	// 开中断
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

功能描述：
	统计输出由进程 ID 指定的进程的虚拟地址描述符的信息。

参数：
	ProcID。进程的 ID。

返回值：
	无。

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
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 由进程 ID 获得进程控制块
	//
	Status = ObRefObjectById(ProcID, PspProcessType, (PVOID*)&pProcCtrlBlock);
	
	//
	// 将进程控制块中 VAD 链表的指针保存下来，方便后面使用
	//
	pVadList = &pProcCtrlBlock->Pas->VadList;
	TotalVpnStart = pVadList->StartingVpn;
	TotalVpnEnd = pVadList->EndVpn;
	
	//
	// 遍历 VAD 链表，输出所有 VAD 的起始页框号，结束页框号和包含的虚拟页框数量
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
	// 统计虚拟页框总数、已分配的虚拟页框和未分配的虚拟页框
	//
	TotalVpnCount = pVadList->EndVpn - pVadList->StartingVpn + 1;
	FreeVpnCount = TotalVpnCount - AllocatedVpnCount;
	
	ObDerefObject(pProcCtrlBlock);
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;	
}

/*
定义了一个页表的缓冲区，大小为 8 * 1024 字节，可以用来存储页目录或页表中的数据。
其中只存储有效的页表项，从而可以大大加快获取页表的速度。
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
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	for(i = 0; i < PTE_PER_TABLE; i++)
	{
		pte = Page_Table_Base[i];
		
		/* 跳过无效的 PTE */
		if(!(pte & 1))
			continue;
			
		Page_Table_Buffer[Page_Entry_Count].Index = i;
		Page_Table_Buffer[Page_Entry_Count].Pte = pte;
		Page_Entry_Count++;
	}	
	
	KeEnableInterrupts(IntState);	// 开中断
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

功能描述：
	获取记录型信号量信息。

参数：
	

返回值：
	无。

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
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	for(pListEntry = PspSemaphoreType->ObjectListHead.Next;
		pListEntry != &PspSemaphoreType->ObjectListHead && SemNum < MAXSEMCOUNT;
		pListEntry = pListEntry->Next, SemNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
			
		//
		// 记录型信号量
		//
		pSem = (PSEMAPHORE)&pObjectHeader->Body;
		
		SEMAPHORE Sem;
		
		Sem.Count = pSem->Count;
		Sem.MaximumCount = pSem->MaximumCount;
		Sem.WaitListHead = pSem->WaitListHead;
		
		PTHREAD pThread;
		SemInfoArr[SemNum].Sem = Sem;
		
		// 等待线程
		PLIST_ENTRY pWListEntry;

		for(WaitSemThreadNum = 0,pWListEntry = pSem->WaitListHead.Next;
		pWListEntry != &pSem->WaitListHead && WaitSemThreadNum < MAXSEMWAITCOUNT;
		pWListEntry = pWListEntry->Next, WaitSemThreadNum++)
		{
			//
			// 获得对象头的指针
			//
			pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
			
			SemInfoArr[SemNum].WaitObj[WaitSemThreadNum] = ObGetObjectId(pThread);
		}
		SemInfoArr[SemNum].Count = WaitSemThreadNum;
	}
	
	KeEnableInterrupts(IntState);	// 开中断
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

功能描述：
	获取互斥信号量信息。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PMUTEX pMutex;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	for(MutexNum = 0, pListEntry = PspMutexType->ObjectListHead.Next;
		pListEntry != &PspMutexType->ObjectListHead && MutexNum < MAXMUTEXCOUNT;
		pListEntry = pListEntry->Next,MutexNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
			
		//
		// 记录型信号量
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
		// 等待线程
		PLIST_ENTRY pWListEntry;

		for(WaitMutexThreadNum = 0, pWListEntry = pMutex->WaitListHead.Next;
		pWListEntry != &pMutex->WaitListHead && WaitMutexThreadNum < MAXMUTEXWAITCOUNT;
		pWListEntry = pWListEntry->Next, WaitMutexThreadNum++)
		{
			//
			// 获得对象头的指针
			//
			pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
			
			MutexInfoArr[MutexNum].WaitObj[WaitMutexThreadNum] = ObGetObjectId(pThread);
		}
		MutexInfoArr[MutexNum].Count = WaitMutexThreadNum;
	}
	
	KeEnableInterrupts(IntState);	// 开中断
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
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
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
	
	KeEnableInterrupts(IntState);	// 开中断
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

功能描述：
	获取进程和线程信息。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	POBJECT_HEADER pObjectHeader;
	PDEVICE_OBJECT pDevice;
	PLIST_ENTRY pListEntry;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	//
	// 遍历所有的进程对象
	//
	DNum = 0;
	for(pListEntry = IopDeviceObjectType->ObjectListHead.Next;
		pListEntry != &IopDeviceObjectType->ObjectListHead && DNum < MAXDEVICECOUNT;
		pListEntry = pListEntry->Next, DNum++) {
		
		//
		// 获得对象头的指针
		//
		pObjectHeader = CONTAINING_RECORD(pListEntry, OBJECT_HEADER, TypeObjectListEntry);
		
		DeviceArr[DNum].Id = pObjectHeader->Id;
		DeviceArr[DNum].Name = pObjectHeader->Name;	
		//
		// 对象体就是进程控制块
		//
		pDevice = (PDEVICE_OBJECT)&pObjectHeader->Body;
		DeviceArr[DNum].PDevice = pDevice;
	}
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}


#endif
