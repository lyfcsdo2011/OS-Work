/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: bugcheck.c

描述: PC 机 KeBugCheck 的实现，KeBugCheck 依赖硬件平台。



*******************************************************************************/

#include "ki.h"

//
// VGA缓冲位置，和内存映射相关。
//
#define VGA_BUFFER				0x800B8000

VOID
KeBugCheck(
	IN PCSTR Format,
	...
	)
{
	PCHAR DestPtr, SrcPtr;
	PULONG Pos;
	va_list argptr;

	//
	// 关闭中断。
	//
	KeEnableInterrupts(FALSE);

	//
	// 从第0页开始显示。
	//
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0C);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0x00);
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0D);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0x00);

	//
	// 隐藏光标（放到第二页）。
	//
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0E);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0x07);
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0F);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0xD0);

	//
	// 刷第一页的缓冲区为蓝底白字。
	//
	for(Pos = (PULONG)VGA_BUFFER; Pos < (PULONG)VGA_BUFFER + 1000; Pos++) {
		*Pos = 0x1F001F00;
	}

	//
	// 格式化字符串，用第二页视频缓冲区作为格式化输出缓冲区。
	//
	va_start(argptr, Format);
	vsprintf((PCHAR)VGA_BUFFER + 4000, Format, argptr);

	//
	// 显示标题。
	//
	DestPtr = (PCHAR)VGA_BUFFER;
	SrcPtr = "This Message is print by KeBugCheck().";
	while(*SrcPtr != 0)
	{
		*DestPtr = *SrcPtr++;
		DestPtr += 2;
	}

	//
	// 显示正文。
	//
	DestPtr = (PCHAR)VGA_BUFFER + 160;
	for (SrcPtr = (PCHAR)VGA_BUFFER + 4000; *SrcPtr!=0; SrcPtr++)
	{
		if ('\n' == *SrcPtr)
		{
			DestPtr = DestPtr - (DestPtr - (PCHAR)VGA_BUFFER) % 160 + 80;
		}
		else if ('\t' == *SrcPtr)
		{
			DestPtr = (PCHAR)(((ULONG_PTR)DestPtr + 0x10) & ~0xF);
		}
		else
		{
			*DestPtr = *SrcPtr;
			DestPtr += 2;
		}
	}

	/* 死循环。*/
	while(1);
}
