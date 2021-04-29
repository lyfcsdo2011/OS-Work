/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: dispatch.c

����: Inerrupt dispatch module for the KE subcomponent of EOS



*******************************************************************************/

#include "ki.h"
#include "ps.h"

//
// �� 8259 ���� EOI ����
//
VOID
Ki8259EOI(
	VOID
	);

//
// �����жϴ�����ָ�롣
//
ISR KeIsrClock = NULL;
ISR KeIsrKeyBoard = NULL;
ISR KeIsrCom1 = NULL;
ISR KeIsrCom2 = NULL;
ISR KeIsrLPT1 = NULL;
ISR KeIsrLPT2 = NULL;
ISR KeIsrFloppy = NULL;
ISR KeIsrHD = NULL;
ISR KeIsrFPU = NULL;
ISR KeIsrPS2 = NULL;

VOID
KiDispatchInterrupt(
	ULONG IntNumber
	)
/*++

����������

	��ǲ�����жϸ��ʵ����жϴ������

������

	IntNumber - ��ǰ�ж������š�

����ֵ��

	�ޡ�

--*/
{
	//
	// ���������ŵ�����Ӧ���жϴ����������жϴ������ָ��ΪNULL�������֮��
	//
	switch (IntNumber)
	{
	case INT_TIMER:

		KiIsrTimer();

		break;
	case INT_KEYBOARD:

		if (NULL != KeIsrKeyBoard)
			KeIsrKeyBoard();

		break;
	case INT_COM2:

		if (NULL != KeIsrCom2)
			KeIsrCom2();

		break;
	case INT_COM1:

		if (NULL != KeIsrCom1)
			KeIsrCom1();

		break;
	case INT_LPT2:

		if (NULL != KeIsrLPT2)
			KeIsrLPT2();

		break;
	case INT_FLOPPY:

		if (NULL != KeIsrFloppy)
			KeIsrFloppy();

		break;
	case INT_LPT1:

		if (NULL != KeIsrLPT1)
			KeIsrLPT1();

		break;
	case INT_CLOCK:

		if (NULL != KeIsrClock)
			KeIsrClock();

		break;
	case INT_PS2:

		if (NULL != KeIsrPS2)
			KeIsrPS2();

		break;
	case INT_FPU:

		if (NULL != KeIsrFPU)
			KeIsrFPU();

		break;
	case INT_HD:

		if (NULL != KeIsrHD)
			KeIsrHD();

		break;
	default:

		ASSERT(FALSE);

		break;
	}

	//
	// ���� EOI ������ɱ���жϿ����� 8259��
	// ���棺
	//		��Ҫ�ڴ��д��봦����ϵ㡣��ʹ���Ե����д��룬Ҳ��Ҫʹ�á�����̡�
	//		�������ԣ��˲������������Ԥ��ĵ��Խ��������ʹ�á��������ԡ���
	//		�ܼ������ԡ�
	Ki8259EOI();
}



BOOL
KiDispatchException(
	ULONG ExceptionNumber,
	ULONG ErrorCode,
	PCONTEXT Context
	)
/*++

����������

	��ǲ�쳣���ʵ����쳣�������

������

	ExceptionNumber - �쳣�����š�
	ErrorCode - �쳣�����롣
	Context - �����쳣��CPU�����Ļ�����

����ֵ��

	FALSE�����쳣�����߳�ʱ��Ҫִ���̵߳��ȡ�
	TRUE�����쳣�����߳�ʱ��Ҫִ���̵߳��ȡ�

���棺
	
	�ڴ˺����е��κ�λ�ò���ϵ㶼����ɵ���ʧ�ܡ�
--*/
{
	//
	// �ǵ�������£�������жϷ������������쳣��ֱ�������������ɽ��̹�����
	// �����̲߳������쳣��
	// ע�⣺�ж���ȴ���0��˵����ǰִ�����жϻ����С������쳣����Ҳ��ִ�����ж�
	// �����еģ�����������жϷ����������쳣���Ƕ�׽����쳣�����жϷ������
	// ��ʱ�õ����ж����Ӧ����1��
	//
	if (KeGetIntNesting() > 1) {
		KeBugCheck( "Interrupt service routine error!\n\
					Exception number is %d.\n\
					Error code is %d.\n\
					Register value:\n\
					\tEAX:0x%.8X\tEBX:0x%.8X\tECX:0x%.8X\n\
					\tEDX:0x%.8X\tESI:0x%.8X\tEDI:0x%.8X\n\
					\tESP:0x%.8X\tEBP:0x%.8X\tEIP:0x%.8X\n\
					\tEFLAGS:0x%.8X\n\
					\tCS:0x%.4X\tSS:0x%.4X\tDS:0x%.4X\n\
					\tES:0x%.4X\tFS:0x%.4X\tGS:0x%.4X\n",
					ExceptionNumber, ErrorCode,
					Context->Eax, Context->Ebx, Context->Ecx,
					Context->Edx, Context->Esi, Context->Edi,
					Context->Esp, Context->Ebp, Context->Eip,
					Context->EFlag,
					Context->SegCs, Context->SegSs, Context->SegDs,
					Context->SegEs, Context->SegFs, Context->SegGs );
	} else {
		PsHandleException(ExceptionNumber, ErrorCode, Context);
	}

	return TRUE;
}
