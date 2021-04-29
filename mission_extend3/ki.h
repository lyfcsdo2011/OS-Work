/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: ki.h

描述: 内核支撑模块的内部函数及变量的声明。



*******************************************************************************/

#ifndef _KI_
#define _KI_

#include "ke.h"

//
// 中断计数器，指示当前中断嵌套的深度。
//
extern ULONG KiIntNesting;


//
// 初始化全局描述符表。
//
VOID
KiInitializeProcessor(
	VOID
	);

//
// 初始化中断描述符表。
//
VOID
KiInitializeInterrupt(
	VOID
	);

//
// 初始化可编程中断控制器(Programmable Interrupt Controller)。
//
VOID
KiInitializePic(
	VOID
	);

//
// 可编程定时计数器(Programmable Interval Timer)。
//
VOID 
KiInitializePit(
	VOID
	);

//
// 停机指令。
//
VOID
KiHaltProcessor(
	VOID
	);

//
// 计时器中断函数。
//
VOID
KiIsrTimer(
	VOID
	);

//
// 退出系统启动函数。
//
VOID
KiStartExit(
	VOID
	);

//
// 系统进程函数。
//
ULONG
KiSystemProcessRoutine(
	IN PVOID Parameter
	);

#endif // _KI_
