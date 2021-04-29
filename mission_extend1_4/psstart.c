/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: psstart.c

描述: 线程、用户进程的启动函数。



*******************************************************************************/

#include "psp.h"

VOID
PspThreadStartup(
	VOID
	)
/*++

功能描述：
	线程启动函数，所有线程都从这里开始执行。

参数：
	无

返回值：
	无

--*/
{
	ULONG ExitCode;
	
	//
	// 调用当前线程的线程函数。
	//
	ExitCode = PspCurrentThread->StartAddr(PspCurrentThread->Parameter);

	//
	// 退出当前线程。
	//
	PsExitThread(ExitCode);

	//
	// 本函数永远不返回。
	//
	ASSERT(FALSE);
}

ULONG
PspProcessStartup(
	PVOID Parameter
	)
/*++

功能描述：
	用户进程的主线程函数。

参数：
	Parameter -- 无用。

返回值：
	正常情况下本函数永远不会返回，因为用户进程的入口函数应该是crt中的启动函数，
	当main函数结束后，crt应该调用ExitProcess结束进程。

--*/
{
	//
	// 调用进程映像的入口地址。
	//
	PspCurrentProcess->ImageEntry();

	return -1;
}
