/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: vadlist.c

����: �����ַ�������������ģ�顣



*******************************************************************************/

#include "mi.h"

VOID
MiInitializeVadList(
	IN PMMVAD_LIST VadList, 
	IN PVOID StartingAddress, 
	IN PVOID EndAddress
	)
/*++

����������
	��ʼ����ַMMVAD_LIST��

������
	VadList -- �����ַ����������ָ�롣
	StartingAddress -- ��ʼ�����ַ��
	EndAddress -- ���������ַ��

����ֵ��
	�ޡ�

--*/
{
	//
	// ȷ����ַ��Χ��Ч��ҳ����롣
	//
	ASSERT(StartingAddress != NULL && StartingAddress < EndAddress);
	ASSERT(IS_PAGE_ALIGNED(StartingAddress) && IS_PAGE_ALIGNED(EndAddress + 1));

	VadList->StartingVpn = MI_VA_TO_VPN(StartingAddress);
	VadList->EndVpn = MI_VA_TO_VPN(EndAddress);
	ListInitializeHead(&VadList->VadListHead);
}

STATUS
MiReserveAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PVOID BaseAddress,
	IN SIZE_T RegionSize,
	OUT PMMVAD *Vad
	)
/*++

����������
	����һ�������ַ����

������
	VadList -- �����ַ����������ָ�롣
	BaseAddress --���������ĵ�ַ�������ʼ��ַ��
	RegionSize -- ���������ĵ�ַ����Ĵ�С��
	Vad -- ָ�룬ָ��������������ַ������ָ��Ļ�������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS������ʧ�ܡ�

--*/
{
	PLIST_ENTRY ListEntry;
	PMMVAD PointerVad;
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;
	ULONG_PTR Size;

	ASSERT(RegionSize != 0 && BaseAddress + RegionSize - 1 >= BaseAddress);

	if (NULL != BaseAddress) {

		StartingVpn = MI_VA_TO_VPN(BaseAddress);
		EndVpn = MI_VA_TO_VPN(BaseAddress + RegionSize - 1);

		if (StartingVpn < VadList->StartingVpn || EndVpn > VadList->EndVpn) {
			return STATUS_INVALID_ADDRESS;
		}

		//
		// �ҵ�Ŀ������ǰ���ѱ�������P��
		//
		for (ListEntry = VadList->VadListHead.Prev;
			ListEntry != &VadList->VadListHead;
			ListEntry = ListEntry->Prev) {

			PointerVad = CONTAINING_RECORD(ListEntry, MMVAD, VadListEntry);
			
			if(PointerVad->EndVpn < StartingVpn) {
				break;
			}
		}

		//
		// ���P֮����ѱ��������Ŀ�������ص��򷵻�ʧ�ܡ�
		//
		ListEntry = ListEntry->Next;

		if(ListEntry != &VadList->VadListHead) {

			PointerVad = CONTAINING_RECORD(ListEntry, MMVAD, VadListEntry);
			
			if (PointerVad->StartingVpn <= EndVpn) {
				return STATUS_INVALID_ADDRESS;
			}
		}

	} else {

		//
		// �ӵ�ַ�ռ����ʼ�������ҵ�һ�����������С��δ��������
		//
		Size = (RegionSize + PAGE_SIZE - 1) >> PAGE_SHIFT;

		StartingVpn = VadList->StartingVpn;
		EndVpn = StartingVpn + Size - 1;

		if (EndVpn > VadList->EndVpn) {
			return STATUS_NO_MEMORY;
		}

		for (ListEntry = VadList->VadListHead.Next;
			ListEntry != &VadList->VadListHead;
			ListEntry = ListEntry->Next) {

			PointerVad = CONTAINING_RECORD(ListEntry, MMVAD, VadListEntry);

			if (EndVpn < PointerVad->StartingVpn) {
				break;
			}

			//
			// �����ѱ�����������ƶ���
			//
			StartingVpn = PointerVad->EndVpn + 1;
			EndVpn = StartingVpn + Size - 1;

			//
			// �����ַ��Χ������߳����ռ䷶Χ��ʧ�ܣ�
			//
			if (EndVpn > VadList->EndVpn) {
				return STATUS_NO_MEMORY;
			}
		}
	}

	//
	// ��ϵͳ�ڴ���з���һ�������ַ�������ṹ�塣
	//
	PointerVad = MmAllocateSystemPool(sizeof(MMVAD));

	if (NULL == PointerVad) {
		return STATUS_NO_MEMORY;
	}

	//
	// �������ַ�����������ѱ������������С�
	//
	PointerVad->StartingVpn = StartingVpn;
	PointerVad->EndVpn = EndVpn;
	ListInsertBefore(ListEntry, &PointerVad->VadListEntry);

	*Vad = PointerVad;
	
	return STATUS_SUCCESS;
}

STATUS
MiFindReservedAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PVOID BaseAddress,
	IN SIZE_T RegionSize,
	OUT PMMVAD *Vad
	)
/*++

����������
	����ָ����ַ�����Ƿ��Ѿ���������

������
	VadList -- �����ַ����������ָ�롣
	BaseAddress -- ����Ŀ�����ʼ��ַ��
	RegionSize -- ����Ŀ��Ĵ�С��Ϊ0ʱ���ο�BaseAddress��
	Vad -- ָ�룬ָ��������������ַ������ָ��Ļ�������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS������ʧ�ܡ�

--*/
{
	PLIST_ENTRY ListEntry;
	PMMVAD PointerVad;
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;

	ASSERT(0 == RegionSize || BaseAddress + RegionSize - 1 >= BaseAddress);

	StartingVpn = MI_VA_TO_VPN(BaseAddress);

	if (0 == RegionSize) {
		EndVpn = StartingVpn;
	} else {
		EndVpn = MI_VA_TO_VPN(BaseAddress + RegionSize - 1);
	}

	if (VadList->StartingVpn <= StartingVpn && EndVpn <= VadList->EndVpn) {

		for (ListEntry = VadList->VadListHead.Next;
			ListEntry != &VadList->VadListHead;
			ListEntry = ListEntry->Next) {

			PointerVad = CONTAINING_RECORD(ListEntry, MMVAD, VadListEntry);

			if (PointerVad->EndVpn >= EndVpn) {

				if (PointerVad->StartingVpn <= StartingVpn) {

					*Vad = PointerVad;

					return STATUS_SUCCESS;

				} else {

					return STATUS_MEMORY_NOT_ALLOCATED;
				}
			}
		}
	}

	return STATUS_INVALID_ADDRESS;
}

VOID
MiFreeAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PMMVAD Vad
	)
/*++

����������
	�ͷű�����ַ����

������
	VadList -- �����ַ����������ָ�롣
	Vad -- �����ַ������ָ�롣

����ֵ��
	�ޡ�

--*/
{
	ASSERT(NULL != VadList && NULL != Vad);

	ListRemoveEntry(&Vad->VadListEntry);

	MmFreeSystemPool(Vad);
}

VOID
MiCleanAddressRegion(
	IN PMMVAD_LIST VadList
	)
/*++

����������
	����VAD�����е������ѱ������򣬽������ٵ�ַ�ռ�ʱ��ʹ�á�

������
	VadList -- VAD����ָ�롣

����ֵ��
	��

--*/
{
	PLIST_ENTRY CurrentEntry;
	PLIST_ENTRY NextEntry;

	NextEntry = VadList->VadListHead.Next;

	while (NextEntry != &VadList->VadListHead) {

		CurrentEntry = NextEntry;
		NextEntry = NextEntry->Next;

		ListRemoveEntry(CurrentEntry);
		MmFreeSystemPool(CONTAINING_RECORD(CurrentEntry, MMVAD, VadListEntry));
	}
}
