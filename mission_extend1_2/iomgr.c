/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: iomgr.c

����: �������������豸����Ĺ���Ŀǰ��ʵ��������������豸����Ĵ�����



*******************************************************************************/

#include "iop.h"

//
// I/O�������õ��ĸ��ֶ������͡�
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

����������
	���������������

������
	DriverName - ������������ơ�
	DriverObject - ָ�룬ָ�����ڱ����½������������ָ��ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

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
		return STATUS_OBJECT_NAME_COLLISION; // �ڴ�����������ʱ������������
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

����������
	�����豸����

������
	DriverObject - �����������ָ�롣
	DeviceExtensionSize - �豸������չ�ṹ�ߴ硣
	DeviceName - �豸�����ַ�����
	IsBlockDevice - �Ƿ��ǿ��豸��
	DeviceNumber - �豸��ţ�������ο�DEVICE_OJECT�ṹ�塣
	DeviceObject - ��������ɹ��������豸����ָ�롣

����ֵ��
	�ɹ����� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PDEVICE_OBJECT NewDevice;

	ASSERT(NULL != DriverObject);

	//
	// ����һ���豸����
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
		return STATUS_OBJECT_NAME_COLLISION; // �ڴ�����������ʱ������������
	}

	//
	// �����豸��չ�ṹָ�롣
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
	// ���豸�����������������豸�����С�
	//
	ListInsertTail(&DriverObject->DeviceListHead, &NewDevice->DeviceListEntry);

	*DeviceObject = NewDevice;
	return STATUS_SUCCESS;
}
