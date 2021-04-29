/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: eos.h

描述: EOS 内核导出头文件，EOS 应用程序需包含此头文件。



*******************************************************************************/

#ifndef _EOS_
#define _EOS_

#ifdef __cplusplus
extern "C"
{
#endif

#include "eosdef.h"
#include "error.h"

EOSAPI
VOID
DebugBreak(
	VOID
	);

EOSAPI
BOOL
CloseHandle(
	IN HANDLE Handle
	);

EOSAPI 
PVOID
VirtualAlloc(
	IN PVOID Address,
	IN SIZE_T Size,
	IN ULONG AllocationType
	);

EOSAPI
BOOL
VirtualFree(
	IN PVOID Address,
	IN SIZE_T Size,
	IN ULONG FreeType
	);

EOSAPI
BOOL
CreateProcess(
	IN PCSTR ImageName,
	IN PCSTR CmdLine,
	IN ULONG CreateFlags,
	IN PSTARTUPINFO StartupInfo,
	OUT PPROCESS_INFORMATION ProcInfo
	);

EOSAPI
HANDLE
CreateThread(
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PULONG ThreadId OPTIONAL
	);

EOSAPI
HANDLE
OpenProcess(
	IN ULONG ProcessId
	);

EOSAPI
HANDLE
OpenThread(
	IN ULONG ThreadId
	);

EOSAPI
BOOL
TerminateProcess(
	IN HANDLE Handle,
	IN ULONG ExitCode
	);

EOSAPI
BOOL
TerminateThread(
	IN HANDLE Handle,
	IN ULONG ExitCode
	);

EOSAPI
VOID
ExitProcess(
	IN ULONG ExitCode
	);

EOSAPI
VOID
ExitThread(
	IN ULONG ExitCode
	);

EOSAPI
VOID
Sleep(
	IN ULONG Milliseconds
	);

EOSAPI
ULONG
GetLastError(
	VOID
	);

EOSAPI 
VOID
SetLastError(
	IN ULONG ErrCode
	);

EOSAPI
HANDLE
GetStdHandle(
	IN ULONG StdHandle
	);

EOSAPI
VOID
GetImageNameAndCmdLine(
	OUT PCHAR ImageNameBuffer,
	OUT PCHAR CmdLineBuffer
	);


EOSAPI
BOOL
GetExitCodeProcess(
	IN HANDLE ProcessHandle,
	OUT PULONG ExitCode
	);

EOSAPI
BOOL
GetExitCodeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG ExitCode
	);

EOSAPI 
HANDLE
CreateEvent(
	IN BOOL ManualReset,
	IN BOOL InitialState,
	IN PCSTR Name
	);

EOSAPI
BOOL
SetEvent(
	IN HANDLE Handle
	);

EOSAPI
BOOL
ResetEvent(
	IN HANDLE Handle
	);

EOSAPI
HANDLE
CreateMutex(
	IN BOOL InitialOwner,
	IN PCSTR Name
	);

EOSAPI
BOOL
ReleaseMutex(
	IN HANDLE Handle
	);

EOSAPI
HANDLE
CreateSemaphore(
	IN LONG InitialCount,
	IN LONG MaximumCount,
	IN PSTR Name
	);

EOSAPI
BOOL
ReleaseSemaphore(
	IN HANDLE Handle,
	IN LONG ReleaseCount,
	IN PLONG PreviousCount
	);

EOSAPI
ULONG
WaitForSingleObject(
	IN HANDLE Handle,
	IN ULONG Milliseconds
	);

EOSAPI
HANDLE
CreateFile(
	IN PCSTR FileName, 
	IN ULONG DesiredAccess,
	IN ULONG ShareMode,
	IN ULONG CreationDisposition,
	IN ULONG FlagsAndAttributes
	);

EOSAPI
BOOL
ReadFile(
	IN HANDLE Handle,
	OUT PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	);

EOSAPI
BOOL
WriteFile(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);

EOSAPI
BOOL
DeleteFile(
	IN PCSTR FileName
	);

EOSAPI
BOOL
GetFileTime(
	IN HANDLE FileHandle,
	OUT PFILETIME CreationTime,
	OUT PFILETIME LastAccessTime,
	OUT PFILETIME LastWriteTime
	);

EOSAPI
ULONG
GetFileSize(
	IN HANDLE FileHandle
	);

EOSAPI
ULONG
SetFilePointer(
	IN HANDLE FileHandle,
	IN LONG DistanceToMove,
	IN ULONG MoveMethod
	);

EOSAPI
ULONG
GetFileAttributes(
	IN PCSTR FileName
	);

EOSAPI
BOOL
SetFileAttributes(
	IN PCSTR FileName,
	IN ULONG FileAttributes
	);

EOSAPI
BOOL
CreateDirectory(
	IN PCSTR PathName
	);

EOSAPI
BOOL
RemoveDirectory(
	IN PCSTR PathName
	);

EOSAPI
BOOL
SetConsoleCursorPosition(
	IN HANDLE Handle,
	IN COORD CursorPosition
	);
	
EOSAPI
ULONG
GetCurrentThreadId(
	VOID
	);
	
EOSAPI
BOOL
SuspendThread(
	IN HANDLE hThread
	);
	
EOSAPI
BOOL
ResumeThread(
	IN HANDLE hThread
	);

#ifdef __cplusplus
}
#endif

#endif // _EOS_
