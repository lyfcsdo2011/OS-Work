/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: ki.h

����: �ں�֧��ģ����ڲ�������������������



*******************************************************************************/

#ifndef _KI_
#define _KI_

#include "ke.h"

//
// �жϼ�������ָʾ��ǰ�ж�Ƕ�׵���ȡ�
//
extern ULONG KiIntNesting;


//
// ��ʼ��ȫ����������
//
VOID
KiInitializeProcessor(
	VOID
	);

//
// ��ʼ���ж���������
//
VOID
KiInitializeInterrupt(
	VOID
	);

//
// ��ʼ���ɱ���жϿ�����(Programmable Interrupt Controller)��
//
VOID
KiInitializePic(
	VOID
	);

//
// �ɱ�̶�ʱ������(Programmable Interval Timer)��
//
VOID 
KiInitializePit(
	VOID
	);

//
// ͣ��ָ�
//
VOID
KiHaltProcessor(
	VOID
	);

//
// ��ʱ���жϺ�����
//
VOID
KiIsrTimer(
	VOID
	);

//
// �˳�ϵͳ����������
//
VOID
KiStartExit(
	VOID
	);

//
// ϵͳ���̺�����
//
ULONG
KiSystemProcessRoutine(
	IN PVOID Parameter
	);

#endif // _KI_
