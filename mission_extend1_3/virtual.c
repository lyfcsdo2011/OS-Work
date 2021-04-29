/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: virtual.c

����: �����ڴ�������������ڴ�ķ��䡢���յȡ�



*******************************************************************************/

#include "mi.h"

PRIVATE STATUS
MiCommitPages(
	IN ULONG_PTR StartingVpn,
	IN ULONG_PTR EndVpn
	);

PRIVATE STATUS
MiDecommitPages(
	IN ULONG_PTR StartingVpn,
	IN ULONG_PTR EndVpn
	);

STATUS
MmAllocateVirtualMemory(
	IN OUT PVOID *BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG AllocationType,
	IN BOOL SystemVirtual
	)
/*++

����������
	�ڵ�ǰ���̵�ַ�ռ��ϵͳ��ַ�ռ��з��������ڴ档

������
	BaseAddress -- ��Ϊ����ʱ�������������������ύ�ĵ�ַ�������ʼ��ַ����Ϊ��
		��ʱ�����ʵ�ʱ��������ύ�ĵ�ַ�������ʼ��ַ����������NULL�������ֵ
		Ϊ����ֵ���¶��뵽ҳ��߽磬������ϵͳ��������λ�ã���Ȼ���뵽ҳ��߽磩��
	RegionSize -- ��Ϊ����ʱ�������������������ύ���ڴ�����Ĵ�С����Ϊ���ʱ��
		���ʵ�ʱ��������ύ���ڴ�����Ĵ�С��Ϊҳ���С����������
	AllocationType -- �������ͣ�����ȡֵ��
		MEM_RESERVE�����ڽ��̵�ַ�ռ��б���һ�������ַ�����Ա�ʹ�ã�
		MEM_COMMIT����Ϊ�ѱ����������ύ�ڴ棨Ϊ�����ַӳ�������ڴ棩��
		��������ֵ����ͬʱָ����MEM_RESERVE | MEM_COMMIT�����ڱ�����ַ�����ͬʱ
		Ϊ�������������ύ�����ڴ档
	SystemVirtual -- �Ƿ���ϵͳ��ַ�ռ���������ڴ档

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS�������ʾʧ�ܡ�

--*/
{
	STATUS Status;
	BOOL IntState;
	PMMPAS Pas;
	PMMVAD Vad;
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;

	ASSERT(BaseAddress != NULL && RegionSize != NULL);

	//
	// ȷ��������Ч����ַ��Χû���������
	//
	if (0 == *RegionSize || *BaseAddress + *RegionSize - 1 < *BaseAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ֻ����MEM_RESERVE��MEM_COMMIT��ѡ������ѡ��һ���������ѡ��
	//
	if ((AllocationType & (MEM_RESERVE | MEM_COMMIT)) == 0 ||
		(AllocationType & ~(MEM_RESERVE | MEM_COMMIT)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	IntState = KeEnableInterrupts(FALSE);

	if (SystemVirtual) {
		Pas = &MiSystemPas;
	} else {
		Pas = MiCurrentPas;
	}

	do {

		if ((AllocationType & MEM_RESERVE) != 0) {

			//
			// �ڵ�ַ�ռ��б���һ�ε�ַ���򣬱����ĵ�λΪҳ��
			//
			Status = MiReserveAddressRegion( &Pas->VadList,
											 *BaseAddress,
											 *RegionSize,
											 &Vad );

			if (!EOS_SUCCESS(Status)) {
				break;
			}

			//
			// ��¼ʵ�ʱ�������ʼ��������ҳ��š�
			//
			StartingVpn = Vad->StartingVpn;
			EndVpn = Vad->EndVpn;

		} else {

			//
			// ��ѯMEM_COMMIT������ַ�����Ƿ�Ϊ�ѱ�����ַ�����粻���򷵻�ʧ�ܡ�
			//
			Status = MiFindReservedAddressRegion( &Pas->VadList,
												  *BaseAddress,
												  *RegionSize,
												  &Vad );

			if (!EOS_SUCCESS(Status)) {
				break;
			}

			//
			// MEM_COMMIT�����ĵ�λΪҳ��
			//
			StartingVpn = MI_VA_TO_VPN(*BaseAddress);
			EndVpn = MI_VA_TO_VPN(*BaseAddress + *RegionSize - 1);
		}

		if ((AllocationType & MEM_COMMIT) != 0) {

			//
			// ִ��MEM_COMMIT������
			//
			Status = MiCommitPages(StartingVpn, EndVpn);

			if (!EOS_SUCCESS(Status)) {

				ASSERT(STATUS_NO_MEMORY == Status);

				//
				// ���ǰ����ִ��MEM_RESERVE��ع�MEM_RESERVE��
				//
				if ((AllocationType & MEM_RESERVE) != 0) {
					MiFreeAddressRegion(&Pas->VadList, Vad);
				}

				break;
			}
		}

		//
		// ���÷���ֵ��
		//
		*BaseAddress = MI_VPN_TO_VA(StartingVpn);
		*RegionSize = (EndVpn - StartingVpn + 1) << PAGE_SHIFT;

		Status = STATUS_SUCCESS;

	} while (0);

	KeEnableInterrupts(IntState);

	return Status;
}

STATUS
MmFreeVirtualMemory(
	IN OUT PVOID *BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG FreeType,
	IN BOOL SystemVirtual
	)
/*++

����������
	�ڵ�ǰ���̵�ַ�ռ��ϵͳ��ַ�ռ����ͷ������ڴ档

������
	BaseAddress -- ������ʱ�����������ͷŵ��������ʼ��ַ���������FreeType��ֵ
		ΪMEM_RELEASE����BaseAddress��ֵ�������� MmAllocateVirtualMemory ������
		��ʱ�ķ���ֵ�������ʱ�����ʵ���ͷŵĵ�ַ�������ʼ��ַ��������ֵ���¶�
		�뵽ҳ��߽����õ���
	RegionSize -- ������ʱ���������FreeType��ֵΪMEM_RELEASE�����Ϊ0����������
		����MEM_DECOMMIT���ڴ��С�������ʱ�����ʵ���ͷŵ������С��
	FreeType -- �ͷŲ������ͣ���ֵֻ��������֮һ��
		MEM_RELEASE���ͷ������ѱ������������ύ�ڴ�ͬʱ���ͷţ�
		MEM_DECOMMIT���Ա��������ڲ������ύ��������з��ύ������
	SystemVirtual -- �Ƿ���ϵͳ��ַ�ռ��ͷ������ڴ档

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	PMMPAS Pas;
	PMMVAD Vad;
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;

	//
	// *BaseAddress��������Ч��ַ��
	//
	if (NULL == *BaseAddress) {
		return STATUS_INVALID_ADDRESS;
	}

	//
	// ��֤��ַ��Χû�������*RegionSize����Ϊ0����
	//
	if (*RegionSize > 0 && *BaseAddress + *RegionSize - 1 < *BaseAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ֻ����MEM_DECOMMIT��MEM_RELEASE��ѡ��֮һ��
	//
	if ((FreeType & ~(MEM_DECOMMIT | MEM_RELEASE)) != 0 ||
		((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) == 0) ||
		((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) == (MEM_DECOMMIT | MEM_RELEASE))) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ���ָ����MEM_RELEASE��*RegionSize����Ϊ0��
	//
	if ((FreeType & MEM_RELEASE) != 0 && *RegionSize != 0) {
		return STATUS_INVALID_PARAMETER;		
	}

	IntState = KeEnableInterrupts(FALSE);

	if (SystemVirtual) {
		Pas = &MiSystemPas;
	} else {
		Pas = MiCurrentPas;
	}

	do {

		//
		// �����ѱ�����ַ�������Ŀ��������ѱ��������򷵻�ʧ�ܡ�
		//
		Status = MiFindReservedAddressRegion( &Pas->VadList,
											  *BaseAddress,
											  *RegionSize,
											  &Vad );
		
		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// ��¼�����ѱ����������ֹҳ�š�
		//
		StartingVpn = Vad->StartingVpn;
		EndVpn = Vad->EndVpn;

		if ((FreeType & MEM_RELEASE) != 0) {

			//
			// ִ��MEM_RELEASE����ʱ��*BaseAddress�����������ѱ�������Ļ�ַ��
			//
			if (*BaseAddress != MI_VPN_TO_VA(StartingVpn)) {

				Status = STATUS_FREE_VM_NOT_AT_BASE;

				break;
			}

			//
			// �ͷ��ѱ�����ַ����
			//
			MiFreeAddressRegion(&Pas->VadList, Vad);

		} else {

			//
			// ��*BaseAddress���ڱ����������ʼ��ַ��*RegionSizeΪ0ʱ���ɶ�����
			// ��������DECOMMIT�����������*RegionSize����Ϊ0��
			//
			if(0 == *RegionSize) {

				if(*BaseAddress != MI_VPN_TO_VA(StartingVpn)) {

					Status = STATUS_FREE_VM_NOT_AT_BASE;

					break;
				}

			} else {
				
				//
				// MEM_DECOMMIT�����ĵ�λΪҳ��
				//
				StartingVpn = MI_VA_TO_VPN(*BaseAddress);
				EndVpn = MI_VA_TO_VPN(*BaseAddress + *RegionSize - 1);
			}
		}

		//
		// ִ��MEM_DECOMMIT������
		//
		Status = MiDecommitPages(StartingVpn, EndVpn);
		ASSERT(EOS_SUCCESS(Status));

		//
		// ���÷���ֵ��
		//
		*BaseAddress = MI_VPN_TO_VA(StartingVpn);
		*RegionSize = (EndVpn - StartingVpn + 1) << PAGE_SHIFT;

		Status = STATUS_SUCCESS;

	} while (0);

	KeEnableInterrupts(IntState);

	return Status;
}

VOID
MmCleanVirtualMemory(
	VOID
	)
{
	ASSERT(MiCurrentPas != &MiSystemPas);

	MiCleanAddressRegion(&MiCurrentPas->VadList);
	MiDecommitPages(MiCurrentPas->VadList.StartingVpn, MiCurrentPas->VadList.EndVpn);
}

STATUS
MiCommitPages(
	IN ULONG_PTR StartingVpn,
	IN ULONG_PTR EndVpn
	)
/*++

����������
	Ϊ����������ҳ��ӳ������ҳ��

������
	StartingVpn -- ��ʼ��ҳ��š�
	EndVpn -- ������ҳ��š�

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	ULONG_PTR Vpn;
	ULONG_PTR Pfn;
	ULONG_PTR DemandPages;

	//
	// ͳ��Ϊ�����ַ�����ύ�ڴ����������ҳ��������
	//
	DemandPages = 0;
	
	//
	// �������ϵͳ�ڴ���ͳ����������ַ��ȱ��ҳ����������Ϊϵͳ��ַ�ռ�ȫ����
	// װ�������ҳ����û���ַ�ռ�û�С�
	//
	if (StartingVpn < MI_VA_TO_VPN(MM_SYSTEM_RANGE_START)) {

		for (Vpn = StartingVpn & ~((1 << PTI_BITS) -1); Vpn <= EndVpn; Vpn += (1 << PTI_BITS))  {

			if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid) {
				DemandPages++;
			}
		}
	}
	

	//
	// ͳ����������ַ��ҳ��ն�������
	//
	for (Vpn = StartingVpn; Vpn <= EndVpn; Vpn++) {

		if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid ||
			0 == MiGetPteAddress(Vpn)->u.Hard.Valid) {
			DemandPages++;
		}
	}

	//
	// ���û���㹻�Ŀ�������ҳ�򷵻�ʧ�ܡ�
	//
	if (DemandPages > MiGetAnyPageCount()) {
		return STATUS_NO_MEMORY;
	}

	for (Vpn = StartingVpn; Vpn <= EndVpn; Vpn++) {

		//
		// ���ҳĿ¼�����ҳ������Ч��˵����ǰ��ַû��ӳ�������ڴ档
		//
		if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid ||
			0 == MiGetPteAddress(Vpn)->u.Hard.Valid) {

			//
			// ���ҳĿ¼����Ч�����һ����ҳ��Ϊҳ������ҳĿ¼�
			//
			if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid) {

				ASSERT(Vpn < MI_VA_TO_VPN(MM_SYSTEM_RANGE_START));
				
				Status = MiAllocateZeroedPages(1, &Pfn);
				ASSERT(EOS_SUCCESS(Status));
				
				MiMapPageTable(Vpn, Pfn);
			}

			//
			// ����һ������ҳ����֮ӳ�䵽����ҳ��
			//
			Status = MiAllocateZeroedPages(1, &Pfn);
			ASSERT(EOS_SUCCESS(Status));

			MiMapPage(Vpn, Pfn);

			//
			// ����ҳ���Ӧ����ЧPTE��������
			// ע�⣺ϵͳ��ַ�ռ�Ӳ�ʹ��PTE��������ҳ��Ӳ���ж�ػ�װ��
			//
			if (Vpn < MI_VA_TO_VPN(MM_SYSTEM_RANGE_START)) {
				MiIncPteCounter(Vpn);
			}
		}
	}

	return STATUS_SUCCESS;
}

STATUS
MiDecommitPages(
	IN ULONG_PTR StartingVpn,
	IN ULONG_PTR EndVpn
	)
/*++

����������
	�ͷ�ӳ������������ҳ���ϵ�����ҳ��

������
	StartingVpn -- ��ʼ��ҳ��š�
	EndVpn -- ������ҳ��š�

����ֵ��
	����ɹ��򷵻�STATUS_SCCESS�������ʾʧ�ܡ�

--*/
{
	ULONG_PTR Vpn;
	ULONG_PTR Pfn;

	//
	// ���ÿ������ҳ���������ҳӳ��������ҳ����ȡ��ӳ�䲢�ͷ�����ҳ��
	//
	for (Vpn = StartingVpn; Vpn <= EndVpn; Vpn++) {

		if (1 == MiGetPdeAddress(Vpn)->u.Hard.Valid &&
			1 == MiGetPteAddress(Vpn)->u.Hard.Valid) {

			Pfn = MiGetPteAddress(Vpn)->u.Hard.PageFrameNumber;
			MiUnmapPage(Vpn);
	
			MiFreePages(1, &Pfn);

			//
			// ��СPTE�����������Ϊ0��ж�ز�����ҳ��
			// ע�⣺ϵͳ��ַ�ռ��ҳ��Ӳ���ж�ء�
			//
			if (Vpn < MI_VA_TO_VPN(MM_SYSTEM_RANGE_START) && 1 == MiDecPteCounter(Vpn)) {

				Pfn = MiGetPdeAddress(Vpn)->u.Hard.PageFrameNumber;

				MiUnmapPageTable(Vpn);

				MiFreePages(1, &Pfn);
			}
		}
	}

	return STATUS_SUCCESS;
}

BOOL
MmIsAddressValid(
	IN PVOID VirtualAddress
	)
/*++

����������
	����������ָ���������ַ�Ƿ���������Υ���쳣��Ҳ���Ǽ�������ַ�Ƿ�ӳ��
	�������ڴ档

������
	VirtualAddress -- �����ַ��

����ֵ��
	�����д���ʵ�ַ���������쳣�򷵻�TRUE��

--*/
{
	BOOL Result;
	BOOL IntState;
	ULONG_PTR Vpn;

	Vpn = MI_VA_TO_VPN(VirtualAddress);

	IntState = KeEnableInterrupts(FALSE);

	Result = MiGetPdeAddress(Vpn)->u.Hard.Valid && MiGetPteAddress(Vpn)->u.Hard.Valid;

	KeEnableInterrupts(IntState);

	return Result;
}

STATUS
MmGetPhysicalAddress(
	IN PVOID VirtualAddress,
	OUT PVOID* PhysicalAddress
	)
/*++

����������
	�õ������ַ��Ӧ�������ַ.

������
	VirtualAddress -- �����ַ��
	PhysicalAddress -- ָ�룬ָ��������������ַ�Ļ�������

����ֵ��
	����ɹ��򷵻�STATUS_SUCESS��

--*/
{
	STATUS Status;
	BOOL IntState;
	ULONG_PTR Vpn;
	ULONG_PTR Pfn;

	IntState = KeEnableInterrupts(FALSE);

	Vpn = MI_VA_TO_VPN(VirtualAddress);

	if (1 == MiGetPdeAddress(Vpn)->u.Hard.Valid &&
		1 == MiGetPteAddress(Vpn)->u.Hard.Valid) {

		Pfn = MiGetPteAddress(Vpn)->u.Hard.PageFrameNumber;

		*PhysicalAddress = (PVOID)((Pfn << PAGE_SHIFT) + BYTE_OFFSET(VirtualAddress));

		Status = STATUS_SUCCESS;

	} else {

		Status = STATUS_INVALID_ADDRESS;
	}

	KeEnableInterrupts(IntState);

	return Status;
}
