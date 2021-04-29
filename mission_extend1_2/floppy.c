/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: floppy.c

描述: EOS 软盘驱动程序。
	  为了代码简洁易懂，仅支持一个 1.44M 的软盘驱动器。



*******************************************************************************/

#include "iop.h"

//
// 软盘控制器寄存器端口定义。
//
#define DOR				0x03f2	// 数字输出寄存器。
#define FDC_STATUS		0x03f4	// FDC主状态寄存器。
#define FDC_DATA		0x03f5	// FDC数据寄存器。
#define DIR				0x03f7	// 数字输入寄存器。
#define DCR				0x03f7	// 软盘控制寄存器。

//
// 数字输出寄存器位定义。
//
#define DOR_MOT_EN3		0x80	// 启动软驱D马达：1-启动；0-关闭。
#define DOR_MOT_EN2		0x40	// 启动软驱C马达：1-启动；0-关闭。
#define DOR_MOT_EN1		0x20	// 启动软驱B马达：1-启动；0-关闭。
#define DOR_MOT_EN0		0x10	// 启动软驱A马达：1-启动；0-关闭。
#define DOR_DMA_INT		0x08	// 1-允许DMA中断请求；0-禁止DMA中断请求。
#define DOR_RESET		0x04	// 1-允许软盘控制器FDC工作；0-复位FDC。

//
// 主状态寄存器定义。
//
#define	MSR_RQM			0x80	// FDC_DATA就绪。
#define MSR_DIO			0x40	// 传输方向：1-FDC->CPU；0-CPU->FDC。
#define MSR_NDMA		0x20	// 非DMA方式。
#define MSR_CB			0x10	// 控制器忙。
#define MSR_DDB			0x08	// 软驱D忙。
#define MSR_DCB			0x04	// 软驱C忙。
#define MSR_DBB			0x02	// 软驱B忙。
#define MSR_DAB			0x01	// 软驱A忙。

//
// 软盘控制器命令定义。
//
#define FD_RECALIBRATE	0x07	// 重新校正命令。
#define FD_SEEK			0x0F	// 寻道命令。
#define FD_READ			0xE6	// 读扇区命令。
#define FD_WRITE		0xC5	// 写扇区命令。
#define FD_SENSE		0x08	// 检测中断状态命令。
#define FD_SPECIFY		0x03	// 设定驱动器参数命令。
#define FD_CONFIGURE	0x13	// 配置命令。

//
// 定义软驱马达的状态。
//
typedef enum _MOTOR_STATE {
	ON,		// 0，开。
	DELAY,	// 1，延时关（仍然开着）。
	OFF		// 2，关。
} MOTOR_STATE;

//
// 当前软驱马达的状态。
//
PRIVATE UCHAR MotorState = OFF;

//
// 用于延时关闭软驱马达的计时器。
//
PRIVATE KTIMER DelayedOffTimer;

//
// 软驱中断事件。
//
PRIVATE EVENT InterruptEvent;


PRIVATE VOID
FloppyIsr(
	VOID
	)
/*++

功能描述：
	软驱中断服务程序。仅仅设置软驱中断事件为有效，从而通知驱动服务线程中断到达。

参数：
	无。

返回值：
	无。

--*/
{
	PsSetEvent(&InterruptEvent);
}

PRIVATE VOID
FloppyTurnOnMotor(
	VOID
	)
/*++

功能描述：
	启动软驱马达。

参数：
	无。

返回值：
	无。

--*/
{
	BOOL IntState;

	//
	// 本操作和延时关闭马达的计时器互斥，需要关闭中断。
	//
	IntState = KeEnableInterrupts(FALSE);

	if (DELAY == MotorState) {

		//
		// 软驱马达正在被延时关闭（仍在正常转动），注销延迟关闭计时器即可。
		//
		KeUnregisterTimer(&DelayedOffTimer);

	} else if (OFF == MotorState) {

		//
		// 马达处于关闭状态，启动软驱A的马达。
		//
		WRITE_PORT_UCHAR((PUCHAR)DOR, DOR_MOT_EN0 | DOR_DMA_INT | DOR_RESET);

		//
		// 软驱马达启动300-500ms后才能达到稳定转速，睡眠等待之。
		//
		PsSleep(500);
	}

	//
	// 修改软驱马达状态。
	//
	MotorState = ON;

	//
	// 恢复中断状态。
	//
	KeEnableInterrupts(IntState);
}

PRIVATE VOID
DelayOffTimerRoutine(
	IN ULONG_PTR Parameter
	)
/*++

功能描述：
	延迟关闭马达的计时器函数，真正关闭马达。

参数：
	Parameter -- 无用。

返回值：
	无。

--*/
{
	ASSERT(DELAY == MotorState);

	//
	// 关闭软驱马达、修改状态、注销计时器。
	//
	WRITE_PORT_UCHAR((PUCHAR)DOR, DOR_DMA_INT | DOR_RESET);
	
	MotorState = OFF;

	KeUnregisterTimer(&DelayedOffTimer);
}

PRIVATE VOID
FloppyTurnOffMotor(
	VOID
	)
/*++

功能描述：
	关闭软驱马达。

参数：
	无。

返回值：
	无。

--*/
{
	BOOL IntState;

	//
	// 本操作和延时关闭马达的计时器互斥，需要关闭中断。
	//
	IntState = KeEnableInterrupts(FALSE);

	if (ON == MotorState) {

		//
		// 为了避免频繁启动马达而不立刻关闭马达，如果在3秒内不再使用软驱，软驱马
		// 达才将被关闭。现在要做的仅仅是注册一个延迟3秒关闭马达的计时器。
		//
		KeInitializeTimer( &DelayedOffTimer,
						   3000,
						   DelayOffTimerRoutine,
						   0 );

		KeRegisterTimer(&DelayedOffTimer);

		MotorState = DELAY;
	}

	KeEnableInterrupts(IntState);
}

PRIVATE BOOL
FloppyWriteFdc(
	IN PUCHAR Buffer,
	IN UCHAR BytesToWrite
	)
/*++

功能描述：
	写数据到FDC_DATA寄存器。

参数：
	Buffer - 数据缓冲区指针。
	BytesToWrite - 期望写的字节数。

返回值：
	如果成功则返回TRUE，否则返回FALSE。

--*/
{
	UCHAR i;
	UCHAR ErrorCount;
	UCHAR Msr;

	for (i = 0; i < BytesToWrite; i++) {

		for (ErrorCount = 0;;) {

			//
			// 读取FDC_STATUS主状态寄存器。
			//
			Msr = READ_PORT_UCHAR((PUCHAR)FDC_STATUS);

			if ((Msr & MSR_RQM) != 0) {

				//
				// FDC_DATA就绪，如果传输方向为CPU->FDC则可以写FDC_DATA了，否则
				// 返回失败。
				//
				if ((Msr & MSR_DIO) == 0) {
					WRITE_PORT_UCHAR((PUCHAR)FDC_DATA, Buffer[i]);
					break;
				} else {
					return FALSE;
				}

			} else {

				//
				// FDC未就绪，如果对同一字节重试达到3次则返回失败，否则等待重试。
				//
				if (3 == ++ErrorCount) {
					return FALSE;
				}

				//
				// 睡眠等待一小会儿zZZ^
				//
				PsSleep(10);
			}
		}
	}

	return TRUE;
}

PRIVATE BOOL
FloppyReadFdc(
	OUT PUCHAR Buffer,
	IN UCHAR BytesToRead
	)
/*++

功能描述：
	读取FDC执行命令的结果字节序列。

参数：
	Buffer - 用于输出读取结果的缓冲区的指针。
	BytesToRead - 期望读取的字节数。

返回值：
	如果成功则返TRUE，否则返回FALSE。

--*/
{
	UCHAR i;
	UCHAR ErrorCount; 
	UCHAR Msr;

	ASSERT(BytesToRead > 0);

	for (i = 0;;) {

		for (ErrorCount = 0;;) {

			//
			// 读取FDC_STATUS主状态寄存器。
			//
			Msr = READ_PORT_UCHAR((PUCHAR)FDC_STATUS);

			if ((Msr & MSR_RQM) != 0) {

				//
				// FDC_DATA就绪，如果传输方向为FDC->CPU则可以读取FDC_DATA了，否
				// 则说明数据读取完毕。
				//
				if ((Msr & MSR_DIO) != 0) {

					//
					// 继续读取FDC_DATA，如果数据长度超过期望读取的长度则返回失败。
					//
					if (i == BytesToRead) {
						return FALSE;
					}

					Buffer[i++] = READ_PORT_UCHAR((PUCHAR)FDC_DATA);

					//
					// 继续读取下一字节。
					//
					break;

				} else {

					//
					// 数据读取完毕，如果读取长度达到期望读取长度则返回TRUE。
					//
					return (i == BytesToRead);
				}

			} else {

				//
				// FDC未就绪，如果对同一字节重试达到3次则返回失败，否则等待重试。
				//
				if (3 == ++ErrorCount) {
					return FALSE;
				}

				//
				// 睡眠等待一小会儿zZZ^
				//
				PsSleep(10);
			}
		}
	}
}

PRIVATE VOID
FloppyResetFDC(
	VOID
	)
/*++

功能描述：
	复位软驱控制器。

参数：
	无。

返回值：
	无。

--*/
{
	UCHAR DataBuffer[4];

	for(;;) {

		//
		// 使DOR的RESET位为0并保持一小会儿，然后再恢复为1，即可复位控制器。
		//
		WRITE_PORT_UCHAR((PUCHAR)DOR, DOR_MOT_EN0 | DOR_DMA_INT);

		PsSleep(10);

		WRITE_PORT_UCHAR((PUCHAR)DOR, DOR_MOT_EN0 | DOR_DMA_INT | DOR_RESET);

		//
		// 软驱控制器复位完成后会触发一个中断，等待复位中断的到来。
		//
		PsWaitForEvent(&InterruptEvent, INFINITE);

		//
		// 检测中断状态，确保是复位中断。
		//
		DataBuffer[0] = FD_SENSE;

		if (!FloppyWriteFdc(DataBuffer, 1) ||
			!FloppyReadFdc(DataBuffer, 2) ||
			0xC0 != DataBuffer[0] ) {
			continue;
		}

		//
		// 设定1.44M软盘驱动器的参数。
		//
		DataBuffer[0] = FD_SPECIFY;
		DataBuffer[1] = 0xDF;
		DataBuffer[2] = 8 << 1;

		if (!FloppyWriteFdc(DataBuffer, 3)) {
			continue;
		}

		//
		// 启动隐含寻道。
		//
		DataBuffer[0] = FD_CONFIGURE;
		DataBuffer[1] = 0;
		DataBuffer[2] = 0x60;
		DataBuffer[3] = 0;

		if (!FloppyWriteFdc(DataBuffer, 4)) {
			continue;
		}

		//
		// 设置传输速率，1.44M软盘驱动器的传输速率是500kb/s。
		//
		WRITE_PORT_UCHAR((PUCHAR)DCR, 0);

		break;
	}
}

PRIVATE BOOL
FloppyRecalibrate(
	VOID
	)
/*++

功能描述：
	校正磁头位置，将磁头强制归位到0磁道。

参数：
	无。

返回值：
	如果成功则返回TRUE。

--*/
{
	UCHAR DataBuffer[2];

	for (;;) {

		//
		// 对驱动器0重新校正。
		//
		DataBuffer[0] = FD_RECALIBRATE;
		DataBuffer[1] = 0;

		if (FloppyWriteFdc(DataBuffer, 2)) {

			//
			// 成功发送重新校正命令，等待校正中断。
			//
			PsWaitForEvent(&InterruptEvent, INFINITE);

			//
			// 检测中断状态，确保是校正中断。
			//
			DataBuffer[0] = FD_SENSE;

			if (FloppyWriteFdc(DataBuffer, 1) &&
				FloppyReadFdc(DataBuffer, 2) &&
				(DataBuffer[0] & 0x20) != 0) {

				break;
			}
		}

		//
		// FDC逻辑错误，复位软驱控制器然后重新校正。
		//
		FloppyResetFDC();
	}

	//
	// 命令正常结束后磁头应该位于0磁道。
	//
	return (0 == (DataBuffer[0] >> 6) && 0 == DataBuffer[1]);
}

PRIVATE STATUS
FloppyRw(
	IN PVOID Buffer,
	IN USHORT StartingSector,
	IN USHORT SectorsToRW,
	IN BOOL IsRead
	)
/*++

功能描述：
	读写磁盘扇区。

参数：
	Buffer -- 缓冲区的物理地址。
	StartingSector -- 读写的起始扇区。
	SectorsToRW -- 期望读写的扇区数量。
	IsRead -- TRUE:读扇区；FALSE:写扇区。

返回值：
	如果成功则返回STATUS_SUCESS。

--*/
{
	STATUS Status;
	BOOL IntState;
	UCHAR Cylinder;
	UCHAR Head;
	UCHAR Sector;
	UCHAR ErrorCount;
	UCHAR DataBuffer[9];
	USHORT BytesToRW;
	PVOID BufferPhysicalAdress;

	Status = MmGetPhysicalAddress(Buffer, &BufferPhysicalAdress);

	//
	// DMA只能寻址1M的物理内存。
	//
	if (!EOS_SUCCESS(Status) || (ULONG_PTR)BufferPhysicalAdress >= 0x100000) {
		return STATUS_INVALID_ADDRESS;
	}

	//
	// 计算IO起始扇区的CHS编址和字节数。
	//
	Cylinder = (StartingSector / 18) / 2;
	Head = (StartingSector / 18) % 2;
	Sector = (StartingSector % 18) + 1;
	BytesToRW = SectorsToRW * 512;

	//
	// 启动软驱马达。
	//
	FloppyTurnOnMotor();

	//
	// 执行IO操作，如果IO错误则校正磁头位置后重试，最多重试6次。
	//
	Status = STATUS_FLOPPY_UNKNOWN_ERROR;

	for (ErrorCount = 0; ErrorCount < 6;) {

		//
		// 初始化DMA通道2，关闭中断防止打扰。
		//
		IntState = KeEnableInterrupts(FALSE);

		//
		// 屏蔽通道2请求。
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0A, 0x06);

		//
		// 清除高/低触发器。
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0C, 0x06);

		//
		// 写方式控制字，工作模式为单字节递增读/写传送，禁止自动预置。
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0B, IsRead ? 0x46 : 0x4A);

		//
		// 分别写低8位和高8位地址寄存器。
		//
		WRITE_PORT_UCHAR((PUCHAR)0x04, (UCHAR)((ULONG_PTR)BufferPhysicalAdress));
		WRITE_PORT_UCHAR((PUCHAR)0x04, (UCHAR)((ULONG_PTR)BufferPhysicalAdress >> 8));

		//
		// 写地址的16-19位到页面寄存器。
		//
		WRITE_PORT_UCHAR((PUCHAR)0x81, (UCHAR)((ULONG_PTR)Buffer >> 16));

		//
		// 分别写低8位和高8位传送字节数寄存器。
		//
		WRITE_PORT_UCHAR((PUCHAR)0x05, (UCHAR)(BytesToRW - 1));
		WRITE_PORT_UCHAR((PUCHAR)0x05, (UCHAR)((BytesToRW - 1) >> 8));

		//
		// 允许通道2请求。
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0A, 0x02);

		//
		// DMA初始化完成，恢复中断。
		//
		KeEnableInterrupts(IntState);

		//
		// 向FDC发读/写命令。
		//
		DataBuffer[0] = IsRead ? FD_READ : FD_WRITE;	// 命令字节
		DataBuffer[1] = Head << 2;						// 磁头号 和 驱动器号
		DataBuffer[2] = Cylinder;						// 柱面
		DataBuffer[3] = Head;							// 磁头
		DataBuffer[4] = Sector;							// 扇区
		DataBuffer[5] = 2;								// 扇区大小（512Bytes)
		DataBuffer[6] = 18;								// 每磁道扇区数
		DataBuffer[7] = 0x1B;							// 扇区之间的间隔长度
		DataBuffer[8] = 0xFF;							// 无用

		if (!FloppyWriteFdc(DataBuffer, 9)) {						

			//
			// 控制器逻辑出错，复位控制器。
			//
			FloppyResetFDC();
			continue;
		}

		//
		// 等待读写完成中断。
		//
		PsWaitForEvent(&InterruptEvent, INFINITE);

		//
		// 读取读/写命令的执行结果。
		//
		if (!FloppyReadFdc(DataBuffer, 7)) {

			//
			// 控制器逻辑出错，复位控制器。
			//
			FloppyResetFDC();
			continue;
		}

		if ((DataBuffer[0] >> 6) != 0) {

			//
			// 命令异常结束。
			//
			ASSERT(0x01 == (DataBuffer[0] >> 6));

			if ((DataBuffer[1] & 0x02) != 0) {

				//
				// 软盘写保护，中止写操作。
				//
				Status = STATUS_ACCESS_DENIED;
				break;
			}

			//
			// 磁盘读写错误，增加错误计数器，校正磁头位置后重试。
			//
			ErrorCount++;
			FloppyRecalibrate();
			continue;
		}

		Status = STATUS_SUCCESS;
		break;
	}

	//
	// 关闭马达。
	//
	FloppyTurnOffMotor();

	return Status;
}

PRIVATE STATUS
FloppyRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	)
/*++

功能描述：
	软驱驱动的读派遣例程，读取指定的一个扇区到指定的缓冲区中。

参数：
	参看 iop.h 中对驱动程序 Read 功能函数的介绍。

返回值：
	如果成功则返回 STATUS_SUCCESS。

--*/
{
	//
	// 1.44M 软盘只有 2880 个扇区。
	//
	if (Request >= 2880) {
		return STATUS_INVALID_PARAMETER;
	}

	return FloppyRw(Buffer, Request, 1, TRUE);
}

PRIVATE STATUS
FloppyWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	)
/*++

功能描述：
	软驱驱动的写派遣例程，写缓冲区中的数据到指定的一个扇区中。

参数：
	参看 iop.h 中对驱动程序 Write 功能函数的介绍。

返回值：
	如果成功则返回 STATUS_SUCCESS。

--*/
{
	//
	// 1.44M 软盘只有 2880 个扇区。
	//
	if (Request >= 2880) {
		return STATUS_INVALID_PARAMETER;
	}

	return FloppyRw(Buffer, Request, 1, FALSE);
}

PRIVATE STATUS
FlopyAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT NextLayerDevice,
	IN USHORT DeviceNumber,
	OUT PDEVICE_OBJECT *DeviceObject
	)
{
	STATUS Status;

	//
	// 软驱没有下层设备且目前驱动程序仅支持一个软驱设备。
	//
	ASSERT(NULL == NextLayerDevice);
	ASSERT(0 == DeviceNumber);

	Status = IopCreateDevice( DriverObject,
							  0,
							  "FLOPPY0",
							  DeviceNumber,
							  TRUE,
							  DeviceObject);

	if (EOS_SUCCESS(Status)) {

		//
		// 初始化中断事件。
		//
		PsInitializeEvent(&InterruptEvent, FALSE, FALSE);

		//
		// 设置中断向量。
		//
		KeIsrFloppy = FloppyIsr;

		//
		// 允许设备中断。
		//
		KeEnableDeviceInterrupt(INT_FLOPPY, TRUE);

		//
		// 复位软驱控制器。
		//
		FloppyResetFDC();
	}
	
	return Status;
}

VOID
FloppyInitializeDriver(
	PDRIVER_OBJECT DriverObject
	)
{
	DriverObject->AddDevice = FlopyAddDevice;

	//
	// 磁盘不支持直接被读写，必须通过磁盘之上的文件系统访问磁盘，所以磁盘驱动不提
	// 供Create和Close功能函数。
	//
	DriverObject->Read = FloppyRead;
	DriverObject->Write = FloppyWrite;
}
