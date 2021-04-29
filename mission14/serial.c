/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: serial.c

描述: 串口驱动程序的实现。
	  实现了基本的串口通信，不提供串口属性设置和通信错误处理，仅为了演示驱动程序
	  工作过程。



*******************************************************************************/

#include "iop.h"

//
// 串口设备对象的扩展块。
//
typedef struct _SERIAL_DEVICE_EXTENSION {
	PRING_BUFFER SendBuffer;		// 用于发送数据的环形缓冲区。
	EVENT CompletionEvent;			// 发送缓冲区空事件。当发送缓冲区为空时处于signaled状态

	PRING_BUFFER RecvBuffer;		// 接收数据缓冲区
	EVENT RecvBufferNotEmpty;		// 当接收缓冲区非空时处于signaled状态
	

}SERIAL_DEVICE_EXTENSION, *PSERIAL_DEVICE_EXTENSION;

//
// COM1和COM2对应的设备对象指针，供中断服务程序使用。
//
PDEVICE_OBJECT SrlDevice[2] = {NULL, NULL};

//
// COM1和COM2的寄存器端口地址的基址。
//
const PUCHAR SrlRegPortBase[2] = {(PUCHAR)0x03F8, (PUCHAR)0x02F8};

//
// 8250的寄存器端口地址定义(相对于0x3F8或0x2F8的偏移)。
//
#define RBR			0
#define THR			0
#define IER			1
#define IIR			2
#define LCR			3
#define MCR			4
#define LSR			5
#define MSR			6
#define DIVRL		0
#define DIVRH		1

//
// 取设备对象对应的COM端口的寄存器地址，d-设备对象指针，r-RBR、THR等。
//
#define REG_PORT(d, r) (SrlRegPortBase[(d)->DeviceNumber] + (r))

STATUS
SrlAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT NextLayerDevice,
	IN USHORT DeviceNumber,
	OUT PDEVICE_OBJECT *DeviceObject
	);

STATUS
SrlCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN ULONG CreationDisposition,
	IN OUT PFILE_OBJECT FileObject
	);

VOID
SrlClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PFILE_OBJECT FileObject
	);

STATUS
SrlRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	);

STATUS
SrlWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	);

VOID
SrlIsr(
   PDEVICE_OBJECT Device
   );

//
// COM1和COM2的中断服务程序。
//
VOID
SrlIsrCom1(
	VOID
	)
{
	SrlIsr(SrlDevice[0]);
}

VOID
SrlIsrCom2(
	VOID
	)
{
	SrlIsr(SrlDevice[1]);
}

//
// 驱动程序初始化函数。
//
VOID
SrlInitializeDriver(
	PDRIVER_OBJECT DriverObject
	)
{
	DriverObject->AddDevice = SrlAddDevice;
	DriverObject->Create = SrlCreate;
	DriverObject->Close = SrlClose;
	DriverObject->Read = SrlRead;
	DriverObject->Write = SrlWrite;
}

//
// 串口驱动的AddDevice功能函数，参数和返回值请参看iop.h源文件。
//
STATUS
SrlAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT NextLayerDevice,
	IN USHORT DeviceNumber,
	OUT PDEVICE_OBJECT *DeviceObject
	)
{
	STATUS Status;
	CHAR DeviceName[] = "COM1";
	PDEVICE_OBJECT SerialDevice;
	PSERIAL_DEVICE_EXTENSION Ext;

	ASSERT(NULL == NextLayerDevice); // 串口没有下层设备
	ASSERT(0 == DeviceNumber || 1 == DeviceNumber); // 目前仅支持COM1和COM2两个串口设备
	ASSERT(NULL == SrlDevice[DeviceNumber]);

	//
	// COM1、COM2的设备编号分别为0，1，设备名分别是“COM1”、“COM2”。
	//
	DeviceName[3] += DeviceNumber;

	//
	// 创建串口对应的设备对象。
	//
	Status = IopCreateDevice( DriverObject,				// 串口驱动程序对象指针
							  sizeof(SERIAL_DEVICE_EXTENSION),	// 设备扩展块的大小
							  DeviceName,				// 设备名，COM1为“COM1”，COM2为“COM2”
							  DeviceNumber,				// 设备编号，COM1为0，COM2为1
							  FALSE,					// 不是块设备
							  &SerialDevice );

	if (EOS_SUCCESS(Status)) {

		SrlDevice[DeviceNumber] = SerialDevice;

		//
		// 初始化设备扩展块。
		//
		Ext = (PSERIAL_DEVICE_EXTENSION)SerialDevice->DeviceExtension;
		Ext->SendBuffer = IopCreateRingBuffer(256);				// 环行缓冲区初始化为 256 个字节。
		PsInitializeEvent(&Ext->CompletionEvent, FALSE, FALSE);	// 发送缓冲区空事件初始化为无效、自动事件。

		
		Ext->RecvBuffer = IopCreateRingBuffer(256);				// 环行缓冲区初始化为 256 个字节。
		PsInitializeEvent(&Ext->RecvBufferNotEmpty, TRUE, FALSE);	// 接收缓冲区非空事件初始化为无效、手动事件。


		//
		// 设置默认通信参数，波特率57600、8个数据位、一个停止位、无奇偶校验。
		//
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, LCR), 0x80);	// 写LCR使DLAB = 1，然后才能写除数寄存器。
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, DIVRL), 0x02);	// 写除数寄存器低8位。
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, DIVRH), 0);		// 写除数寄存器高8位，除数寄存器的值为：1843200 / (57600 * 16) = 2。
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, LCR), 0x03);	// LCR = 00000011B，8数据位、1停止位、无奇偶校验。
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, MCR), 0x0B);	// OUT2输出低电平，允许中断信号发出；~DTR和~RTS为低，正常工作。
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, IER), 0x03);	// 允许发送和接收中断。

		//
		// 设置中断服务程序的入口地址，取消8259可编程中断控制器对COM中断的屏蔽。
		//
		if (0 ==DeviceNumber) {
			KeIsrCom1 = SrlIsrCom1;
			KeEnableDeviceInterrupt(INT_COM1, TRUE);
		} else {
			KeIsrCom2 = SrlIsrCom2;
			KeEnableDeviceInterrupt(INT_COM2, TRUE);
		}

		*DeviceObject = SerialDevice;
	}

	return Status;
}

//
// 串口驱动的Create函数，参数和返回值请参看iop.h源文件。
//
STATUS
SrlCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN ULONG CreationDisposition,
	IN OUT PFILE_OBJECT FileObject
	)
{
	BOOL IntState;
	PSERIAL_DEVICE_EXTENSION Ext = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	//
	// 串口设备不存在子设备，路径名应该是空字符串。
	//
	if (*FileName != '\0') {
		return STATUS_FILE_NOT_FOUND;
	}

	//
	// 只能使用OPEN_EXISTING标志。
	//
	if (OPEN_EXISTING != CreationDisposition) {
		return STATUS_INVALID_PARAMETER;
	}

	if (0 == DeviceObject->OpenCount) {

		//
		// 串口设备由关闭状态被打开，可根据需要在此添加代码进行相应的处理。
		//

	}

	//
	// 打开设备成功，使文件对象和COM设备对象关联。
	//
	FileObject->FsContext = DeviceObject;
	return STATUS_SUCCESS;
}

//
// 串口驱动的CloseFile函数，参数和返回值请参看iop.h源文件。
//
VOID
SrlClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PFILE_OBJECT FileObject
	)
{
	if (0 == DeviceObject->OpenCount) {

		//
		// 设备由打开状态被关闭，可根据需要在此添加代码进行相应的处理。
		//

	}
}

//
// 串口驱动的Read功能函数，参数和返回值请参看iop.h源文件。
//
STATUS
SrlRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	)
{
	//
	// 在此添加读取串口设备的代码。
	//
	return STATUS_NOT_SUPPORTED;
}

//
// 串口驱动的Write功能函数，参数和返回值请参看iop.h源文件。
//
STATUS
SrlWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	)
{
	CHAR Data;
	ULONG Count = 0;
	PSERIAL_DEVICE_EXTENSION Ext = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	PsResetEvent(&Ext->CompletionEvent); // 复位发送完成事件为无效状态。

	while (Count < Request) {

		//
		// 记录要发送的第一个字节，将其余要发送的内容写入发送缓冲区。
		// 注意，缓冲区的大小可能不足以一次保存所有要发送的数据，IopWriteRingBuffer
		// 将返回实际写入缓冲区的字节数。
		//
		Data = ((PCHAR)Buffer)[Count++];
		Count += IopWriteRingBuffer(Ext->SendBuffer, Buffer + Count, Request - Count);

		//
		// 启动发送过程。
		// 启动发送过程仅需将第一个字节数据写入THR，THR中的数据被送出后会触发一个
		// THR空中断，中断处理程程序将继续发送缓冲区中的数据。缓冲区中数据被发送
		// 完后，中断处理程序会设置CompletionEvent事件为有效。
		//
		WRITE_PORT_UCHAR(REG_PORT(DeviceObject, THR), Data);

		//
		// 阻塞等待直到数据发送完毕。注意，等待函数返回后，该事件对象会自动变为无效状态。
		//
		PsWaitForEvent(&Ext->CompletionEvent, INFINITE);
	}

	*Result = Count;
	return STATUS_SUCCESS;
}

//
// 串口的中断服务程序。
//
VOID
SrlIsr(
   PDEVICE_OBJECT Device
   )
{
	CHAR Data;
	PSERIAL_DEVICE_EXTENSION Ext = (PSERIAL_DEVICE_EXTENSION)Device->DeviceExtension;

	//
	// 根据中断识别寄存器确定中断来源，进行相应的处理。
	//
	if (2 == READ_PORT_UCHAR(REG_PORT(Device, IIR))) {

		//
		// THR空，可以向THR写入下一个要发送的字节数据。
		// 如果发送缓冲区为空则说明发送过程结束，应设置发送完成事件为有效状态，
		// 否则，从发送缓冲区中读取一个字节数据并写入THR进行发送。
		//
		if (IopIsRingBufferEmpty(Ext->SendBuffer)) {
			PsSetEvent(&Ext->CompletionEvent);
		} else {
			IopReadRingBuffer(Ext->SendBuffer, &Data, 1);
			WRITE_PORT_UCHAR(REG_PORT(Device, THR), Data);
		}

	} else  {

		//
		// RBR就绪，读取RBR，得到接收到的数据。
		//
		Data = READ_PORT_UCHAR(REG_PORT(Device, RBR));

		//
		// 在此添加代码，进一步处理接收中断。
		//

	}
}

#define MAXSERIALEVENTWAITCOUNT 10
typedef struct _SERIALEVENT
{
	INT WaitObj[MAXSERIALEVENTWAITCOUNT];
	INT Count;	
}SERIALEVENT;

SERIALEVENT COM1SendEvent;
SERIALEVENT COM1RecvEvent;

#ifdef _DEBUG

PRIVATE VOID GetCOM1( )
/*++

功能描述：
	获取串口1的信息。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	PTHREAD pThread;	
	PLIST_ENTRY pWListEntry;
	INT WaitThreadNum = 0;
	INT Size = 0, FillCount = 0;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	if(SrlDevice[0] != 0)
	{
		if((*(PSERIAL_DEVICE_EXTENSION)(SrlDevice[0]-> DeviceExtension)).SendBuffer != 0 && (*(PSERIAL_DEVICE_EXTENSION)(SrlDevice[0]-> DeviceExtension)).RecvBuffer != 0)
		{
			for(WaitThreadNum = 0,
			pWListEntry = (*(PSERIAL_DEVICE_EXTENSION)(SrlDevice[0]-> DeviceExtension)).CompletionEvent.WaitListHead.Next;
			pWListEntry != NULL
			&& pWListEntry != &((*(PSERIAL_DEVICE_EXTENSION)(SrlDevice[0]-> DeviceExtension)).CompletionEvent.WaitListHead) 
			&& WaitThreadNum < MAXSERIALEVENTWAITCOUNT;
			pWListEntry = pWListEntry->Next, WaitThreadNum++)
			{
				//
				// 获得线程对象的指针
				//
				pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
				
				COM1SendEvent.WaitObj[WaitThreadNum] = ObGetObjectId(pThread);
			}
			COM1SendEvent.Count = WaitThreadNum;
		
			for(WaitThreadNum = 0,
			pWListEntry = (*(PSERIAL_DEVICE_EXTENSION)(SrlDevice[0]-> DeviceExtension)).RecvBufferNotEmpty.WaitListHead.Next;
			pWListEntry != NULL 
			&& pWListEntry != &((*(PSERIAL_DEVICE_EXTENSION)(SrlDevice[0]-> DeviceExtension)).RecvBufferNotEmpty.WaitListHead) 
			&& WaitThreadNum < MAXSERIALEVENTWAITCOUNT;
			pWListEntry = pWListEntry->Next, WaitThreadNum++)
			{
				//
				// 获得线程对象的指针
				//
				pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
				
				COM1SendEvent.WaitObj[WaitThreadNum] = ObGetObjectId(pThread);
			}
			COM1RecvEvent.Count = WaitThreadNum;
		}
		
	}
	
			
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;	
}

#endif
