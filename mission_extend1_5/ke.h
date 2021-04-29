/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: ke.h

描述: 系统支撑模块对外接口的头文件，供内核其它模块使用。



*******************************************************************************/

#ifndef _KE_
#define _KE_

#include "rtl.h"

#ifdef _I386

//
// i386处理器上下文环境结构体。
// 注意：不要调整结构体中的变量顺序，内核汇编代码、内核调试器都已约定如此。
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
// I386程序状态字寄存器PSW(EFLAGS)的位掩码.
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
// 代码段和数据段选择子常量
//
extern CONST ULONG KeCodeSegmentSelector;
extern CONST ULONG KeDataSegmentSelector;

//
// i386 异常编号。
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
// 设备中断号。
//
#define PIC1_VECTOR		0x20
#define PIC2_VECTOR		0x28
#define	INT_TIMER		0x20	// 编号 32。8253 可编程定时计数器。
#define INT_KEYBOARD	0x21	// 编号 33。键盘。
#define INT_COM2		0x23	// 编号 35。串口 2。
#define INT_COM1		0x24	// 编号 36。串口 1。
#define INT_LPT2		0x25	// 编号 37。并口 2。
#define INT_FLOPPY		0x26	// 编号 38。软盘驱动器。
#define INT_LPT1		0x27	// 编号 39。并口 1。
#define	INT_CLOCK		0x28	// 编号 40。实时时钟。
#define INT_PS2			0x2C	// 编号 44。PS2 接口。
#define INT_FPU			0x2D	// 编号 45。浮点处理单元。
#define INT_HD			0x2E	// 编号 46。硬盘。

typedef VOID (*ISR)(VOID);

//
// 各设备对应的中断向量。
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
// 开关某一设备的中断。
//
VOID
KeEnableDeviceInterrupt(
	ULONG IntVector,
	BOOL Enable
	);

//
// 触发专用于线程调度的软中断，在中断返回时会执行线程调度。
// 注意：在中断中执行 KeThreadSchedule 会使所有外层嵌套中断丢失，直接返回线程环境。
//
#define KeThreadSchedule() __asm("int $48")

#endif

//
// ISR服务程序专用栈指针（指向栈底）。
//
extern PVOID KeIsrStack;

#ifdef	_DEBUG

extern volatile long Tick;						// 从开机开始算起的滴答数（10ms/滴答）
extern volatile ULONG StopKeyboard;

#endif

//
// 开/关外部中断并返回调用前的开关状态
//
BOOL
KeEnableInterrupts(
	IN BOOL EnableInt
	);

//
// 得到当前中断嵌套深度，返回 0 说明当前执行在非中断环境中。
//
ULONG
KeGetIntNesting(
	VOID
	);

//
// 计时器能够识别的时间范围。
//
#define KTIMER_MAXIMUM  0x7FFFFFFF
#define KTIMER_MINIMUM  0x0000000A


//
// 计时器回调函数的类型定义。
//
typedef VOID (*PKTIMER_ROUTINE)(ULONG_PTR);

//
// 计时器结构体。
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
// 初始化计时器。
//
VOID
KeInitializeTimer(
	IN PKTIMER Timer,
	IN ULONG Milliseconds,
	IN PKTIMER_ROUTINE TimerRoutine,
	IN ULONG_PTR Parameter
	);

//
// 注册计时器。
//
VOID
KeRegisterTimer(
	IN PKTIMER Timer
	);

//
// 注销计时器。
//
VOID
KeUnregisterTimer(
	IN PKTIMER Timer
	);

//
// 系统失败处理函数。
//
VOID
KeBugCheck(
	IN PCSTR Format,
	...
	);

#endif // _KE_
