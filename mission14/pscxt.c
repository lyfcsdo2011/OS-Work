/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: pscxt.c

描述: 线程上下文环境的初始化，依赖具体的硬件。



*******************************************************************************/

#include "psp.h"

//
// 线程启动函数，所有线程都从这里开始执行。
//
VOID
PspThreadStartup(
	VOID
	);

VOID
PspInitializeThreadContext(
	IN PTHREAD Thread
	)
/*++

功能描述：
	初始化线程的上下文环境（和硬件平台相关）。

参数：
	Thread -- 线程控制块指针。

返回值：
	无。

--*/
{
	PCONTEXT cxt;

	cxt = &Thread->KernelContext;
	
	//
	// 初始化所有通用寄存器为0
	//
	cxt->Eax = 0;
	cxt->Ebx = 0;
	cxt->Ecx = 0;
	cxt->Edx = 0;
	cxt->Edi = 0;
	cxt->Esi = 0;

	//
	// 设置段寄存器
	//
	cxt->SegCs = KeCodeSegmentSelector;
	cxt->SegDs = KeDataSegmentSelector;
	cxt->SegEs = KeDataSegmentSelector;
	cxt->SegFs = KeDataSegmentSelector;
	cxt->SegGs = KeDataSegmentSelector;
	cxt->SegSs = KeDataSegmentSelector;

	//
	// 所有线程都从线程启动函数 PspThreadStartup 开始执行。
	//
	cxt->Eip = (ULONG)PspThreadStartup;

	//
	// 初始化内核模式栈。
	// x86 CPU 堆栈满递减，堆栈的底部在堆栈空间的高地址端。
	//
	cxt->Esp = (ULONG_PTR)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(ULONG); 
	*(PULONG)cxt->Esp = 0;	// PspThreadStartup 返回地址为 0。
	cxt->Ebp = 0;			// PspThreadStartup 没有外层调用帧。

	//
	// 初始化程序状态字，仅仅设置了一位：允许中断。
	//
	cxt->EFlag = PSW_MASK_IF;
}
