/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: iomgr.c

描述: 驱动程序对象和设备对象的管理，目前仅实现了驱动对象和设备对象的创建。



*******************************************************************************/

#include "iop.h"

//
// I/O管理器用到的各种对象类型。
//
POBJECT_TYPE IopDriverObjectType;
POBJECT_TYPE IopDeviceObjectType;
POBJECT_TYPE IopFileObjectType;
POBJECT_TYPE IopConsoleType;

STATUS
IopCreateDriver(
	IN PCSTR DriverName,
	OUT PDRIVER_OBJECT *DriverObject
	)
/*++

功能描述：
	创建驱动程序对象。

参数：
	DriverName - 驱动程序的名称。
	DriverObject - 指针，指向用于保存新建驱动程序对象指针的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PDRIVER_OBJECT NewDriver;

	Status = ObCreateObject( IopDriverObjectType,
							 DriverName,
							 sizeof(DRIVER_OBJECT),
							 0,
							 (PVOID*)&NewDriver );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	} else if (STATUS_OBJECT_NAME_EXISTS == Status) {
		ObDerefObject(NewDriver);
		return STATUS_OBJECT_NAME_COLLISION; // 在创建驱动程序时不允许重名。
	}

	ListInitializeHead(&NewDriver->DeviceListHead);
	NewDriver->AddDevice = NULL;
	NewDriver->Create = NULL;
	NewDriver->Close = NULL;
	NewDriver->Read = NULL;
	NewDriver->Write = NULL;
	NewDriver->Query = NULL;
	NewDriver->Set = NULL;

	*DriverObject = NewDriver;
	return STATUS_SUCCESS;
}

STATUS
IopCreateDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN ULONG DeviceExtensionSize,
	IN PCSTR DeviceName OPTIONAL,
	IN USHORT DeviceNumber,
	IN BOOL IsBlockDevice,
	OUT PDEVICE_OBJECT *DeviceObject
	)
/*++

功能描述：
	创建设备对象。

参数：
	DriverObject - 驱动程序对象指针。
	DeviceExtensionSize - 设备对象扩展结构尺寸。
	DeviceName - 设备名称字符串。
	IsBlockDevice - 是否是块设备。
	DeviceNumber - 设备编号，具体请参看DEVICE_OJECT结构体。
	DeviceObject - 如果创建成功将返回设备对象指针。

返回值：
	成功返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PDEVICE_OBJECT NewDevice;

	ASSERT(NULL != DriverObject);

	//
	// 创建一个设备对象。
	//
	Status = ObCreateObject( IopDeviceObjectType,
							 DeviceName,
							 sizeof(DEVICE_OBJECT) + DeviceExtensionSize,
							 0,
							 (PVOID)&NewDevice );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	} else if (STATUS_OBJECT_NAME_EXISTS == Status) {
		ObDerefObject(NewDevice);
		return STATUS_OBJECT_NAME_COLLISION; // 在创建驱动程序时不允许重名。
	}

	//
	// 设置设备扩展结构指针。
	//
	if (DeviceExtensionSize != 0) {
		NewDevice->DeviceExtension = (PVOID)(NewDevice + 1);
		memset(NewDevice->DeviceExtension, 0, DeviceExtensionSize);
	} else {
		NewDevice->DeviceExtension = NULL;
	}

	NewDevice->IsBlockDevice = IsBlockDevice;
	NewDevice->DriverObject = DriverObject;
	NewDevice->DeviceNumber = DeviceNumber;
	NewDevice->OpenCount = 0;
	NewDevice->ShareRead = FALSE;
	NewDevice->ShareWrite = FALSE;
	PsInitializeMutex(&NewDevice->Mutex, FALSE);

	//
	// 将设备对象插入驱动程序的设备链表中。
	//
	ListInsertTail(&DriverObject->DeviceListHead, &NewDevice->DeviceListEntry);

	*DeviceObject = NewDevice;
	return STATUS_SUCCESS;
}
