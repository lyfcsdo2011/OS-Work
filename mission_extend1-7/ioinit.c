/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: ioinit.c

����: I/Oģ��ĳ�ʼ�����������ݽṹ��ĳ�ʼ�����豸�����ļ��ء�



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

����������
	I/O��������һ����ʼ������ʼ��������������Ҫ�Ǵ���IO�������͡�

������
	�ޡ�

����ֵ��
	�ޡ�

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
	// ������������������͡�
	//
	Status = ObCreateObjectType("DRIVER", &Initializer, &IopDriverObjectType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create driver object type!");
	}

	//
	// �����豸�������͡�
	//
	Status = ObCreateObjectType("DEVICE", &Initializer, &IopDeviceObjectType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create device object type!");
	}

	//
	// ��������̨�������͡�
	//
	Initializer.Read = (OB_READ_METHOD)IopReadConsoleInput;
	Initializer.Write = (OB_WRITE_METHOD)IopWriteConsoleOutput;

	Status = ObCreateObjectType("CONSOLE", &Initializer, &IopConsoleType);

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create console object type!");
	}

	//
	// �����ļ��������͡�
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

����������
	I/O�������ڶ�����ʼ������������������װ��������ʼ���豸��

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	STATUS Status;
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;

	//
	// ���豸��ĳ�ʼ���������ڸ��ֿ��豸�����������������ĳ�ʼ��֮ǰ��ɡ�
	//
	IopInitializeBlockDeviceLayer();

	//
	// EOSΪ�˱��ּ򵥣���֧���Զ����Ӳ�����ò���װ�����Ĺ��ܣ�ֻ��ͨ��Ӳ�����
	// ��ʽ��װ������������豸����
	// ���⣬Ϊ�˷������������������������ں˱�����һ��
	//

	//
	// �������������������
	//
	Status = IopCreateDriver("Floppy Driver", &DriverObject);
	ASSERT(EOS_SUCCESS(Status));

	if (EOS_SUCCESS(Status)) {

		//
		// ��ʼ�����������������
		//
		FloppyInitializeDriver(DriverObject);

		//
		// ��������豸����Ŀǰ��֧��һ��������
		//
		Status = DriverObject->AddDevice(DriverObject, NULL, 0, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));
	}

	//
	// ��������豸����ɹ���ӣ���ô������֮�ϰ�װFAT12�ļ�ϵͳ��������
	//
	if (EOS_SUCCESS(Status)) {

		//
		// ����FAT12�ļ�ϵͳ�����������
		//
		Status = IopCreateDriver("FAT12 Driver", &DriverObject);
		ASSERT(EOS_SUCCESS(Status));

		if (EOS_SUCCESS(Status)) {

			//
			// ��ʼ��FAT12�ļ�ϵͳ�����������
			//
			FatInitializeDriver(DriverObject);

			//
			// �������豸����֮�����FAT12�ļ�ϵͳ�豸����
			//
			Status = DriverObject->AddDevice(DriverObject, DeviceObject, 0, &DeviceObject);
			ASSERT(EOS_SUCCESS(Status));
		}
	}

	//
	// �������������������
	//
	Status = IopCreateDriver("Keyboard Driver", &DriverObject);
	ASSERT(EOS_SUCCESS(Status));

	if (EOS_SUCCESS(Status)) {

		//
		// ��ʼ�����������������
		//
		KbdInitializeDriver(DriverObject);

		//
		// ��Ӽ����豸����
		//
		Status = DriverObject->AddDevice(DriverObject, NULL, 0, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));
	}

	//
	// �������������������
	//
	Status = IopCreateDriver("Serial Driver", &DriverObject);
	ASSERT(EOS_SUCCESS(Status));

	if (EOS_SUCCESS(Status)) {

		//
		// ��ʼ�����������������
		//
		SrlInitializeDriver(DriverObject);

		//
		// �ֱ���� COM1 �� COM2 ��Ӧ�Ĵ����豸����
		//
		Status = DriverObject->AddDevice(DriverObject, NULL, 0, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));

		Status = DriverObject->AddDevice(DriverObject, NULL, 1, &DeviceObject);
		ASSERT(EOS_SUCCESS(Status));
	}

	//
	// ��ʼ������̨I/Oģ�顣
	//
	IopInitializeConsole();
}
