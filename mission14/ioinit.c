/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: ioinit.c

描述: I/O模块的初始化，包括数据结构体的初始化和设备驱动的加载。



*******************************************************************************/

#include "iop.h"

STATUS
IopReadFileObject(
	IN PFILE_OBJECT File,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	);

STATUS
IopWriteFileObject(
	IN PFILE_OBJECT File,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);

VOID
IopCloseFileObject(
	IN PFILE_OBJECT FileObject
	);

STATUS
IopReadConsoleInput(
	IN PCONSOLE Console,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	);

STATUS
IopWriteConsoleOutput(
	IN PCONSOLE Console,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);

VOID
FloppyInitializeDriver(
	PDRIVER_OBJECT DriverObject
	);

VOID
FatInitializeDriver(
	PDRIVER_OBJECT DriverObject
	);

VOID
KbdInitializeDriver(
	PDRIVER_OBJECT DriverObject
	);

VOID
IopInitializeConsole(
	VOID
	);

VOID
SrlInitializeDriver(
	PDRIVER_OBJECT DriverObject
	);

VOID
IoInitializeSystem1(
	VOID
	)
/*++

功能描述：
	I/O管理器第一步初始化，初始化不可阻塞，主要是创建IO对象类型。

参数：
	无。

返回值：
	无。

--*/
{
	STATUS Status;
	OBJECT_TYPE_INITIALIZER Initializer;

	Initializer.Create = NULL;
	Initializer.Delete = NULL;
	Initializer.Wait = NULL;
	Initializer.Read = NULL;
	Initializer.Write = NULL;

	//
	// 创建驱动程序对象类型。
	//
	Status = ObCreateObjectType("DRIVER", &Initializer, &IopDriverObjectType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create driver object type!");
	}

	//
	// 创建设备对象类型。
	//
	Status = ObCreateObjectType("DEVICE", &Initializer, &IopDeviceObjectType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create device object type!");
	}

	//
	// 创建控制台对象类型。
	//
	Initializer.Read = (OB_READ_METHOD)IopReadConsoleInput;
	Initializer.Write = (OB_WRITE_METHOD)IopWriteConsoleOutput;

	Status = ObCreateObjectType("CONSOLE", &Initializer, &IopConsoleType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create console object type!");
	}

	//
	// 创建文件对象类型。
	//
	Initializer.Delete = (OB_DELETE_METHOD)IopCloseFileObject;
	Initializer.Read = (OB_READ_METHOD)IopReadFileObject;
	Initializer.Write = (OB_WRITE_METHOD)IopWriteFileObject;

	Status = ObCreateObjectType("FILE", &Initializer, &IopFileObjectType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create file object type!");
	}
}

VOID
IoInitializeSystem2(
	VOID
	)
/*++

功能描述：
	I/O管理器第二步初始化函数，可阻塞，安装驱动并初始化设备。

参数：
	无。

返回值：
	无。

--*/
{
	STATUS Status;
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;

	//
	// 块设备层的初始化，必须在各种块设备对象及其驱动程序对象的初始化之前完成。
	//
	IopInitializeBlockDeviceLayer();

	//
	// EOS为了保持简单，不支持自动侦测硬件配置并安装驱动的功能，只能通过硬编码的
	// 方式安装驱动程序并添加设备对象。
	// 另外，为了方便调试驱动程序，驱动程序和内核编译在一起。
	//

	//
	// 创建软驱驱动程序对象。
	//
	Status = IopCreateDriver("Floppy Driver", &DriverObject);
	ASSERT(EOS_SUCCESS(Status));

	if (EOS_SUCCESS(Status)) {

		//
		// 初始化软驱驱动程序对象。
		//
		FloppyInitializeDriver(DriverObject);

		//
		// 添加软驱设备对象，目前仅支持一个软驱。
		//
		Status = DriverObject->AddDevice(DriverObject, NULL, 0, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));
	}

	//
	// 如果软驱设备对象成功添加，那么在软驱之上安装FAT12文件系统驱动程序。
	//
	if (EOS_SUCCESS(Status)) {

		//
		// 创建FAT12文件系统驱动程序对象。
		//
		Status = IopCreateDriver("FAT12 Driver", &DriverObject);
		ASSERT(EOS_SUCCESS(Status));

		if (EOS_SUCCESS(Status)) {

			//
			// 初始化FAT12文件系统驱动程序对象。
			//
			FatInitializeDriver(DriverObject);

			//
			// 在软驱设备对象之上添加FAT12文件系统设备对象。
			//
			Status = DriverObject->AddDevice(DriverObject, DeviceObject, 0, &DeviceObject);
			ASSERT(EOS_SUCCESS(Status));
		}
	}

	//
	// 创建键盘驱动程序对象。
	//
	Status = IopCreateDriver("Keyboard Driver", &DriverObject);
	ASSERT(EOS_SUCCESS(Status));

	if (EOS_SUCCESS(Status)) {

		//
		// 初始化键盘驱动程序对象。
		//
		KbdInitializeDriver(DriverObject);

		//
		// 添加键盘设备对象。
		//
		Status = DriverObject->AddDevice(DriverObject, NULL, 0, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));
	}

	//
	// 创建串口驱动程序对象。
	//
	Status = IopCreateDriver("Serial Driver", &DriverObject);
	ASSERT(EOS_SUCCESS(Status));

	if (EOS_SUCCESS(Status)) {

		//
		// 初始化串口驱动程序对象。
		//
		SrlInitializeDriver(DriverObject);

		//
		// 分别添加 COM1 和 COM2 对应的串口设备对象。
		//
		Status = DriverObject->AddDevice(DriverObject, NULL, 0, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));

		Status = DriverObject->AddDevice(DriverObject, NULL, 1, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));
	}

	//
	// 初始化控制台I/O模块。
	//
	IopInitializeConsole();
}
