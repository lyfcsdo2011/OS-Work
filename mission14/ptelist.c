/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: ptelist.c

����: ϵͳ PTE ����ʵ���˵�ҳ�����ڴ�Ŀ���ӳ�䡣



*******************************************************************************/

#include "mi.h"

PRIVATE ULONG_PTR MiFreePteListHead = MI_START_VPN_OF_SYSTEM_PTE;

VOID
MiInitialzieSystemPte(
	VOID
	)
{
	ULONG_PTR Vpn;

	//
	// ��ϵͳPTE���������PTE��ʼ��Ϊ����
	//
	for (Vpn = MI_START_VPN_OF_SYSTEM_PTE; Vpn < MI_END_VPN_OF_SYSTEM_PTE; Vpn++) {
		MiGetPteAddress(Vpn)->u.Hard.PageFrameNumber = Vpn + 1;
	}

	MiGetPteAddress(Vpn)->u.Hard.PageFrameNumber = 0;
}

PVOID
MiMapPageToSystemPte(
	IN ULONG_PTR Pfn
	)
/*++

����������
	������ҳ��ӳ�䵽ϵͳPTE����

������
	Pfn -- ϣ��ӳ�������ҳ���ҳ��š�

����ֵ��
	ӳ���������ַ�����ϵͳPTE�Ѿ������򷵻�NULL��һ�㲻�����꣩��

--*/
{
	BOOL IntState;
	ULONG_PTR Vpn;

	IntState = KeEnableInterrupts(FALSE);

	Vpn = MiFreePteListHead;

	if (0 != MiFreePteListHead) {	

		MiFreePteListHead = MiGetPteAddress(MiFreePteListHead)->u.Hard.PageFrameNumber;

		MiMapPage(Vpn, Pfn);
	}

	KeEnableInterrupts(IntState);

	return MI_VPN_TO_VA(Vpn);
}

VOID
MiFreeSystemPte(
	IN PVOID Va 
	)
/*++

����������
	�ͷ�ϵͳPTE��

������
	Va -- ��MiMapPageToSystemPte���ص������ַ��

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	ULONG_PTR vpn = MI_VA_TO_VPN(Va);

	ASSERT(vpn >= MI_START_VPN_OF_SYSTEM_PTE && vpn <= MI_END_VPN_OF_SYSTEM_PTE);
	ASSERT(MiGetPteAddress(vpn)->u.Hard.Valid);

	MiUnmapPage(vpn);

	IntState = KeEnableInterrupts(FALSE);

	MiGetPteAddress(vpn)->u.Hard.PageFrameNumber = MiFreePteListHead;

	MiFreePteListHead =  vpn;

	KeEnableInterrupts(IntState);
}
