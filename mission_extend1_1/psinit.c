/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: psinit.c

描述: 进程管理模块的初始化。



*******************************************************************************/

#include "psp.h"

VOID
PspCreateProcessObjectType(
	VOID
	);

VOID
PspCreateEventObjectType(
	VOID
	);

VOID
PspCreateMutexObjectType(
	VOID
	);

VOID
PspCreateSemaphoreObjectType(
	VOID
	);

VOID
PspInitSchedulingQueue(
	VOID
	);

VOID
PsInitializeSystem1(
	VOID
	)
/*++

功能描述：
	初始化进程管理模块。

参数：
	无。

返回值：
	无。

--*/
{
	//
	// 创建进程和线程对象类型。
	//
	PspCreateProcessObjectType();

	//
	// 创建事件对象类型。
	//
	PspCreateEventObjectType();

	//
	// 创建互斥信号量对象类型。
	//
	PspCreateMutexObjectType();

	//
	// 创建记录型信号量对象类型。
	PspCreateSemaphoreObjectType();

	//
	// 初始化线程调度要使用的各种队列。
	//
	PspInitSchedulingQueue();
}

VOID
PsInitializeSystem2(
	VOID
	)
{

}
