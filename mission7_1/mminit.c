/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: mminit.c

����: ��ʼ���ڴ����ģ�顣



*******************************************************************************/

#include "mi.h"

VOID
MmInitializeSystem1(
	IN PVOID LoaderBlock
	)
/*++

����������
	�ڴ����ģ��ĵ�һ����ʼ����

������
	LoaderBlock - Loader���ݵļ��ز�����ṹ��ָ�룬�ڴ������Ҫʹ�á�

����ֵ��
	�ޡ�

--*/
{
	PLOADER_PARAMETER_BLOCK lb = (PLOADER_PARAMETER_BLOCK)LoaderBlock;

	ASSERT(MM_SYSTEM_RANGE_START == (PVOID)lb->SystemVirtualBase);
	ASSERT(PTE_BASE == (PMMPTE)lb->PageTableVirtualBase);
	ASSERT(MM_KERNEL_IMAGE_BASE == (PVOID)lb->ImageVirtualBase);

	//
	// ��ʼ��ҳ������ݿ⡣
	//
	MiInitializePfnDatabase(lb);

	//
	// ��ʼ��ϵͳ�ڴ�ء�
	//
	MiInitializeSystemPool(lb);

	//
	// ����ISRջָ�롣ISRջ����ӳ���ڴ����ߴ�����������
	//
	KeIsrStack = MM_SYSTEM_RANGE_START + lb->MappedMemorySize;

	//
	// ��ʼ��ϵͳPTE��
	//
	MiInitialzieSystemPte();

	//
	// ��ʼ��ϵͳ���̵�ַ�ռ䡣
	//
	MiInitializeSystemPas();
}

VOID
MmInitializeSystem2(
	VOID
	)
/*++

����������
	�ڴ�������ĵڶ�����ʼ�������Դ���ϵͳ�̣߳����Ե�������������

������
	

����ֵ��
	

--*/
{

}
