/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: dispatch.c

描述: Inerrupt dispatch module for the KE subcomponent of EOS



*******************************************************************************/

#include "ki.h"
#include "ps.h"

//
// 向 8259 发送 EOI 命令
//
VOID
Ki8259EOI(
	VOID
	);

//
// 外设中断处理函数指针。
//
ISR KeIsrClock = NULL;
ISR KeIsrKeyBoard = NULL;
ISR KeIsrCom1 = NULL;
ISR KeIsrCom2 = NULL;
ISR KeIsrLPT1 = NULL;
ISR KeIsrLPT2 = NULL;
ISR KeIsrFloppy = NULL;
ISR KeIsrHD = NULL;
ISR KeIsrFPU = NULL;
ISR KeIsrPS2 = NULL;

VOID
KiDispatchInterrupt(
	ULONG IntNumber
	)
/*++

功能描述：

	派遣外设中断给适当的中断处理程序。

参数：

	IntNumber - 当前中断向量号。

返回值：

	无。

--*/
{
	//
	// 根据向量号调用相应的中断处理程序。如果中断处理程序指针为NULL，则忽略之。
	//
	switch (IntNumber)
	{
	case INT_TIMER:

		KiIsrTimer();

		break;
	case INT_KEYBOARD:

		if (NULL != KeIsrKeyBoard)
			KeIsrKeyBoard();

		break;
	case INT_COM2:

		if (NULL != KeIsrCom2)
			KeIsrCom2();

		break;
	case INT_COM1:

		if (NULL != KeIsrCom1)
			KeIsrCom1();

		break;
	case INT_LPT2:

		if (NULL != KeIsrLPT2)
			KeIsrLPT2();

		break;
	case INT_FLOPPY:

		if (NULL != KeIsrFloppy)
			KeIsrFloppy();

		break;
	case INT_LPT1:

		if (NULL != KeIsrLPT1)
			KeIsrLPT1();

		break;
	case INT_CLOCK:

		if (NULL != KeIsrClock)
			KeIsrClock();

		break;
	case INT_PS2:

		if (NULL != KeIsrPS2)
			KeIsrPS2();

		break;
	case INT_FPU:

		if (NULL != KeIsrFPU)
			KeIsrFPU();

		break;
	case INT_HD:

		if (NULL != KeIsrHD)
			KeIsrHD();

		break;
	default:

		ASSERT(FALSE);

		break;
	}

	//
	// 发送 EOI 命令给可编程中断控制器 8259。
	// 警告：
	//		不要在此行代码处插入断点。即使调试到此行代码，也不要使用“逐过程”
	//		单步调试，此操作会产生不可预测的调试结果。建议使用“继续调试”功
	//		能继续调试。
	Ki8259EOI();
}



BOOL
KiDispatchException(
	ULONG ExceptionNumber,
	ULONG ErrorCode,
	PCONTEXT Context
	)
/*++

功能描述：

	派遣异常给适当的异常处理程序。

参数：

	ExceptionNumber - 异常向量号。
	ErrorCode - 异常错误码。
	Context - 发生异常的CPU上下文环境。

返回值：

	FALSE：从异常返回线程时不要执行线程调度。
	TRUE：从异常返回线程时需要执行线程调度。

警告：
	
	在此函数中的任何位置插入断点都会造成调试失败。
--*/
{
	//
	// 非调试情况下，如果是中断服务程序产生了异常则直接蓝屏，否则交由进程管理器
	// 处理线程产生的异常。
	// 注意：中断深度大于0则说明当前执行在中断环境中。由于异常处理也是执行在中断
	// 环境中的，所以如果是中断服务程序产生异常则会嵌套进入异常处理中断服务程序。
	// 这时得到的中断深度应大于1。
	//
	if (KeGetIntNesting() > 1) {
		KeBugCheck( "Interrupt service routine error!\n\
					Exception number is %d.\n\
					Error code is %d.\n\
					Register value:\n\
					\tEAX:0x%.8X\tEBX:0x%.8X\tECX:0x%.8X\n\
					\tEDX:0x%.8X\tESI:0x%.8X\tEDI:0x%.8X\n\
					\tESP:0x%.8X\tEBP:0x%.8X\tEIP:0x%.8X\n\
					\tEFLAGS:0x%.8X\n\
					\tCS:0x%.4X\tSS:0x%.4X\tDS:0x%.4X\n\
					\tES:0x%.4X\tFS:0x%.4X\tGS:0x%.4X\n",
					ExceptionNumber, ErrorCode,
					Context->Eax, Context->Ebx, Context->Ecx,
					Context->Edx, Context->Esi, Context->Edi,
					Context->Esp, Context->Ebp, Context->Eip,
					Context->EFlag,
					Context->SegCs, Context->SegSs, Context->SegDs,
					Context->SegEs, Context->SegFs, Context->SegGs );
	} else {
		PsHandleException(ExceptionNumber, ErrorCode, Context);
	}

	return TRUE;
}
