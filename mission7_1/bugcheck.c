/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: bugcheck.c

����: PC �� KeBugCheck ��ʵ�֣�KeBugCheck ����Ӳ��ƽ̨��



*******************************************************************************/

#include "ki.h"

//
// VGA����λ�ã����ڴ�ӳ����ء�
//
#define VGA_BUFFER				0x800B8000

VOID
KeBugCheck(
	IN PCSTR Format,
	...
	)
{
	PCHAR DestPtr, SrcPtr;
	PULONG Pos;
	va_list argptr;

	//
	// �ر��жϡ�
	//
	KeEnableInterrupts(FALSE);

	//
	// �ӵ�0ҳ��ʼ��ʾ��
	//
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0C);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0x00);
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0D);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0x00);

	//
	// ���ع�꣨�ŵ��ڶ�ҳ����
	//
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0E);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0x07);
	WRITE_PORT_UCHAR((PUCHAR)0x03D4, (UCHAR)0x0F);
	WRITE_PORT_UCHAR((PUCHAR)0x03D5, (UCHAR)0xD0);

	//
	// ˢ��һҳ�Ļ�����Ϊ���װ��֡�
	//
	for(Pos = (PULONG)VGA_BUFFER; Pos < (PULONG)VGA_BUFFER + 1000; Pos++) {
		*Pos = 0x1F001F00;
	}

	//
	// ��ʽ���ַ������õڶ�ҳ��Ƶ��������Ϊ��ʽ�������������
	//
	va_start(argptr, Format);
	vsprintf((PCHAR)VGA_BUFFER + 4000, Format, argptr);

	//
	// ��ʾ���⡣
	//
	DestPtr = (PCHAR)VGA_BUFFER;
	SrcPtr = "This Message is print by KeBugCheck().";
	while(*SrcPtr != 0)
	{
		*DestPtr = *SrcPtr++;
		DestPtr += 2;
	}

	//
	// ��ʾ���ġ�
	//
	DestPtr = (PCHAR)VGA_BUFFER + 160;
	for (SrcPtr = (PCHAR)VGA_BUFFER + 4000; *SrcPtr!=0; SrcPtr++)
	{
		if ('\n' == *SrcPtr)
		{
			DestPtr = DestPtr - (DestPtr - (PCHAR)VGA_BUFFER) % 160 + 80;
		}
		else if ('\t' == *SrcPtr)
		{
			DestPtr = (PCHAR)(((ULONG_PTR)DestPtr + 0x10) & ~0xF);
		}
		else
		{
			*DestPtr = *SrcPtr;
			DestPtr += 2;
		}
	}

	/* ��ѭ����*/
	while(1);
}
