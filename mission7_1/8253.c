/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: 8253.c

描述: PC 机 8253 可编程定时计数器 (Programmable Interval Timer) 的初始化。



*******************************************************************************/

#include "ki.h"

VOID 
KiInitializePit(
	VOID
	)
{
	//
	// 初始化 8253 每秒钟中断 100 次。
	//
	WRITE_PORT_UCHAR((PUCHAR)0x43, 0x34);
	WRITE_PORT_UCHAR((PUCHAR)0x40, (UCHAR)(11932 & 0xFF));
	WRITE_PORT_UCHAR((PUCHAR)0x40, (UCHAR)((11932 >> 8) & 0xFF));

	//
	// 打开 8253 中断。
	//
	KeEnableDeviceInterrupt(INT_TIMER, TRUE);
}
