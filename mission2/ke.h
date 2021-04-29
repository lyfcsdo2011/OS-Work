/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: ke.h

����: ϵͳ֧��ģ�����ӿڵ�ͷ�ļ������ں�����ģ��ʹ�á�



*******************************************************************************/

#ifndef _KE_
#define _KE_

#include "rtl.h"

#ifdef _I386

//
// i386�����������Ļ����ṹ�塣
// ע�⣺��Ҫ�����ṹ���еı���˳���ں˻����롢�ں˵���������Լ����ˡ�
//
typedef struct _CONTEXT
{
	ULONG Eax;
	ULONG Ecx;
	ULONG Edx;
	ULONG Ebx;
	ULONG Esp;
	ULONG Ebp;
	ULONG Esi;
	ULONG Edi;
	ULONG Eip;
	ULONG EFlag;
	ULONG SegCs;
	ULONG SegSs;
	ULONG SegDs;
	ULONG SegEs;
	ULONG SegFs;
	ULONG SegGs;
}CONTEXT, *PCONTEXT;

//
// I386����״̬�ּĴ���PSW(EFLAGS)��λ����.
//
#define PSW_MASK_CF		(1<<0)
#define PSW_MASK_PF		(1<<2)
#define PSW_MASK_AF		(1<<4)
#define PSW_MASK_ZF		(1<<6)
#define PSW_MASK_SF		(1<<7)
#define PSW_MASK_TF		(1<<8)
#define PSW_MASK_IF		(1<<9)
#define PSW_MASK_DF		(1<<10)
#define PSW_MASK_OF		(1<<11)

//
// ����κ����ݶ�ѡ���ӳ���
//
extern CONST ULONG KeCodeSegmentSelector;
extern CONST ULONG KeDataSegmentSelector;

//
// i386 �쳣��š�
//
#define EXP_DIVIDE_ERROR				0x00	// Fault
#define EXP_DEBUG						0x01	// Fault / Trap
#define	EXP_BREAKPOINT					0x03	// Trap
#define EXP_OVERFLOW					0x04	// Trap
#define EXP_BOUNDS_CHECK				0x05	// Fault
#define EXP_BAD_CODE					0x06	// Fault
#define EXP_NO_CORPROCESSOR				0x07	// Fault
#define EXP_DOUBLE_FAULT				0x08	// Abort, ErrorCode
#define EXP_CORPROCESSOR_OVERRUN		0x09	// Abort
#define	EXP_INVALID_TSS					0x0A	// Fault, ErrorCode
#define EXP_SEGMENT_NOT_PRESENT			0x0B	// Fault, ErrorCode
#define EXP_STACK_FAULT					0x0C	// Fault, ErrorCode
#define EXP_GENERAL_PROTECTION_FAULT	0x0D	// Fault, ErrorCode
#define EXP_PAGE_FAULT					0x0E	// Fault, ErrorCode
#define EXP_CORPROCESSOR_ERROR			0x10	// Fault

//
// �豸�жϺš�
//
#define PIC1_VECTOR		0x20
#define PIC2_VECTOR		0x28
#define	INT_TIMER		0x20	// ��� 32��8253 �ɱ�̶�ʱ��������
#define INT_KEYBOARD	0x21	// ��� 33�����̡�
#define INT_COM2		0x23	// ��� 35������ 2��
#define INT_COM1		0x24	// ��� 36������ 1��
#define INT_LPT2		0x25	// ��� 37������ 2��
#define INT_FLOPPY		0x26	// ��� 38��������������
#define INT_LPT1		0x27	// ��� 39������ 1��
#define	INT_CLOCK		0x28	// ��� 40��ʵʱʱ�ӡ�
#define INT_PS2			0x2C	// ��� 44��PS2 �ӿڡ�
#define INT_FPU			0x2D	// ��� 45�����㴦��Ԫ��
#define INT_HD			0x2E	// ��� 46��Ӳ�̡�

typedef VOID (*ISR)(VOID);

//
// ���豸��Ӧ���ж�������
//
extern ISR KeIsrKeyBoard;
extern ISR KeIsrCom2;
extern ISR KeIsrCom1;
extern ISR KeIsrLPT2;
extern ISR KeIsrFloppy;
extern ISR KeIsrLPT1;
extern ISR KeIsrClock;
extern ISR KeIsrPS2;
extern ISR KeIsrFPU;
extern ISR KeIsrHD;

//
// ����ĳһ�豸���жϡ�
//
VOID
KeEnableDeviceInterrupt(
	ULONG IntVector,
	BOOL Enable
	);

//
// ����ר�����̵߳��ȵ����жϣ����жϷ���ʱ��ִ���̵߳��ȡ�
// ע�⣺���ж���ִ�� KeThreadSchedule ��ʹ�������Ƕ���ж϶�ʧ��ֱ�ӷ����̻߳�����
//
#define KeThreadSchedule() __asm("int $48")

#endif

//
// ISR�������ר��ջָ�루ָ��ջ�ף���
//
extern PVOID KeIsrStack;

#ifdef	_DEBUG

extern volatile long Tick;						// �ӿ�����ʼ����ĵδ�����10ms/�δ�
extern volatile ULONG StopKeyboard;

#endif

//
// ��/���ⲿ�жϲ����ص���ǰ�Ŀ���״̬
//
BOOL
KeEnableInterrupts(
	IN BOOL EnableInt
	);

//
// �õ���ǰ�ж�Ƕ����ȣ����� 0 ˵����ǰִ���ڷ��жϻ����С�
//
ULONG
KeGetIntNesting(
	VOID
	);

//
// ��ʱ���ܹ�ʶ���ʱ�䷶Χ��
//
#define KTIMER_MAXIMUM  0x7FFFFFFF
#define KTIMER_MINIMUM  0x0000000A


//
// ��ʱ���ص����������Ͷ��塣
//
typedef VOID (*PKTIMER_ROUTINE)(ULONG_PTR);

//
// ��ʱ���ṹ�塣
//
typedef struct _KTIMER
{
	ULONG IntervalTicks;
	ULONG ElapsedTicks;
	PKTIMER_ROUTINE TimerRoutine;
	ULONG_PTR Parameter;
	LIST_ENTRY TimerListEntry;
}KTIMER, *PKTIMER;

//
// ��ʼ����ʱ����
//
VOID
KeInitializeTimer(
	IN PKTIMER Timer,
	IN ULONG Milliseconds,
	IN PKTIMER_ROUTINE TimerRoutine,
	IN ULONG_PTR Parameter
	);

//
// ע���ʱ����
//
VOID
KeRegisterTimer(
	IN PKTIMER Timer
	);

//
// ע����ʱ����
//
VOID
KeUnregisterTimer(
	IN PKTIMER Timer
	);

//
// ϵͳʧ�ܴ�������
//
VOID
KeBugCheck(
	IN PCSTR Format,
	...
	);

#endif // _KE_
