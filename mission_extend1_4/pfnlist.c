/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: pfnlist.c

����: �����ڴ������������ҳ�ķ��䡢�����Լ���ҳ�̡߳�



*******************************************************************************/

#include "mi.h"

//
// ����ҳ��������
//
ULONG_PTR MiTotalPageFrameCount;

//
// ��ҳ����ͷ�Լ���ҳ������
//
ULONG_PTR MiZeroedPageListHead;
ULONG_PTR MiZeroedPageCount;

//
// ����ҳ����ͷ�Լ�����ҳ������
//
ULONG_PTR MiFreePageListHead;
ULONG_PTR MiFreePageCount;

VOID
MiInitializePfnDatabase(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	)
/*++

����������
	��ʼ��ҳ������ݿ⡣
	ҳ������ݿ���һ��MMPFN�ṹ�����飬Ԫ������������ҳ������һ�£������±�Ϊ
	n��MMPFN�ṹ���Ӧ��ҳ���Ϊn������ҳ��MMPFN�ṹ���е�PageState�����˶�
	Ӧ������ҳ���״̬��Ŀǰ������״̬ZEROED_PAGE��FREE_PAGE��BUSY_PAGE���ṹ
	���е�NextΪ��һ��ͬ״̬ҳ���ҳ��ţ������͹�����ͬ״̬ҳ������

������
	LoaderBlock -- ���ز����ṹ��ָ�롣

����ֵ��
	�ޡ�

--*/
{
	ULONG_PTR i;

	MiTotalPageFrameCount = LoaderBlock->PhysicalMemorySize / PAGE_SIZE;

	//
	// ��ʼ������ҳ������ݿ⡣
	// ҳ������ݿ���һ��MMPFN�ṹ�����飬���鳤�Ⱥ�����ҳ������һ�£������±�Ϊ
	// n��MMPFN�ṹ���Ӧ��ҳ���Ϊn������ҳ��MMPFN�ṹ���е�PageState�����˶�
	// Ӧ������ҳ���״̬��Ŀǰ������״̬ZEROED_PAGE��FREE_PAGE��BUSY_PAGE���ṹ
	// ���е�NextΪ��һ��ͬ״̬ҳ���ҳ��ţ������͹�����ͬ״̬ҳ������
	//

	//
	// ��ʼ��ʹ��ҳ��Ӧ�����ݿ��
	//
	for (i = 0; i < LoaderBlock->FirstFreePageFrame; i++) {
		MiGetPfnDatabaseEntry(i)->PageState = BUSY_PAGE;
	}

	//
	// ��ʼ������ҳ��Ӧ�����ݿ������֮�������
	//
	MiFreePageListHead = LoaderBlock->FirstFreePageFrame;
	MiFreePageCount = MiTotalPageFrameCount - MiFreePageListHead;

	for (i = MiFreePageListHead; i < MiTotalPageFrameCount; i++) {
		MiGetPfnDatabaseEntry(i)->PageState = FREE_PAGE;
		MiGetPfnDatabaseEntry(i)->Next = i + 1;
	}

	MiGetPfnDatabaseEntry(i - 1)->Next = -1;

	//
	// ��ҳ����Ϊ�ա�
	//
	MiZeroedPageListHead = -1;
	MiZeroedPageCount = 0;
}


ULONG_PTR
MiGetAnyPageCount(
	VOID
	)
/*++

����������
	�õ����õĿ�������ҳ���������

������
	�ޡ�

����ֵ��
	Ŀǰ���õĿ�������ҳ���������

--*/
{
	return MiFreePageCount + MiZeroedPageCount;
}

STATUS
MiAllocateAnyPages(
	IN ULONG_PTR NumberOfPages,
	OUT PULONG_PTR PfnArray
	)
/*++

����������
	��������ҳ�����ȴӿ���ҳ�����з��䣬�������ҳ���������ٴ���ҳ������䡣

������
	NumberOfPages -- �������������ҳ��������
	PfnArray -- ָ�룬ָ�������������ҳ��ŵĻ�������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS�����򷵻�STATUS_NO_MEMORY��

--*/
{
	BOOL IntState;
	ULONG_PTR Pfn;
	ULONG_PTR i;

	IntState = KeEnableInterrupts(FALSE);

	if (NumberOfPages <= MiZeroedPageCount + MiFreePageCount) {

		//
		// �ȴӿ��������з��䡣
		//
		for (i = 0; i < NumberOfPages && MiFreePageCount > 0; i++) {

			Pfn = MiFreePageListHead;
			MiFreePageListHead = MiGetPfnDatabaseEntry(Pfn)->Next;
			MiFreePageCount--;

			MiGetPfnDatabaseEntry(Pfn)->PageState = BUSY_PAGE;

			PfnArray[i] = Pfn;
		}

		//
		// ����������������������ҳ������䡣
		//
		for (; i < NumberOfPages; i++) {

			Pfn = MiZeroedPageListHead;
			MiZeroedPageListHead = MiGetPfnDatabaseEntry(Pfn)->Next;
			MiZeroedPageCount--;

			MiGetPfnDatabaseEntry(Pfn)->PageState = BUSY_PAGE;

			PfnArray[i] = Pfn;
		}
	}

	KeEnableInterrupts(IntState);

	return STATUS_SUCCESS;
}

STATUS
MiAllocateZeroedPages(
	IN ULONG_PTR NumberOfPages,
	OUT PULONG_PTR PfnArray
	)
/*++

����������
	���ȴ���ҳ�����з��䣬�����ҳ���������ٴӿ���ҳ
	������䣨�������㣩��

������
	NumberOfPages -- �������������ҳ��������
	PfnArray -- ָ�룬ָ�������������ҳ��ŵĻ�������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS�����򷵻�STATUS_NO_MEMORY��

--*/
{
	BOOL IntState;
	ULONG_PTR Pfn;
	PVOID ZeroBuffer;
	ULONG_PTR i;

	IntState = KeEnableInterrupts(FALSE);

	if (NumberOfPages <= MiZeroedPageCount + MiFreePageCount) {

		//
		// �ȴ���ҳ������䡣
		//
		for (i = 0; i < NumberOfPages && MiZeroedPageCount > 0; i++) {

			Pfn = MiZeroedPageListHead;
			MiZeroedPageListHead = MiGetPfnDatabaseEntry(Pfn)->Next;
			MiZeroedPageCount--;

			MiGetPfnDatabaseEntry(Pfn)->PageState = BUSY_PAGE;

			PfnArray[i] = Pfn;
		}

		//
		// �����ҳ�����㣬������ӿ���ҳ������䡣
		//
		for (; i < NumberOfPages; i++) {

			Pfn = MiFreePageListHead;
			MiFreePageListHead = MiGetPfnDatabaseEntry(Pfn)->Next;
			MiFreePageCount--;

			MiGetPfnDatabaseEntry(Pfn)->PageState = BUSY_PAGE;

			//
			// ������ҳӳ�䵽ϵͳPTE����������㡣
			//
			ZeroBuffer = MiMapPageToSystemPte(Pfn);
			memset(ZeroBuffer, 0, PAGE_SIZE);
			MiFreeSystemPte(ZeroBuffer);

			PfnArray[i] = Pfn;
		}
	}

	KeEnableInterrupts(IntState);

	return STATUS_SUCCESS;
}

STATUS
MiFreePages(
	IN ULONG_PTR NumberOfPages,
	IN PULONG_PTR PfnArray
	)
/*++

����������
	�ͷ�����ҳ�档

������
	NumberOfPages -- �ͷŵ�ҳ��������
	PfnArray -- ָ��ҳ��Ż�������ָ�롣

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	ULONG_PTR Pfn;
	ULONG_PTR i;

	//
	// ������д����յ�����ҳ��ȷ����Ϊ��Ч����ʹ������ҳ��
	//
	for (i = 0; i < NumberOfPages; i++) {

		Pfn = PfnArray[i];

		if (Pfn >= MiTotalPageFrameCount || MiGetPfnDatabaseEntry(Pfn)->PageState != BUSY_PAGE) {
			ASSERT(FALSE);
			return STATUS_MEMORY_NOT_ALLOCATED;
		}
	}

	//
	// �޸���Щ����ҳ��״̬Ϊ���У��������ǲ������ҳ����ͷ����
	//
	for (i = 0; i < NumberOfPages; i++) {

		Pfn = PfnArray[i];

		MiGetPfnDatabaseEntry(Pfn)->PageState = FREE_PAGE;
		MiGetPfnDatabaseEntry(Pfn)->Next = MiFreePageListHead;
		MiFreePageListHead = Pfn;
	}

	MiFreePageCount += NumberOfPages;
	
	return STATUS_SUCCESS;
}
