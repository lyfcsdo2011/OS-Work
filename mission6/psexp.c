/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: psexp.c

描述: 进程（线程）异常处理模块。



*******************************************************************************/

#include "psp.h"

VOID
PsHandleException(
	ULONG ExceptionNumber,
	ULONG ErrorCode,
	PVOID Context
	)
/*++

功能描述：
	处理进程运行时产生的异常。
	目前不做太多处理，如果是系统进程产生异常则直接蓝屏，否则直接结束用户进程。

参数：
	ExceptionNumber -- 异常号。
	ErrorCode -- 异常错误码。
	Context -- 异常的上下文环境。

返回值：
	无。

--*/
{
	PCONTEXT Cpu = (PCONTEXT)Context;

	ASSERT(KeGetIntNesting() == 1);

	if (PspCurrentProcess == PspSystemProcess) {

		KeBugCheck( "System process error!\n\
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
					Cpu->Eax, Cpu->Ebx, Cpu->Ecx,
					Cpu->Edx, Cpu->Esi, Cpu->Edi,
					Cpu->Esp, Cpu->Ebp, Cpu->Eip,
					Cpu->EFlag,
					Cpu->SegCs, Cpu->SegSs, Cpu->SegDs,
					Cpu->SegEs, Cpu->SegFs, Cpu->SegGs );
	}

	PspTerminateProcess(PspCurrentProcess, -1);
}
