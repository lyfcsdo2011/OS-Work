/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: pscxt.c

����: �߳������Ļ����ĳ�ʼ�������������Ӳ����



*******************************************************************************/

#include "psp.h"

//
// �߳����������������̶߳������￪ʼִ�С�
//
VOID
PspThreadStartup(
	VOID
	);

VOID
PspInitializeThreadContext(
	IN PTHREAD Thread
	)
/*++

����������
	��ʼ���̵߳������Ļ�������Ӳ��ƽ̨��أ���

������
	Thread -- �߳̿��ƿ�ָ�롣

����ֵ��
	�ޡ�

--*/
{
	PCONTEXT cxt;

	cxt = &Thread->KernelContext;
	
	//
	// ��ʼ������ͨ�üĴ���Ϊ0
	//
	cxt->Eax = 0;
	cxt->Ebx = 0;
	cxt->Ecx = 0;
	cxt->Edx = 0;
	cxt->Edi = 0;
	cxt->Esi = 0;

	//
	// ���öμĴ���
	//
	cxt->SegCs = KeCodeSegmentSelector;
	cxt->SegDs = KeDataSegmentSelector;
	cxt->SegEs = KeDataSegmentSelector;
	cxt->SegFs = KeDataSegmentSelector;
	cxt->SegGs = KeDataSegmentSelector;
	cxt->SegSs = KeDataSegmentSelector;

	//
	// �����̶߳����߳��������� PspThreadStartup ��ʼִ�С�
	//
	cxt->Eip = (ULONG)PspThreadStartup;

	//
	// ��ʼ���ں�ģʽջ��
	// x86 CPU ��ջ���ݼ�����ջ�ĵײ��ڶ�ջ�ռ�ĸߵ�ַ�ˡ�
	//
	cxt->Esp = (ULONG_PTR)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(ULONG); 
	*(PULONG)cxt->Esp = 0;	// PspThreadStartup ���ص�ַΪ 0��
	cxt->Ebp = 0;			// PspThreadStartup û��������֡��

	//
	// ��ʼ������״̬�֣�����������һλ�������жϡ�
	//
	cxt->EFlag = PSW_MASK_IF;
}
