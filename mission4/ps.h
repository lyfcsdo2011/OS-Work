/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: ps.h

����: ���̹���ģ��Ķ���ӿ�ͷ�ļ���



*******************************************************************************/

#ifndef _PS_
#define _PS_

#include "eosdef.h"
#include "rtl.h"

//
// �¼��ṹ��
//
typedef struct _EVENT {
	BOOL IsManual;				// �Ƿ��ֶ������¼�
	BOOL IsSignaled;			// �Ƿ��� Signaled ״̬
	LIST_ENTRY WaitListHead;	// �ȴ�����
}EVENT, *PEVENT;

//
// �����ź����ṹ��
//
typedef struct _MUTEX {
	PVOID OwnerThread;			// ��ǰӵ�� Mutex ���߳�ָ��
	ULONG RecursionCount;		// �ݹ�ӵ�� Mutex �ļ�����
	LIST_ENTRY WaitListHead;	// �ȴ�����
}MUTEX, *PMUTEX;

//
// ��¼���ź����ṹ��
//
typedef struct _SEMAPHORE {
	LONG Count;					// �ź���������ֵ
	LONG MaximumCount;			// �������ֵ
	LIST_ENTRY WaitListHead;	// �ȴ�����
}SEMAPHORE, *PSEMAPHORE;

//
// ���̻����ź����ṹ��
//
typedef struct _DISKMUTEX {
	PVOID OwnerThread;			// ��ǰӵ�� Mutex ���߳�ָ��
	LIST_ENTRY WaitListHead;	// �ȴ�����
}DISKMUTEX, *PDISKMUTEX;

//
// ��ǰ�̺߳ͽ��̵�α������塣
//
#define CURRENT_PROCESS_HANDLE	((HANDLE)-1)
#define CURRENT_THREAD_HANDLE	((HANDLE)-2)

//
// ��ʼ������ģ���һ����
//
VOID
PsInitializeSystem1(
	VOID
	);

//
// ��ʼ������ģ��ڶ�����
//
VOID
PsInitializeSystem2(
	VOID
	);

//
// ������Keģ����ǲ�������߳��쳣��
//
VOID
PsHandleException(
	ULONG ExceptionNumber,
	ULONG ErrorCode,
	PVOID Context
	);

//
// �õ���ǰ���̶��󣬲����������á�
//
PVOID
PsGetCurrentProcessObject(
	VOID
	);

//
// �õ���ǰ�̶߳��󣬲����������á�
//
PVOID
PsGetCurrentThreadObject(
	VOID
	);

//
// �õ���ǰ���̵ı�׼������������
//
STATUS
PsGetStdHandle(
	IN ULONG StdHandle,
	OUT PHANDLE Handle
	);

//
// �õ���ǰ���̵�ӳ���ļ����ƺ������в�����
//
VOID
PsGetImageNameAndCmdLine(
	OUT PCHAR ImageNameBuffer,
	OUT PCHAR CmdLineBuffer
	);

//
// �õ���ǰ�̵߳Ĵ����롣
//
ULONG
PsGetLastError(
	VOID
	);

//
// ���õ�ǰ�̵߳Ĵ����롣
//
VOID
PsSetLastError(
	ULONG ErrCode
	);

//
// �õ�ָ�����̵��˳��롣
//
STATUS
PsGetExitCodeProcess(
	IN HANDLE ProcessHandle,
	OUT PULONG ExitCode
	);

//
// �õ�ָ���̵߳��˳��롣
//
STATUS
PsGetExitCodeThread(
	IN HANDLE ThreadHandle,
	OUT PULONG ExitCode
	);

//
// �õ�ָ�����̵��ں˶�������
//
STATUS
PsGetObjectTable(
	IN HANDLE ProcessHandle,
	OUT PVOID *ObjectTable
	);

//
// ����ϵͳ���̡�
//
VOID
PsCreateSystemProcess(
	IN PTHREAD_START_ROUTINE StartAddr
	);

//
// �����û����̡�
//
STATUS
PsCreateProcess(
	IN PCSTR ImageName,
	IN PCSTR CmdLine,
	IN ULONG CreateFlags,
	IN PSTARTUPINFO StartupInfo,
	OUT PPROCESS_INFORMATION ProcInfo
	);

//
// �����̡߳�
//
STATUS
PsCreateThread(
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PHANDLE ThreadHandle,
	OUT PULONG ThreadId OPTIONAL
	);

//
// ͨ������Id�򿪽��̶���
//
STATUS
PsOpenProcess(
	IN ULONG ProcessId,
	OUT PHANDLE ProcessHandle
	);

//
// ͨ���߳�Id���̶߳���
//
STATUS
PsOpenThread(
	IN ULONG ThreadId,
	OUT PHANDLE ThreadHandle
	);

//
// ����ָ�����̡�
//
STATUS
PsTerminateProcess(
	IN HANDLE Handle,
	IN ULONG ExitCode
	);

//
// ����ָ���̡߳�
//
STATUS
PsTerminateThread(
	IN HANDLE Handle,
	IN ULONG ExitCode
	);

//
// �˳���ǰ���̡�
//
VOID
PsExitProcess(
	IN ULONG ExitCode
	);

//
// �˳���ǰ�̡߳�
//
VOID
PsExitThread(
	IN ULONG ExitCode
	);

//
// �����̵߳����ȼ���
//
STATUS
PsSetThreadPriority(
	IN HANDLE Handle,
	IN UCHAR Priority
	);

//
// ��ȡ�̵߳����ȼ���
//
STATUS
PsGetThreadPriority(
	IN HANDLE Handle,
	OUT PUCHAR Priority
	);

//
// ��ǰ�߳�˯�ߵȴ�Ƭ�̡�
//
VOID
PsSleep(
	IN ULONG Milliseconds
	);

//
// ��ʼ���¼��ṹ�塣
//
VOID
PsInitializeEvent(
	IN PEVENT Event,
	IN BOOL ManualReset,
	IN BOOL InitialState
	);

//
// �ȴ��¼���
//
STATUS
PsWaitForEvent(
	IN PEVENT Event,
	IN ULONG Milliseconds
	);

//
// ֪ͨ�¼���ʹ�¼���Ϊ Signaled ״̬��
//
VOID
PsSetEvent(
	IN PEVENT Event
	);

//
// ��λ�¼���ʹ�¼���Ϊ Nonsignaled ״̬��
//
VOID
PsResetEvent(
	IN PEVENT Event
	);

//
// �����¼�����
//
STATUS
PsCreateEventObject(
	IN BOOL ManualReset,
	IN BOOL InitialState,
	IN PCSTR EventName,
	OUT PHANDLE EventHandle
	);

//
// ֪ͨ�¼�����ʹ�¼������Ϊ Signaled ״̬��
//
STATUS
PsSetEventObject(
	HANDLE Handle
	);

//
// ��λ�¼�����ʹ�¼������Ϊ Nonsignaled ״̬��
//
STATUS
PsResetEventObject(
	HANDLE Handle
	);

//
// ��ʼ�������ź����ṹ�塣
//
VOID
PsInitializeMutex(
	IN PMUTEX Mutex,
	IN BOOL InitialOwner
	);

//
// �ȴ������ź�����
//
STATUS
PsWaitForMutex(
	IN PMUTEX Mutex,
	IN ULONG Milliseconds
	);

//
// �ͷŻ����ź�����
//
STATUS
PsReleaseMutex(
	IN PMUTEX Mutex
	);

//
// ���������ź�������
//
STATUS
PsCreateMutexObject(
	IN BOOL InitialOwner,
	IN PCSTR MutexName,
	OUT PHANDLE MutexHandle
	);

//
// �ͷŻ����ź�������
//
STATUS
PsReleaseMutexObject(
	IN HANDLE Handle
	);

//
// ��ʼ�����̻����ź����ṹ�塣
//
VOID
PsInitializeDiskMutex(
	IN PDISKMUTEX DiskMutex
	);

//
// �ȴ����̻����ź�����
//
STATUS
PsWaitForDiskMutex(
	IN PDISKMUTEX DiskMutex,
	IN ULONG SectorNumber
	);

//
// �ͷŴ��̻����ź�����
//
STATUS
PsReleaseDiskMutex(
	IN PDISKMUTEX DiskMutex
	);

#endif // _PS_
