/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: psinit.c

����: ���̹���ģ��ĳ�ʼ����



*******************************************************************************/

#include "psp.h"

VOID
PspCreateProcessObjectType(
	VOID
	);

VOID
PspCreateEventObjectType(
	VOID
	);

VOID
PspCreateMutexObjectType(
	VOID
	);

VOID
PspCreateSemaphoreObjectType(
	VOID
	);

VOID
PspInitSchedulingQueue(
	VOID
	);

VOID
PsInitializeSystem1(
	VOID
	)
/*++

����������
	��ʼ�����̹���ģ�顣

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	//
	// �������̺��̶߳������͡�
	//
	PspCreateProcessObjectType();

	//
	// �����¼��������͡�
	//
	PspCreateEventObjectType();

	//
	// ���������ź����������͡�
	//
	PspCreateMutexObjectType();

	//
	// ������¼���ź����������͡�
	PspCreateSemaphoreObjectType();

	//
	// ��ʼ���̵߳���Ҫʹ�õĸ��ֶ��С�
	//
	PspInitSchedulingQueue();
}

VOID
PsInitializeSystem2(
	VOID
	)
{

}
