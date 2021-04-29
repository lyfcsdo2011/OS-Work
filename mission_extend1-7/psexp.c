/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: psexp.c

����: ���̣��̣߳��쳣����ģ�顣



*******************************************************************************/

#include "psp.h"

VOID
PsHandleException(
	ULONG ExceptionNumber,
	ULONG ErrorCode,
	PVOID Context
	)
/*++

����������
	�����������ʱ�������쳣��
	Ŀǰ����̫�ദ�������ϵͳ���̲����쳣��ֱ������������ֱ�ӽ����û����̡�

������
	ExceptionNumber -- �쳣�š�
	ErrorCode -- �쳣�����롣
	Context -- �쳣�������Ļ�����

����ֵ��
	�ޡ�

--*/
{
	PCONTEXT Cpu = (PCONTEXT)Context;

	ASSERT(KeGetIntNesting() == 1);

	if (PspCurrentProcess == PspSystemProcess) {

		KeBugCheck( "System process error!\n\
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
					Cpu->Eax, Cpu->Ebx, Cpu->Ecx,
					Cpu->Edx, Cpu->Esi, Cpu->Edi,
					Cpu->Esp, Cpu->Ebp, Cpu->Eip,
					Cpu->EFlag,
					Cpu->SegCs, Cpu->SegSs, Cpu->SegDs,
					Cpu->SegEs, Cpu->SegFs, Cpu->SegGs );
	}

	PspTerminateProcess(PspCurrentProcess, -1);
}
