/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: psspnd.c

����: �����̵߳ļ�ʵ�֡���ֻ�ܽ����ھ���״̬���̹߳��𣬻��߽�������߳�
      �ָ�Ϊ����״̬��



*******************************************************************************/

#include "psp.h"


//
// �����̶߳��е�ͷ���ѳ�ʼ��Ϊ�ն��У�
//
LIST_ENTRY SuspendListHead = {&SuspendListHead, &SuspendListHead};


STATUS
PsSuspendThread(
	IN HANDLE hThread
	)
/*++

����������
	����ָ�����̡߳�Ŀǰֻ�ܽ����ھ���״̬���̹߳���

������
	hThread - ��Ҫ��������̵߳ľ����

����ֵ��
	����ɹ��򷵻� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	PTHREAD Thread;

	//
	// �����߳̾������̶߳����ָ��
	//
	Status = ObRefObjectByHandle(hThread, PspThreadType, (PVOID*)&Thread);

	if (EOS_SUCCESS(Status)) {
	
		IntState = KeEnableInterrupts(FALSE);	// ���ж�

		if (Ready == Thread->State) {

			//
			// �����ھ���״̬��Ready�����̴߳Ӿ����������Ƴ���
			// �Ӷ�ʹ���߳̽�������״̬��Zero����
			//
			PspUnreadyThread(Thread);
			
			//
			// ����������״̬���̲߳�������̶߳��е�ĩβ��
			// �����̵߳Ĳ���������ˡ��߳��ɻ����״̬��Active Ready�����뾲ֹ����״̬��Static Ready����
			//
			ListInsertTail(&SuspendListHead, &Thread->StateListEntry);

			Status = STATUS_SUCCESS;
			
		} else {
		
			Status = STATUS_NOT_SUPPORTED;
			
		}

		KeEnableInterrupts(IntState);	// ���ж�

		ObDerefObject(Thread);
	}

	return Status;
}

STATUS
PsResumThread(
	IN HANDLE hThread
	)
/*++

����������
	�ָ�ָ�����̡߳�

������
	hThread - ��Ҫ���ָ����̵߳ľ����

����ֵ��
	����ɹ��򷵻� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	PTHREAD Thread;

	//
	// �����߳̾������̶߳����ָ��
	//
	Status = ObRefObjectByHandle(hThread, PspThreadType, (PVOID*)&Thread);

	if (EOS_SUCCESS(Status)) {

		IntState = KeEnableInterrupts(FALSE);	// ���ж�

		if (Zero == Thread->State) {

			//
			// �ڴ���Ӵ��뽫�ָ̻߳�Ϊ����״̬
			//
			
			Status = STATUS_SUCCESS;
			
		} else {
		
			Status = STATUS_NOT_SUPPORTED;
			
		}

		KeEnableInterrupts(IntState);	// ���ж�

		ObDerefObject(Thread);
	}

	return Status;
}
