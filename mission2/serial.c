/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: serial.c

����: �������������ʵ�֡�
	  ʵ���˻����Ĵ���ͨ�ţ����ṩ�����������ú�ͨ�Ŵ�������Ϊ����ʾ��������
	  �������̡�



*******************************************************************************/

#include "iop.h"

//
// �����豸�������չ�顣
//
typedef struct _SERIAL_DEVICE_EXTENSION {
	PRING_BUFFER SendBuffer;		// ���ڷ������ݵĻ��λ�������
	EVENT CompletionEvent;			// ���ͻ��������¼��������ͻ�����Ϊ��ʱ����signaled״̬

	PRING_BUFFER RecvBuffer;		// �������ݻ�����
	EVENT RecvBufferNotEmpty;		// �����ջ������ǿ�ʱ����signaled״̬
	

}SERIAL_DEVICE_EXTENSION, *PSERIAL_DEVICE_EXTENSION;

//
// COM1��COM2��Ӧ���豸����ָ�룬���жϷ������ʹ�á�
//
PDEVICE_OBJECT SrlDevice[2] = {NULL, NULL};

//
// COM1��COM2�ļĴ����˿ڵ�ַ�Ļ�ַ��
//
const PUCHAR SrlRegPortBase[2] = {(PUCHAR)0x03F8, (PUCHAR)0x02F8};

//
// 8250�ļĴ����˿ڵ�ַ����(�����0x3F8��0x2F8��ƫ��)��
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
// ȡ�豸�����Ӧ��COM�˿ڵļĴ�����ַ��d-�豸����ָ�룬r-RBR��THR�ȡ�
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
// COM1��COM2���жϷ������
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
// ���������ʼ��������
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
// ����������AddDevice���ܺ����������ͷ���ֵ��ο�iop.hԴ�ļ���
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

	ASSERT(NULL == NextLayerDevice); // ����û���²��豸
	ASSERT(0 == DeviceNumber || 1 == DeviceNumber); // Ŀǰ��֧��COM1��COM2���������豸
	ASSERT(NULL == SrlDevice[DeviceNumber]);

	//
	// COM1��COM2���豸��ŷֱ�Ϊ0��1���豸���ֱ��ǡ�COM1������COM2����
	//
	DeviceName[3] += DeviceNumber;

	//
	// �������ڶ�Ӧ���豸����
	//
	Status = IopCreateDevice( DriverObject,				// ���������������ָ��
							  sizeof(SERIAL_DEVICE_EXTENSION),	// �豸��չ��Ĵ�С
							  DeviceName,				// �豸����COM1Ϊ��COM1����COM2Ϊ��COM2��
							  DeviceNumber,				// �豸��ţ�COM1Ϊ0��COM2Ϊ1
							  FALSE,					// ���ǿ��豸
							  &SerialDevice );

	if (EOS_SUCCESS(Status)) {

		SrlDevice[DeviceNumber] = SerialDevice;

		//
		// ��ʼ���豸��չ�顣
		//
		Ext = (PSERIAL_DEVICE_EXTENSION)SerialDevice->DeviceExtension;
		Ext->SendBuffer = IopCreateRingBuffer(256);				// ���л�������ʼ��Ϊ 256 ���ֽڡ�
		PsInitializeEvent(&Ext->CompletionEvent, FALSE, FALSE);	// ���ͻ��������¼���ʼ��Ϊ��Ч���Զ��¼���

		
		Ext->RecvBuffer = IopCreateRingBuffer(256);				// ���л�������ʼ��Ϊ 256 ���ֽڡ�
		PsInitializeEvent(&Ext->RecvBufferNotEmpty, TRUE, FALSE);	// ���ջ������ǿ��¼���ʼ��Ϊ��Ч���ֶ��¼���


		//
		// ����Ĭ��ͨ�Ų�����������57600��8������λ��һ��ֹͣλ������żУ�顣
		//
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, LCR), 0x80);	// дLCRʹDLAB = 1��Ȼ�����д�����Ĵ�����
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, DIVRL), 0x02);	// д�����Ĵ�����8λ��
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, DIVRH), 0);		// д�����Ĵ�����8λ�������Ĵ�����ֵΪ��1843200 / (57600 * 16) = 2��
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, LCR), 0x03);	// LCR = 00000011B��8����λ��1ֹͣλ������żУ�顣
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, MCR), 0x0B);	// OUT2����͵�ƽ�������ж��źŷ�����~DTR��~RTSΪ�ͣ�����������
		WRITE_PORT_UCHAR(REG_PORT(SerialDevice, IER), 0x03);	// �����ͺͽ����жϡ�

		//
		// �����жϷ���������ڵ�ַ��ȡ��8259�ɱ���жϿ�������COM�жϵ����Ρ�
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
// ����������Create�����������ͷ���ֵ��ο�iop.hԴ�ļ���
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
	// �����豸���������豸��·����Ӧ���ǿ��ַ�����
	//
	if (*FileName != '\0') {
		return STATUS_FILE_NOT_FOUND;
	}

	//
	// ֻ��ʹ��OPEN_EXISTING��־��
	//
	if (OPEN_EXISTING != CreationDisposition) {
		return STATUS_INVALID_PARAMETER;
	}

	if (0 == DeviceObject->OpenCount) {

		//
		// �����豸�ɹر�״̬���򿪣��ɸ�����Ҫ�ڴ���Ӵ��������Ӧ�Ĵ���
		//

	}

	//
	// ���豸�ɹ���ʹ�ļ������COM�豸���������
	//
	FileObject->FsContext = DeviceObject;
	return STATUS_SUCCESS;
}

//
// ����������CloseFile�����������ͷ���ֵ��ο�iop.hԴ�ļ���
//
VOID
SrlClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PFILE_OBJECT FileObject
	)
{
	if (0 == DeviceObject->OpenCount) {

		//
		// �豸�ɴ�״̬���رգ��ɸ�����Ҫ�ڴ���Ӵ��������Ӧ�Ĵ���
		//

	}
}

//
// ����������Read���ܺ����������ͷ���ֵ��ο�iop.hԴ�ļ���
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
	// �ڴ���Ӷ�ȡ�����豸�Ĵ��롣
	//
	return STATUS_NOT_SUPPORTED;
}

//
// ����������Write���ܺ����������ͷ���ֵ��ο�iop.hԴ�ļ���
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

	PsResetEvent(&Ext->CompletionEvent); // ��λ��������¼�Ϊ��Ч״̬��

	while (Count < Request) {

		//
		// ��¼Ҫ���͵ĵ�һ���ֽڣ�������Ҫ���͵�����д�뷢�ͻ�������
		// ע�⣬�������Ĵ�С���ܲ�����һ�α�������Ҫ���͵����ݣ�IopWriteRingBuffer
		// ������ʵ��д�뻺�������ֽ�����
		//
		Data = ((PCHAR)Buffer)[Count++];
		Count += IopWriteRingBuffer(Ext->SendBuffer, Buffer + Count, Request - Count);

		//
		// �������͹��̡�
		// �������͹��̽��轫��һ���ֽ�����д��THR��THR�е����ݱ��ͳ���ᴥ��һ��
		// THR���жϣ��жϴ���̳��򽫼������ͻ������е����ݡ������������ݱ�����
		// ����жϴ�����������CompletionEvent�¼�Ϊ��Ч��
		//
		WRITE_PORT_UCHAR(REG_PORT(DeviceObject, THR), Data);

		//
		// �����ȴ�ֱ�����ݷ�����ϡ�ע�⣬�ȴ��������غ󣬸��¼�������Զ���Ϊ��Ч״̬��
		//
		PsWaitForEvent(&Ext->CompletionEvent, INFINITE);
	}

	*Result = Count;
	return STATUS_SUCCESS;
}

//
// ���ڵ��жϷ������
//
VOID
SrlIsr(
   PDEVICE_OBJECT Device
   )
{
	CHAR Data;
	PSERIAL_DEVICE_EXTENSION Ext = (PSERIAL_DEVICE_EXTENSION)Device->DeviceExtension;

	//
	// �����ж�ʶ��Ĵ���ȷ���ж���Դ��������Ӧ�Ĵ���
	//
	if (2 == READ_PORT_UCHAR(REG_PORT(Device, IIR))) {

		//
		// THR�գ�������THRд����һ��Ҫ���͵��ֽ����ݡ�
		// ������ͻ�����Ϊ����˵�����͹��̽�����Ӧ���÷�������¼�Ϊ��Ч״̬��
		// ���򣬴ӷ��ͻ������ж�ȡһ���ֽ����ݲ�д��THR���з��͡�
		//
		if (IopIsRingBufferEmpty(Ext->SendBuffer)) {
			PsSetEvent(&Ext->CompletionEvent);
		} else {
			IopReadRingBuffer(Ext->SendBuffer, &Data, 1);
			WRITE_PORT_UCHAR(REG_PORT(Device, THR), Data);
		}

	} else  {

		//
		// RBR��������ȡRBR���õ����յ������ݡ�
		//
		Data = READ_PORT_UCHAR(REG_PORT(Device, RBR));

		//
		// �ڴ���Ӵ��룬��һ����������жϡ�
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

����������
	��ȡ����1����Ϣ��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	PTHREAD pThread;	
	PLIST_ENTRY pWListEntry;
	INT WaitThreadNum = 0;
	INT Size = 0, FillCount = 0;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
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
				// ����̶߳����ָ��
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
				// ����̶߳����ָ��
				//
				pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
				
				COM1SendEvent.WaitObj[WaitThreadNum] = ObGetObjectId(pThread);
			}
			COM1RecvEvent.Count = WaitThreadNum;
		}
		
	}
	
			
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;	
}

#endif
