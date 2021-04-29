/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: eosapi.c

描述: EOS 导出 API 的定义。



*******************************************************************************/

#include "mm.h"
#include "ob.h"
#include "ps.h"
#include "io.h"
#include "psp.h"

EOSAPI
VOID
DebugBreak(
	VOID
	)
{
	DbgBreakPoint();
}

EOSAPI
BOOL
CloseHandle(
	IN HANDLE Handle
	)
{
	STATUS Status;

	Status = ObCloseHandle(Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI 
PVOID
VirtualAlloc(
	IN PVOID Address,
	IN SIZE_T Size,
	IN ULONG AllocationType
	)
{
	STATUS Status;
	PVOID BaseAddress = Address;
	SIZE_T RegionSize = Size;

	Status = MmAllocateVirtualMemory( &BaseAddress,
									  &RegionSize,
									  AllocationType,
									  FALSE );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? BaseAddress : NULL;
}

EOSAPI
BOOL
VirtualFree(
	IN PVOID Address,
	IN SIZE_T Size,
	IN ULONG FreeType
	)
{
	STATUS Status;
	PVOID BaseAddress = Address;
	SIZE_T RegionSize = Size;

	Status = MmFreeVirtualMemory( &BaseAddress,
								  &RegionSize,
								  FreeType,
								  FALSE );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
CreateProcess(
	IN PCSTR ImageName,
	IN PCSTR CmdLine,
	IN ULONG CreateFlags,
	IN PSTARTUPINFO StartupInfo,
	OUT PPROCESS_INFORMATION ProcInfo
	)
{
	STATUS Status;

	Status = PsCreateProcess( ImageName,
							  CmdLine,
							  CreateFlags,
							  StartupInfo,
							  ProcInfo );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
HANDLE
CreateThread(
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PULONG ThreadId OPTIONAL
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = PsCreateThread( StackSize,
							 StartAddr,
							 ThreadParam,
							 CreateFlags,
							 &Handle,
							 ThreadId );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? Handle : NULL;
}

EOSAPI
HANDLE
OpenProcess(
	IN ULONG ProcessId
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = PsOpenProcess(ProcessId, &Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? Handle : NULL;
}

EOSAPI
HANDLE
OpenThread(
	IN ULONG ThreadId
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = PsOpenThread(ThreadId, &Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? Handle : NULL;
}

EOSAPI
BOOL
TerminateProcess(
	IN HANDLE Handle,
	IN ULONG ExitCode
	)
{
	STATUS Status;

	Status = PsTerminateProcess(Handle, ExitCode);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
TerminateThread(
	IN HANDLE Handle,
	IN ULONG ExitCode
	)
{
	STATUS Status;

	Status = PsTerminateThread(Handle, ExitCode);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
VOID
ExitProcess(
	IN ULONG ExitCode
	)
{
	PsExitProcess(ExitCode);
}

EOSAPI
VOID
ExitThread(
	IN ULONG ExitCode
	)
{
	PsExitThread(ExitCode);
}

EOSAPI
VOID
Sleep(
	IN ULONG Milliseconds
	)
{
	PsSleep(Milliseconds);

	PsSetLastError(TranslateStatusToError(STATUS_SUCCESS));
}

EOSAPI
ULONG
GetLastError(
	VOID
	)
{
	return PsGetLastError();
}

EOSAPI 
VOID
SetLastError(
	IN ULONG ErrCode
	)
{
	PsSetLastError(ErrCode);
}

EOSAPI
HANDLE
GetStdHandle(
	IN ULONG StdHandle
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = PsGetStdHandle(StdHandle, &Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? Handle : NULL;
}

EOSAPI
VOID
GetImageNameAndCmdLine(
	OUT PCHAR ImageNameBuffer,
	OUT PCHAR CmdLineBuffer
	)
{
	PsGetImageNameAndCmdLine(ImageNameBuffer, CmdLineBuffer);
}

EOSAPI
BOOL
GetExitCodeProcess(
	IN HANDLE ProcessHandle,
	OUT PULONG ExitCode
	)
{
	STATUS Status;

	Status = PsGetExitCodeProcess(ProcessHandle, ExitCode);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
GetExitCodeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG ExitCode
	)
{
	STATUS Status;

	Status = PsGetExitCodeThread(ThreadHandle, ExitCode);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI 
HANDLE
CreateEvent(
	IN BOOL ManualReset,
	IN BOOL InitialState,
	IN PCSTR Name
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = PsCreateEventObject(ManualReset, InitialState, Name, &Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? Handle : NULL;
}

EOSAPI
BOOL
SetEvent(
	IN HANDLE Handle
	)
{
	BOOL Status;

	Status = PsSetEventObject(Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
ResetEvent(
	IN HANDLE Handle
	)
{
	STATUS Status;
	
	Status = PsResetEventObject(Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
HANDLE
CreateMutex(
	IN BOOL InitialOwner,
	IN PCSTR Name
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = PsCreateMutexObject(InitialOwner, Name, &Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? Handle : NULL;
}

EOSAPI
BOOL
ReleaseMutex(
	IN HANDLE Handle
	)
{
	STATUS Status;
	
	Status = PsReleaseMutexObject(Handle);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
HANDLE
CreateSemaphore(
	IN LONG InitialCount,
	IN LONG MaximumCount,
	IN PSTR Name
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = PsCreateSemaphoreObject( InitialCount,
									  MaximumCount,
									  Name,
									  &Handle );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? Handle : NULL;
}

EOSAPI
BOOL
ReleaseSemaphore(
	IN HANDLE Handle,
	IN LONG ReleaseCount,
	IN PLONG PreviousCount
	)
{
	STATUS Status;

	Status = PsReleaseSemaphoreObject(Handle, ReleaseCount, PreviousCount);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
ULONG
WaitForSingleObject(
	IN HANDLE Handle,
	IN ULONG Milliseconds
	)
{
	STATUS Status;

	Status = ObWaitForObject(Handle, Milliseconds);

	PsSetLastError(TranslateStatusToError(Status));

	return STATUS_SUCCESS == Status ? 0 : (STATUS_TIMEOUT == Status ? WAIT_TIMEOUT : -1);
}

EOSAPI
HANDLE
CreateFile(
	IN PCSTR FileName, 
	IN ULONG DesiredAccess,
	IN ULONG ShareMode,
	IN ULONG CreationDisposition,
	IN ULONG FlagsAndAttributes
	)
{
	STATUS Status;
	HANDLE Handle;

	Status = IoCreateFile( (PSTR)FileName,
						   DesiredAccess,
						   ShareMode,
						   CreationDisposition,
						   FlagsAndAttributes,
						   &Handle );

	PsSetLastError(TranslateStatusToError(Status));
	
	return EOS_SUCCESS(Status) ? Handle : INVALID_HANDLE_VALUE;
}

EOSAPI
BOOL
ReadFile(
	IN HANDLE Handle,
	OUT PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	)
{
	STATUS Status;

	Status = ObRead( Handle,
					 Buffer,
					 NumberOfBytesToRead,
					 NumberOfBytesRead );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
WriteFile(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	)
{
	STATUS Status;

	Status = ObWrite( Handle,
					  Buffer,
					  NumberOfBytesToWrite,
					  NumberOfBytesWritten );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
DeleteFile(
	IN PCSTR FileName
	)
{
	STATUS Status;

	Status = IoDeleteFile((PSTR)FileName);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
GetFileTime(
	IN HANDLE FileHandle,
	OUT PFILETIME CreationTime,
	OUT PFILETIME LastAccessTime,
	OUT PFILETIME LastWriteTime
	)
{
	STATUS Status;

	Status = IoGetFileTime( FileHandle,
							CreationTime,
							LastAccessTime,
							LastWriteTime );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
ULONG
GetFileSize(
	IN HANDLE FileHandle
	)
{
	STATUS Status;
	ULONG FileSize;

	Status = IoGetFileSize(FileHandle, &FileSize);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? FileSize : -1;
}

EOSAPI
ULONG
SetFilePointer(
	IN HANDLE FileHandle,
	IN LONG DistanceToMove,
	IN ULONG MoveMethod
	)
{
	STATUS Status;
	ULONG NewFilePointer;

	Status = IoSetFilePointer( FileHandle,
							   DistanceToMove,
							   MoveMethod,
							   &NewFilePointer );

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? NewFilePointer : -1;
}

EOSAPI
ULONG
GetFileAttributes(
	IN PCSTR FileName
	)
{
	STATUS Status;
	ULONG FileAttributes;

	Status = IoGetFileAttributes((PSTR)FileName, &FileAttributes);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status) ? FileAttributes : -1;
}

EOSAPI
BOOL
SetFileAttributes(
	IN PCSTR FileName,
	IN ULONG FileAttributes
	)
{
	STATUS Status;
	
	Status = IoSetFileAttributes((PSTR)FileName, FileAttributes);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
CreateDirectory(
	IN PCSTR PathName
	)
{
	STATUS Status;

	Status = IoCreateDirectory((PSTR)PathName);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
RemoveDirectory(
	IN PCSTR PathName
	)
{
	STATUS Status;

	Status = IoRemoveDirectory((PSTR)PathName);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
BOOL
SetConsoleCursorPosition(
	IN HANDLE Handle,
	IN COORD CursorPosition
	)
{
	STATUS Status;

	Status = IoSetConsoleCursorPosition(Handle, CursorPosition);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}

EOSAPI
ULONG
GetCurrentThreadId(
	VOID
	)
{
	return ObGetObjectId(PspCurrentThread);
}

EOSAPI
BOOL
SuspendThread(
	IN HANDLE hThread
	)
{
	STATUS Status;

	Status = PsSuspendThread(hThread);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}
	
EOSAPI
BOOL
ResumeThread(
	IN HANDLE hThread
	)
{
	STATUS Status;

	Status = PsResumThread(hThread);

	PsSetLastError(TranslateStatusToError(Status));

	return EOS_SUCCESS(Status);
}
