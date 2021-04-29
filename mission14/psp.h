/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: psp.h

����: ���̹���ģ����ڲ�ͷ�ļ���



*******************************************************************************/

#ifndef _PSP_
#define _PSP_

#include "ke.h"
#include "mm.h"
#include "ob.h"
#include "ps.h"
#include "rtl.h"

//
// ���̶����̶߳���ָ�롣
//
typedef struct _PROCESS *PPROCESS;
typedef struct _THREAD *PTHREAD;

#ifdef _DEBUG

extern void record_task_state(long pid, long new_state, long Tick, const char* fun_name, int line_num);
#define RECORD_TASK_STATE(pid, state, jiffies) record_task_state(pid, state, Tick, __FUNCTION__, __LINE__);

extern volatile INT ThreadSeq;
extern INT MaxTid;

#endif

//
// ���̶���ṹ�� (PCB)��
//
typedef struct _PROCESS {
	BOOLEAN System;						// �Ƿ�ϵͳ����
	UCHAR Priority;						// ���̵����ȼ�
	PMMPAS Pas;							// ���̵�ַ�ռ� 
	PHANDLE_TABLE ObjectTable;			// ���̵��ں˶�������
	LIST_ENTRY ThreadListHead;			// �߳�����ͷ
	PTHREAD PrimaryThread;				// ���߳�ָ��
	LIST_ENTRY WaitListHead;			// �ȴ����У����еȴ����̽������̶߳��ڴ˶��еȴ���

	PSTR ImageName;						// ������ӳ���ļ�����
	PSTR CmdLine;						// �����в���
	PVOID ImageBase;					// ��ִ��ӳ��ļ��ػ�ַ
	PPROCESS_START_ROUTINE ImageEntry;	// ��ִ��ӳ�����ڵ�ַ

	HANDLE StdInput;
	HANDLE StdOutput;
	HANDLE StdError;

	ULONG ExitCode;						// �����˳���
} PROCESS;

//
// �̶߳���ṹ�� (TCB)��
//
typedef	struct _THREAD {
	PPROCESS Process;					// �߳���������ָ��
	LIST_ENTRY ThreadListEntry;			// ���̵��߳�������
	UCHAR Priority;						// �߳����ȼ�
	UCHAR State;						// �̵߳�ǰ״̬
	ULONG RemainderTicks;				// ʣ��ʱ��Ƭ������ʱ��Ƭ��ת����
	STATUS WaitStatus;					// �����ȴ��Ľ��״̬
	KTIMER WaitTimer;					// �������޵ȴ����ѵļ�ʱ��
	LIST_ENTRY StateListEntry;			// ����״̬���е�������
	LIST_ENTRY WaitListHead;			// �ȴ����У����еȴ��߳̽������̶߳��ڴ˶��еȴ���

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

	PTHREAD_START_ROUTINE StartAddr;	// �̵߳���ں�����ַ
	PVOID Parameter;					// ���ݸ���ں����Ĳ���

	ULONG LastError;					// �߳����һ�εĴ�����
	ULONG ExitCode;						// �̵߳��˳���
} THREAD;

//
// �̵߳�����״̬������ (Ready)������ (Running)���ȴ� (Waiting) �ͽ��� (Terminated)��
// ע�⣺Zero �����̵߳���Ч״̬����һ������״̬�����߳�״̬ת�����м�̬��
//
typedef enum _THREAD_STATE {
	Zero,		// 0
	Ready,		// 1
	Running,	// 2
	Waiting,	// 3
	Terminated	// 4
} THREAD_STATE;

#define TS_CREATE 	0	// ����
#define TS_READY	1	// ����̬
#define TS_RUNNING	2	// ����̬
#define TS_WAIT		3	// ����̬
#define TS_STOPPED	4	// ����

//
// ����ʱ��Ƭ��ת���ȵ�ʱ��Ƭ��С����ʱ��Ƭ������ʱ�ӵδ�����
//
#define TICKS_OF_TIME_SLICE		6

//
// �߳��ں�ջ��С--2 ��ҳ�档
//
#define KERNEL_STACK_SIZE	(PAGE_SIZE * 2)

//
// ���̡��̶߳������͡� 
//
extern POBJECT_TYPE PspProcessType;
extern POBJECT_TYPE PspThreadType;
extern POBJECT_TYPE PspSemaphoreType;
extern POBJECT_TYPE PspMutexType;

extern LIST_ENTRY PspReadyListHeads[32];

//
// ϵͳ����ָ�롣
//
extern PPROCESS PspSystemProcess;

//
// ��ǰ�����߳�ָ�롣
//
extern volatile PTHREAD PspCurrentThread;

//
// ��ǰ���̵�ָ��
//
#define PspCurrentProcess (PspCurrentThread->Process)


//
// ʹ Zero �� Running �߳�ת�� Ready ״̬��
//
VOID
PspReadyThread(
	IN PTHREAD Thread
	);

//
// ʹ Ready �߳�ת�� Zero ״̬��
//
VOID
PspUnreadyThread(
	IN PTHREAD Thread
	);

//
// ��ǰ�̰߳��� FCFS �����ȴ���ָ���ĵȴ������У�����ȴ�ʱ�䳬�� Milliseconds ����
// δ�����ѣ��߳̽����Զ����Ѳ����� STATUS_TIMEOUT��
//
STATUS
PspWait(
   IN PLIST_ENTRY WaitListHead,
   IN ULONG Milliseconds
   );

//
// ʹ Waiting �߳�ת�� Zero ״̬��
//
VOID
PspUnwaitThread(
	IN PTHREAD Thread
	);

//
// ���� FCFS ��ԭ����ָ���ȴ������е�һ���̣߳��������߳̽������״̬׼�����С�
// WaitStatus ����Ϊ�������߳��ڵ��� PspWait() ʱ�ķ���ֵ��
//
PTHREAD
PspWakeThread(
	IN PLIST_ENTRY WaitListHead,
	IN STATUS WaitStatus
	);

//
// ִ���̵߳��ȡ�
//
VOID
PspThreadSchedule(
	VOID
	);

//
// ��ǰ�̸߳��ŵ�ָ�����̵ĵ�ַ�ռ���ִ�С�
//
VOID
PspThreadAttachProcess(
	IN PPROCESS Process
	);

//
// ����Ӧ�ó���Ŀ�ִ��ӳ��
//
STATUS
PspLoadProcessImage(
	IN PPROCESS Process,
	IN PSTR ImageName,
	OUT PVOID *ImageBase,
	OUT PVOID *ImageEntry
	);

//
// �û����̵�����������
//
ULONG
PspProcessStartup(
	PVOID Parameter
	);

//
// �������̻�����
//
STATUS
PspCreateProcessEnvironment(
	IN UCHAR Priority,
	IN PCSTR ImageName,
	IN PCSTR CmdLine,
	OUT PPROCESS *Process
	);

//
// ɾ�����̻�����
//
VOID
PspDeleteProcessEnvironment(
	IN PPROCESS Process
	);

//
// ��ָ���Ľ����ڴ���һ���̡߳�
//
STATUS
PspCreateThread(
	IN PPROCESS Process,
	IN SIZE_T StackSize,
	IN PTHREAD_START_ROUTINE StartAddr,
	IN PVOID ThreadParam,
	IN ULONG CreateFlags,
	OUT PTHREAD *Thread
	);

//
// ��ʼ���̵߳������Ļ�����
//
VOID
PspInitializeThreadContext(
	IN PTHREAD Thread
	);

//
// ����ָ�����̵����С�
//
VOID
PspTerminateProcess(
	IN PPROCESS Process,
	IN ULONG ExitCode
	);

//
// ����ָ���̵߳����С�
//
VOID
PspTerminateThread(
	IN PTHREAD Thread,
	IN ULONG ExitCode,
	IN BOOL IsTerminatingProcess
	);

#endif // _PSP_
