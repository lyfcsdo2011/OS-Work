/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: psstart.c

����: �̡߳��û����̵�����������



*******************************************************************************/

#include "psp.h"

VOID
PspThreadStartup(
	VOID
	)
/*++

����������
	�߳����������������̶߳������￪ʼִ�С�

������
	��

����ֵ��
	��

--*/
{
	ULONG ExitCode;
	
	//
	// ���õ�ǰ�̵߳��̺߳�����
	//
	ExitCode = PspCurrentThread->StartAddr(PspCurrentThread->Parameter);

	//
	// �˳���ǰ�̡߳�
	//
	PsExitThread(ExitCode);

	//
	// ��������Զ�����ء�
	//
	ASSERT(FALSE);
}

ULONG
PspProcessStartup(
	PVOID Parameter
	)
/*++

����������
	�û����̵����̺߳�����

������
	Parameter -- ���á�

����ֵ��
	��������±�������Զ���᷵�أ���Ϊ�û����̵���ں���Ӧ����crt�е�����������
	��main����������crtӦ�õ���ExitProcess�������̡�

--*/
{
	//
	// ���ý���ӳ�����ڵ�ַ��
	//
	PspCurrentProcess->ImageEntry();

	return -1;
}
