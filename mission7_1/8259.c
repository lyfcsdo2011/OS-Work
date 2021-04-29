/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: 8259.c

描述: PC 机 8259 可编程中断控制器 (Programmable Interrupt Controller) 的控制。



*******************************************************************************/

#include "ki.h"

//
// 8259 可编程中断控制器 (PIC) 的寄存器地址
//
#define PIC1_PORT0				0x20         // master PIC
#define PIC1_PORT1				0x21
#define PIC2_PORT0				0x0A0        // slave PIC
#define PIC2_PORT1				0x0A1

//
// 8259 可编程中断控制器 (PIC) 的命令
//
#define PIC1_EOI_MASK			0x60
#define PIC2_EOI				0x62
#define OCW2_NON_SPECIFIC_EOI	0x20
#define OCW3_READ_ISR			0xb
#define OCW3_READ_IRR			0xa

VOID
KiInitializePic(
	VOID
	)
/*++

功能描述：
	初始化8259可编程中断控制器。

参数：
	无。

返回值：
	无。

--*/
{
	ASSERT((PIC1_VECTOR & 0x07) == 0);
	ASSERT((PIC2_VECTOR & 0x07) == 0);

	//
	// 初始化 PIC
	//
	WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT0, 0x11);			// Master, ICW1.
	WRITE_PORT_UCHAR((PUCHAR)PIC2_PORT0, 0x11);			// Slave, ICW1.
	WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT1, PIC1_VECTOR);	// Master, ICW2. Set int number as PIC1_VECTOR.
	WRITE_PORT_UCHAR((PUCHAR)PIC2_PORT1, PIC2_VECTOR);	// Slave, ICW2. Set int number as PIC2_VECTOR.
	WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT1, 0x04);			// Master, ICW3. IR2 -> slave.
	WRITE_PORT_UCHAR((PUCHAR)PIC2_PORT1, 0x02);			// Slave, ICW3. -> master IR2.
	WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT1, 0x01);			// Master, ICW4.
	WRITE_PORT_UCHAR((PUCHAR)PIC2_PORT1, 0x01);			// Slave, ICW4.

	//
	// 关闭所有设备的中断
	//
	WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT1, 0xFF);	// Master 8259, OCW1.
	WRITE_PORT_UCHAR((PUCHAR)PIC2_PORT1, 0xFF);	// Slave  8259, OCW1.
}

VOID
KeEnableDeviceInterrupt(
	ULONG IntVector,
	BOOL Enable
	)
/*++

功能描述：
	屏蔽或允许指定外部设备的中断请求。

参数：
	IntVector -- 外部设备的中断向量号。
	Enable -- TRUE 则允许中断请求，FALSE 则屏蔽中断请求。

返回值：
	无。

--*/
{
	PUCHAR Port;
	UCHAR OCW1;

	ASSERT(PIC1_VECTOR == (IntVector & ~0x07) ||
		PIC2_VECTOR == (IntVector & ~0x07));

	//
	// 根据设备中断号选择相应的 PIC 的端口地址。
	//
	if (PIC1_VECTOR == (IntVector & ~0x07)) {
		Port = (PUCHAR)PIC1_PORT1;
	} else if (PIC2_VECTOR == (IntVector & ~0x07)) {
		Port = (PUCHAR)PIC2_PORT1;
	}

	//
	// 读取 OCW1 状态字。
	//
	OCW1 = READ_PORT_UCHAR(Port);

	//
	// 根据开关动作要求，修改 OCW1 对应的状态位。
	//
	if (Enable) {
		OCW1 = OCW1 & ~(1 << (IntVector & 0x07));
	} else {
		OCW1 = OCW1 | (1 << (IntVector & 0x07));
	}

	//
	// 回写 OCW1 状态字
	//
	WRITE_PORT_UCHAR(Port, OCW1);
}

VOID
Ki8259EOI(
	VOID
	)
/*++

功能描述：
	向 8259 发送中断结束命令。

参数：
	无。

返回值：
	无。

警告：
	不要在此函数中插入断点。即使调试到此函数中，也不要使用“逐过程”
	或者“跳出”等单步调试功能，这些操作会产生不可预测的调试结果。
	建议使用“继续调试”功能继续调试。

--*/
{
	WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT0, 0x20);
}
