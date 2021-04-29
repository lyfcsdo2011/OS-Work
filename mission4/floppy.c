/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: floppy.c

����: EOS ������������
	  Ϊ�˴������׶�����֧��һ�� 1.44M ��������������



*******************************************************************************/

#include "iop.h"

//
// ���̿������Ĵ����˿ڶ��塣
//
#define DOR				0x03f2	// ��������Ĵ�����
#define FDC_STATUS		0x03f4	// FDC��״̬�Ĵ�����
#define FDC_DATA		0x03f5	// FDC���ݼĴ�����
#define DIR				0x03f7	// ��������Ĵ�����
#define DCR				0x03f7	// ���̿��ƼĴ�����

//
// ��������Ĵ���λ���塣
//
#define DOR_MOT_EN3		0x80	// ��������D��1-������0-�رա�
#define DOR_MOT_EN2		0x40	// ��������C��1-������0-�رա�
#define DOR_MOT_EN1		0x20	// ��������B��1-������0-�رա�
#define DOR_MOT_EN0		0x10	// ��������A��1-������0-�رա�
#define DOR_DMA_INT		0x08	// 1-����DMA�ж�����0-��ֹDMA�ж�����
#define DOR_RESET		0x04	// 1-�������̿�����FDC������0-��λFDC��

//
// ��״̬�Ĵ������塣
//
#define	MSR_RQM			0x80	// FDC_DATA������
#define MSR_DIO			0x40	// ���䷽��1-FDC->CPU��0-CPU->FDC��
#define MSR_NDMA		0x20	// ��DMA��ʽ��
#define MSR_CB			0x10	// ������æ��
#define MSR_DDB			0x08	// ����Dæ��
#define MSR_DCB			0x04	// ����Cæ��
#define MSR_DBB			0x02	// ����Bæ��
#define MSR_DAB			0x01	// ����Aæ��

//
// ���̿���������塣
//
#define FD_RECALIBRATE	0x07	// ����У�����
#define FD_SEEK			0x0F	// Ѱ�����
#define FD_READ			0xE6	// ���������
#define FD_WRITE		0xC5	// д�������
#define FD_SENSE		0x08	// ����ж�״̬���
#define FD_SPECIFY		0x03	// �趨�������������
#define FD_CONFIGURE	0x13	// �������

//
// ������������״̬��
//
typedef enum _MOTOR_STATE {
	ON,		// 0������
	DELAY,	// 1����ʱ�أ���Ȼ���ţ���
	OFF		// 2���ء�
} MOTOR_STATE;

//
// ��ǰ��������״̬��
//
PRIVATE UCHAR MotorState = OFF;

//
// ������ʱ�ر��������ļ�ʱ����
//
PRIVATE KTIMER DelayedOffTimer;

//
// �����ж��¼���
//
PRIVATE EVENT InterruptEvent;


PRIVATE VOID
FloppyIsr(
	VOID
	)
/*++

����������
	�����жϷ�����򡣽������������ж��¼�Ϊ��Ч���Ӷ�֪ͨ���������߳��жϵ��

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	PsSetEvent(&InterruptEvent);
}

PRIVATE VOID
FloppyTurnOnMotor(
	VOID
	)
/*++

����������
	����������

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;

	//
	// ����������ʱ�ر����ļ�ʱ�����⣬��Ҫ�ر��жϡ�
	//
	IntState = KeEnableInterrupts(FALSE);

	if (DELAY == MotorState) {

		//
		// ����������ڱ���ʱ�رգ���������ת������ע���ӳٹرռ�ʱ�����ɡ�
		//
		KeUnregisterTimer(&DelayedOffTimer);

	} else if (OFF == MotorState) {

		//
		// ��ﴦ�ڹر�״̬����������A����
		//
		WRITE_PORT_UCHAR((PUCHAR)DOR, DOR_MOT_EN0 | DOR_DMA_INT | DOR_RESET);

		//
		// �����������300-500ms����ܴﵽ�ȶ�ת�٣�˯�ߵȴ�֮��
		//
		PsSleep(500);
	}

	//
	// �޸��������״̬��
	//
	MotorState = ON;

	//
	// �ָ��ж�״̬��
	//
	KeEnableInterrupts(IntState);
}

PRIVATE VOID
DelayOffTimerRoutine(
	IN ULONG_PTR Parameter
	)
/*++

����������
	�ӳٹر����ļ�ʱ�������������ر���

������
	Parameter -- ���á�

����ֵ��
	�ޡ�

--*/
{
	ASSERT(DELAY == MotorState);

	//
	// �ر��������޸�״̬��ע����ʱ����
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

����������
	�ر�������

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;

	//
	// ����������ʱ�ر����ļ�ʱ�����⣬��Ҫ�ر��жϡ�
	//
	IntState = KeEnableInterrupts(FALSE);

	if (ON == MotorState) {

		//
		// Ϊ�˱���Ƶ���������������̹ر��������3���ڲ���ʹ��������������
		// ��Ž����رա�����Ҫ���Ľ�����ע��һ���ӳ�3��ر����ļ�ʱ����
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

����������
	д���ݵ�FDC_DATA�Ĵ�����

������
	Buffer - ���ݻ�����ָ�롣
	BytesToWrite - ����д���ֽ�����

����ֵ��
	����ɹ��򷵻�TRUE�����򷵻�FALSE��

--*/
{
	UCHAR i;
	UCHAR ErrorCount;
	UCHAR Msr;

	for (i = 0; i < BytesToWrite; i++) {

		for (ErrorCount = 0;;) {

			//
			// ��ȡFDC_STATUS��״̬�Ĵ�����
			//
			Msr = READ_PORT_UCHAR((PUCHAR)FDC_STATUS);

			if ((Msr & MSR_RQM) != 0) {

				//
				// FDC_DATA������������䷽��ΪCPU->FDC�����дFDC_DATA�ˣ�����
				// ����ʧ�ܡ�
				//
				if ((Msr & MSR_DIO) == 0) {
					WRITE_PORT_UCHAR((PUCHAR)FDC_DATA, Buffer[i]);
					break;
				} else {
					return FALSE;
				}

			} else {

				//
				// FDCδ�����������ͬһ�ֽ����Դﵽ3���򷵻�ʧ�ܣ�����ȴ����ԡ�
				//
				if (3 == ++ErrorCount) {
					return FALSE;
				}

				//
				// ˯�ߵȴ�һС���zZZ^
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

����������
	��ȡFDCִ������Ľ���ֽ����С�

������
	Buffer - ���������ȡ����Ļ�������ָ�롣
	BytesToRead - ������ȡ���ֽ�����

����ֵ��
	����ɹ���TRUE�����򷵻�FALSE��

--*/
{
	UCHAR i;
	UCHAR ErrorCount; 
	UCHAR Msr;

	ASSERT(BytesToRead > 0);

	for (i = 0;;) {

		for (ErrorCount = 0;;) {

			//
			// ��ȡFDC_STATUS��״̬�Ĵ�����
			//
			Msr = READ_PORT_UCHAR((PUCHAR)FDC_STATUS);

			if ((Msr & MSR_RQM) != 0) {

				//
				// FDC_DATA������������䷽��ΪFDC->CPU����Զ�ȡFDC_DATA�ˣ���
				// ��˵�����ݶ�ȡ��ϡ�
				//
				if ((Msr & MSR_DIO) != 0) {

					//
					// ������ȡFDC_DATA��������ݳ��ȳ���������ȡ�ĳ����򷵻�ʧ�ܡ�
					//
					if (i == BytesToRead) {
						return FALSE;
					}

					Buffer[i++] = READ_PORT_UCHAR((PUCHAR)FDC_DATA);

					//
					// ������ȡ��һ�ֽڡ�
					//
					break;

				} else {

					//
					// ���ݶ�ȡ��ϣ������ȡ���ȴﵽ������ȡ�����򷵻�TRUE��
					//
					return (i == BytesToRead);
				}

			} else {

				//
				// FDCδ�����������ͬһ�ֽ����Դﵽ3���򷵻�ʧ�ܣ�����ȴ����ԡ�
				//
				if (3 == ++ErrorCount) {
					return FALSE;
				}

				//
				// ˯�ߵȴ�һС���zZZ^
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

����������
	��λ������������

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	UCHAR DataBuffer[4];

	for(;;) {

		//
		// ʹDOR��RESETλΪ0������һС�����Ȼ���ٻָ�Ϊ1�����ɸ�λ��������
		//
		WRITE_PORT_UCHAR((PUCHAR)DOR, DOR_MOT_EN0 | DOR_DMA_INT);

		PsSleep(10);

		WRITE_PORT_UCHAR((PUCHAR)DOR, DOR_MOT_EN0 | DOR_DMA_INT | DOR_RESET);

		//
		// ������������λ��ɺ�ᴥ��һ���жϣ��ȴ���λ�жϵĵ�����
		//
		PsWaitForEvent(&InterruptEvent, INFINITE);

		//
		// ����ж�״̬��ȷ���Ǹ�λ�жϡ�
		//
		DataBuffer[0] = FD_SENSE;

		if (!FloppyWriteFdc(DataBuffer, 1) ||
			!FloppyReadFdc(DataBuffer, 2) ||
			0xC0 != DataBuffer[0] ) {
			continue;
		}

		//
		// �趨1.44M�����������Ĳ�����
		//
		DataBuffer[0] = FD_SPECIFY;
		DataBuffer[1] = 0xDF;
		DataBuffer[2] = 8 << 1;

		if (!FloppyWriteFdc(DataBuffer, 3)) {
			continue;
		}

		//
		// ��������Ѱ����
		//
		DataBuffer[0] = FD_CONFIGURE;
		DataBuffer[1] = 0;
		DataBuffer[2] = 0x60;
		DataBuffer[3] = 0;

		if (!FloppyWriteFdc(DataBuffer, 4)) {
			continue;
		}

		//
		// ���ô������ʣ�1.44M�����������Ĵ���������500kb/s��
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

����������
	У����ͷλ�ã�����ͷǿ�ƹ�λ��0�ŵ���

������
	�ޡ�

����ֵ��
	����ɹ��򷵻�TRUE��

--*/
{
	UCHAR DataBuffer[2];

	for (;;) {

		//
		// ��������0����У����
		//
		DataBuffer[0] = FD_RECALIBRATE;
		DataBuffer[1] = 0;

		if (FloppyWriteFdc(DataBuffer, 2)) {

			//
			// �ɹ���������У������ȴ�У���жϡ�
			//
			PsWaitForEvent(&InterruptEvent, INFINITE);

			//
			// ����ж�״̬��ȷ����У���жϡ�
			//
			DataBuffer[0] = FD_SENSE;

			if (FloppyWriteFdc(DataBuffer, 1) &&
				FloppyReadFdc(DataBuffer, 2) &&
				(DataBuffer[0] & 0x20) != 0) {

				break;
			}
		}

		//
		// FDC�߼����󣬸�λ����������Ȼ������У����
		//
		FloppyResetFDC();
	}

	//
	// ���������������ͷӦ��λ��0�ŵ���
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

����������
	��д����������

������
	Buffer -- �������������ַ��
	StartingSector -- ��д����ʼ������
	SectorsToRW -- ������д������������
	IsRead -- TRUE:��������FALSE:д������

����ֵ��
	����ɹ��򷵻�STATUS_SUCESS��

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
	// DMAֻ��Ѱַ1M�������ڴ档
	//
	if (!EOS_SUCCESS(Status) || (ULONG_PTR)BufferPhysicalAdress >= 0x100000) {
		return STATUS_INVALID_ADDRESS;
	}

	//
	// ����IO��ʼ������CHS��ַ���ֽ�����
	//
	Cylinder = (StartingSector / 18) / 2;
	Head = (StartingSector / 18) % 2;
	Sector = (StartingSector % 18) + 1;
	BytesToRW = SectorsToRW * 512;

	//
	// ����������
	//
	FloppyTurnOnMotor();

	//
	// ִ��IO���������IO������У����ͷλ�ú����ԣ��������6�Ρ�
	//
	Status = STATUS_FLOPPY_UNKNOWN_ERROR;

	for (ErrorCount = 0; ErrorCount < 6;) {

		//
		// ��ʼ��DMAͨ��2���ر��жϷ�ֹ���š�
		//
		IntState = KeEnableInterrupts(FALSE);

		//
		// ����ͨ��2����
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0A, 0x06);

		//
		// �����/�ʹ�������
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0C, 0x06);

		//
		// д��ʽ�����֣�����ģʽΪ���ֽڵ�����/д���ͣ���ֹ�Զ�Ԥ�á�
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0B, IsRead ? 0x46 : 0x4A);

		//
		// �ֱ�д��8λ�͸�8λ��ַ�Ĵ�����
		//
		WRITE_PORT_UCHAR((PUCHAR)0x04, (UCHAR)((ULONG_PTR)BufferPhysicalAdress));
		WRITE_PORT_UCHAR((PUCHAR)0x04, (UCHAR)((ULONG_PTR)BufferPhysicalAdress >> 8));

		//
		// д��ַ��16-19λ��ҳ��Ĵ�����
		//
		WRITE_PORT_UCHAR((PUCHAR)0x81, (UCHAR)((ULONG_PTR)Buffer >> 16));

		//
		// �ֱ�д��8λ�͸�8λ�����ֽ����Ĵ�����
		//
		WRITE_PORT_UCHAR((PUCHAR)0x05, (UCHAR)(BytesToRW - 1));
		WRITE_PORT_UCHAR((PUCHAR)0x05, (UCHAR)((BytesToRW - 1) >> 8));

		//
		// ����ͨ��2����
		//
		WRITE_PORT_UCHAR((PUCHAR)0x0A, 0x02);

		//
		// DMA��ʼ����ɣ��ָ��жϡ�
		//
		KeEnableInterrupts(IntState);

		//
		// ��FDC����/д���
		//
		DataBuffer[0] = IsRead ? FD_READ : FD_WRITE;	// �����ֽ�
		DataBuffer[1] = Head << 2;						// ��ͷ�� �� ��������
		DataBuffer[2] = Cylinder;						// ����
		DataBuffer[3] = Head;							// ��ͷ
		DataBuffer[4] = Sector;							// ����
		DataBuffer[5] = 2;								// ������С��512Bytes)
		DataBuffer[6] = 18;								// ÿ�ŵ�������
		DataBuffer[7] = 0x1B;							// ����֮��ļ������
		DataBuffer[8] = 0xFF;							// ����

		if (!FloppyWriteFdc(DataBuffer, 9)) {						

			//
			// �������߼�������λ��������
			//
			FloppyResetFDC();
			continue;
		}

		//
		// �ȴ���д����жϡ�
		//
		PsWaitForEvent(&InterruptEvent, INFINITE);

		//
		// ��ȡ��/д�����ִ�н����
		//
		if (!FloppyReadFdc(DataBuffer, 7)) {

			//
			// �������߼�������λ��������
			//
			FloppyResetFDC();
			continue;
		}

		if ((DataBuffer[0] >> 6) != 0) {

			//
			// �����쳣������
			//
			ASSERT(0x01 == (DataBuffer[0] >> 6));

			if ((DataBuffer[1] & 0x02) != 0) {

				//
				// ����д��������ֹд������
				//
				Status = STATUS_ACCESS_DENIED;
				break;
			}

			//
			// ���̶�д�������Ӵ����������У����ͷλ�ú����ԡ�
			//
			ErrorCount++;
			FloppyRecalibrate();
			continue;
		}

		Status = STATUS_SUCCESS;
		break;
	}

	//
	// �ر���
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

����������
	���������Ķ���ǲ���̣���ȡָ����һ��������ָ���Ļ������С�

������
	�ο� iop.h �ж��������� Read ���ܺ����Ľ��ܡ�

����ֵ��
	����ɹ��򷵻� STATUS_SUCCESS��

--*/
{
	//
	// 1.44M ����ֻ�� 2880 ��������
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

����������
	����������д��ǲ���̣�д�������е����ݵ�ָ����һ�������С�

������
	�ο� iop.h �ж��������� Write ���ܺ����Ľ��ܡ�

����ֵ��
	����ɹ��򷵻� STATUS_SUCCESS��

--*/
{
	//
	// 1.44M ����ֻ�� 2880 ��������
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
	// ����û���²��豸��Ŀǰ���������֧��һ�������豸��
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
		// ��ʼ���ж��¼���
		//
		PsInitializeEvent(&InterruptEvent, FALSE, FALSE);

		//
		// �����ж�������
		//
		KeIsrFloppy = FloppyIsr;

		//
		// �����豸�жϡ�
		//
		KeEnableDeviceInterrupt(INT_FLOPPY, TRUE);

		//
		// ��λ������������
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
	// ���̲�֧��ֱ�ӱ���д������ͨ������֮�ϵ��ļ�ϵͳ���ʴ��̣����Դ�����������
	// ��Create��Close���ܺ�����
	//
	DriverObject->Read = FloppyRead;
	DriverObject->Write = FloppyWrite;
}
