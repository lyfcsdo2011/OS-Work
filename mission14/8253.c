/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: 8253.c

����: PC �� 8253 �ɱ�̶�ʱ������ (Programmable Interval Timer) �ĳ�ʼ����



*******************************************************************************/

#include "ki.h"

VOID 
KiInitializePit(
	VOID
	)
{
	//
	// ��ʼ�� 8253 ÿ�����ж� 100 �Ρ�
	//
	WRITE_PORT_UCHAR((PUCHAR)0x43, 0x34);
	WRITE_PORT_UCHAR((PUCHAR)0x40, (UCHAR)(11932 & 0xFF));
	WRITE_PORT_UCHAR((PUCHAR)0x40, (UCHAR)((11932 >> 8) & 0xFF));

	//
	// �� 8253 �жϡ�
	//
	KeEnableDeviceInterrupt(INT_TIMER, TRUE);
}
