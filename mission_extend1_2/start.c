/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: start.c

描述: EOS 内核的入口函数。



*******************************************************************************/

#include "ki.h"
#include "mm.h"
#include "ob.h"
#include "ps.h"
#include "io.h"

VOID
KiSystemStartup(
	PVOID LoaderBlock
	)
/*++

功能描述：
	系统的入口点，Kernel.dll被Loader加载到内存后从这里开始执行。

参数：
	LoaderBlock - Loader传递的加载参数块结构体指针，内存管理器要使用。

返回值：
	无（这个函数永远不会返回）。

注意：
	KiSystemStartup在Loader构造的ISR栈中执行，不存在当前线程，所以不能调用任何可
	能导致阻塞的函数，只能对各个模块进行简单的初始化。

--*/
{
	//
	// 初始化处理器和中断。
	//
	KiInitializeProcessor();
	KiInitializeInterrupt();

	//
	// 初始化可编程中断控制器和可编程定时计数器。
	//
	KiInitializePic();
	KiInitializePit();

	//
	// 对各个管理模块执行第一步初始化，顺序不能乱。
	//
	MmInitializeSystem1(LoaderBlock);
	ObInitializeSystem1();
	PsInitializeSystem1();
	IoInitializeSystem1();

	//
	// 创建系统启动进程。
	//
	PsCreateSystemProcess(KiSystemProcessRoutine);

	//
	// 执行到这里时，所有函数仍然在使用由 Loader 初始化的堆栈，所有系统线程
	// 都已处于就绪状态。执行线程调度后，系统线程开始使用各自的线程堆栈运行。
	//
	KeThreadSchedule();

	//
	// 本函数永远不会返回。
	//
	ASSERT(FALSE);
}
