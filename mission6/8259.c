/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: 8259.c

����: PC �� 8259 �ɱ���жϿ����� (Programmable Interrupt Controller) �Ŀ��ơ�



*******************************************************************************/

#include "ki.h"

//
// 8259 �ɱ���жϿ����� (PIC) �ļĴ�����ַ
//
#define PIC1_PORT0				0x20         // master PIC
#define PIC1_PORT1				0x21
#define PIC2_PORT0				0x0A0        // slave PIC
#define PIC2_PORT1				0x0A1

//
// 8259 �ɱ���жϿ����� (PIC) ������
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

����������
	��ʼ��8259�ɱ���жϿ�������

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	ASSERT((PIC1_VECTOR & 0x07) == 0);
	ASSERT((PIC2_VECTOR & 0x07) == 0);

	//
	// ��ʼ�� PIC
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
	// �ر������豸���ж�
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

����������
	���λ�����ָ���ⲿ�豸���ж�����

������
	IntVector -- �ⲿ�豸���ж������š�
	Enable -- TRUE �������ж�����FALSE �������ж�����

����ֵ��
	�ޡ�

--*/
{
	PUCHAR Port;
	UCHAR OCW1;

	ASSERT(PIC1_VECTOR == (IntVector & ~0x07) ||
		PIC2_VECTOR == (IntVector & ~0x07));

	//
	// �����豸�жϺ�ѡ����Ӧ�� PIC �Ķ˿ڵ�ַ��
	//
	if (PIC1_VECTOR == (IntVector & ~0x07)) {
		Port = (PUCHAR)PIC1_PORT1;
	} else if (PIC2_VECTOR == (IntVector & ~0x07)) {
		Port = (PUCHAR)PIC2_PORT1;
	}

	//
	// ��ȡ OCW1 ״̬�֡�
	//
	OCW1 = READ_PORT_UCHAR(Port);

	//
	// ���ݿ��ض���Ҫ���޸� OCW1 ��Ӧ��״̬λ��
	//
	if (Enable) {
		OCW1 = OCW1 & ~(1 << (IntVector & 0x07));
	} else {
		OCW1 = OCW1 | (1 << (IntVector & 0x07));
	}

	//
	// ��д OCW1 ״̬��
	//
	WRITE_PORT_UCHAR(Port, OCW1);
}

VOID
Ki8259EOI(
	VOID
	)
/*++

����������
	�� 8259 �����жϽ������

������
	�ޡ�

����ֵ��
	�ޡ�

���棺
	��Ҫ�ڴ˺����в���ϵ㡣��ʹ���Ե��˺����У�Ҳ��Ҫʹ�á�����̡�
	���ߡ��������ȵ������Թ��ܣ���Щ�������������Ԥ��ĵ��Խ����
	����ʹ�á��������ԡ����ܼ������ԡ�

--*/
{
	WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT0, 0x20);
}
