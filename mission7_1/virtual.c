/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: virtual.c

描述: 虚拟内存管理，包括虚拟内存的分配、回收等。



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

功能描述：
	在当前进程地址空间或系统地址空间中分配虚拟内存。

参数：
	BaseAddress -- 作为输入时，输入期望保留或者提交的地址区域的起始地址；作为输
		出时，输出实际保留或者提交的地址区域的起始地址。如果输入非NULL，则输出值
		为输入值向下对齐到页面边界，否则由系统决定具体位置（仍然对齐到页面边界）。
	RegionSize -- 作为输入时，输入期望保留或者提交的内存区域的大小；作为输出时，
		输出实际保留或者提交的内存区域的大小，为页面大小的整数倍。
	AllocationType -- 分配类型，可以取值：
		MEM_RESERVE，仅在进程地址空间中保留一段虚拟地址区域，以备使用；
		MEM_COMMIT，仅为已保留的区域提交内存（为虚拟地址映射物理内存）。
		以上两个值可以同时指定（MEM_RESERVE | MEM_COMMIT），在保留地址区域的同时
		为整个保留区域提交物理内存。
	SystemVirtual -- 是否在系统地址空间分配虚拟内存。

返回值：
	如果成功则返回STATUS_SUCCESS，否则表示失败。

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
	// 确保参数有效（地址范围没有溢出）。
	//
	if (0 == *RegionSize || *BaseAddress + *RegionSize - 1 < *BaseAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 只能在MEM_RESERVE、MEM_COMMIT中选择，至少选择一个，可组合选择。
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
			// 在地址空间中保留一段地址区域，保留的单位为页。
			//
			Status = MiReserveAddressRegion( &Pas->VadList,
											 *BaseAddress,
											 *RegionSize,
											 &Vad );

			if (!EOS_SUCCESS(Status)) {
				break;
			}

			//
			// 记录实际保留的起始、结束虚页框号。
			//
			StartingVpn = Vad->StartingVpn;
			EndVpn = Vad->EndVpn;

		} else {

			//
			// 查询MEM_COMMIT操作地址区域是否为已保留地址区域，如不是则返回失败。
			//
			Status = MiFindReservedAddressRegion( &Pas->VadList,
												  *BaseAddress,
												  *RegionSize,
												  &Vad );

			if (!EOS_SUCCESS(Status)) {
				break;
			}

			//
			// MEM_COMMIT操作的单位为页。
			//
			StartingVpn = MI_VA_TO_VPN(*BaseAddress);
			EndVpn = MI_VA_TO_VPN(*BaseAddress + *RegionSize - 1);
		}

		if ((AllocationType & MEM_COMMIT) != 0) {

			//
			// 执行MEM_COMMIT操作。
			//
			Status = MiCommitPages(StartingVpn, EndVpn);

			if (!EOS_SUCCESS(Status)) {

				ASSERT(STATUS_NO_MEMORY == Status);

				//
				// 如果前面已执行MEM_RESERVE则回滚MEM_RESERVE。
				//
				if ((AllocationType & MEM_RESERVE) != 0) {
					MiFreeAddressRegion(&Pas->VadList, Vad);
				}

				break;
			}
		}

		//
		// 设置返回值。
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

功能描述：
	在当前进程地址空间或系统地址空间中释放虚拟内存。

参数：
	BaseAddress -- 作输入时，输入期望释放的区域的起始地址，如果参数FreeType的值
		为MEM_RELEASE，则BaseAddress的值必须是用 MmAllocateVirtualMemory 保留区
		域时的返回值；作输出时，输出实际释放的地址区域的起始地址，由输入值向下对
		齐到页面边界计算得到。
	RegionSize -- 作输入时，如果参数FreeType的值为MEM_RELEASE则必须为0，否则输入
		期望MEM_DECOMMIT的内存大小；作输出时，输出实际释放的区域大小。
	FreeType -- 释放操作类型，其值只能是如下之一：
		MEM_RELEASE，释放整个已保留的区域，已提交内存同时被释放；
		MEM_DECOMMIT，对保留区域内部分已提交的区域进行反提交操作。
	SystemVirtual -- 是否在系统地址空间释放虚拟内存。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	BOOL IntState;
	PMMPAS Pas;
	PMMVAD Vad;
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;

	//
	// *BaseAddress必须是有效地址。
	//
	if (NULL == *BaseAddress) {
		return STATUS_INVALID_ADDRESS;
	}

	//
	// 保证地址范围没有溢出（*RegionSize可以为0）。
	//
	if (*RegionSize > 0 && *BaseAddress + *RegionSize - 1 < *BaseAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 只能在MEM_DECOMMIT、MEM_RELEASE中选择之一。
	//
	if ((FreeType & ~(MEM_DECOMMIT | MEM_RELEASE)) != 0 ||
		((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) == 0) ||
		((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) == (MEM_DECOMMIT | MEM_RELEASE))) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 如果指定了MEM_RELEASE则*RegionSize必须为0。
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
		// 查找已保留地址区域，如果目标区域非已保留区域则返回失败。
		//
		Status = MiFindReservedAddressRegion( &Pas->VadList,
											  *BaseAddress,
											  *RegionSize,
											  &Vad );
		
		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// 记录整个已保留区域的起止页号。
		//
		StartingVpn = Vad->StartingVpn;
		EndVpn = Vad->EndVpn;

		if ((FreeType & MEM_RELEASE) != 0) {

			//
			// 执行MEM_RELEASE操作时，*BaseAddress必须是整个已保留区域的基址。
			//
			if (*BaseAddress != MI_VPN_TO_VA(StartingVpn)) {

				Status = STATUS_FREE_VM_NOT_AT_BASE;

				break;
			}

			//
			// 释放已保留地址区域。
			//
			MiFreeAddressRegion(&Pas->VadList, Vad);

		} else {

			//
			// 当*BaseAddress等于保留区域的起始地址且*RegionSize为0时，可对整个
			// 保留区域DECOMMIT，其它情况下*RegionSize不能为0。
			//
			if(0 == *RegionSize) {

				if(*BaseAddress != MI_VPN_TO_VA(StartingVpn)) {

					Status = STATUS_FREE_VM_NOT_AT_BASE;

					break;
				}

			} else {
				
				//
				// MEM_DECOMMIT操作的单位为页。
				//
				StartingVpn = MI_VA_TO_VPN(*BaseAddress);
				EndVpn = MI_VA_TO_VPN(*BaseAddress + *RegionSize - 1);
			}
		}

		//
		// 执行MEM_DECOMMIT操作。
		//
		Status = MiDecommitPages(StartingVpn, EndVpn);
		ASSERT(EOS_SUCCESS(Status));

		//
		// 设置返回值。
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

功能描述：
	为连续的虚拟页框映射物理页框。

参数：
	StartingVpn -- 起始虚页框号。
	EndVpn -- 结束虚页框号。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	ULONG_PTR Vpn;
	ULONG_PTR Pfn;
	ULONG_PTR DemandPages;

	//
	// 统计为虚拟地址区域提交内存所需的物理页面总数。
	//
	DemandPages = 0;
	
	//
	// 如果不是系统内存则统计这段虚拟地址空缺的页表数量，因为系统地址空间全部安
	// 装了所需的页表而用户地址空间没有。
	//
	if (StartingVpn < MI_VA_TO_VPN(MM_SYSTEM_RANGE_START)) {

		for (Vpn = StartingVpn & ~((1 << PTI_BITS) -1); Vpn <= EndVpn; Vpn += (1 << PTI_BITS))  {

			if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid) {
				DemandPages++;
			}
		}
	}
	

	//
	// 统计这段虚拟地址的页面空洞数量。
	//
	for (Vpn = StartingVpn; Vpn <= EndVpn; Vpn++) {

		if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid ||
			0 == MiGetPteAddress(Vpn)->u.Hard.Valid) {
			DemandPages++;
		}
	}

	//
	// 如果没有足够的空闲物理页则返回失败。
	//
	if (DemandPages > MiGetAnyPageCount()) {
		return STATUS_NO_MEMORY;
	}

	for (Vpn = StartingVpn; Vpn <= EndVpn; Vpn++) {

		//
		// 如果页目录项或者页表项无效，说明当前地址没有映射物理内存。
		//
		if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid ||
			0 == MiGetPteAddress(Vpn)->u.Hard.Valid) {

			//
			// 如果页目录项无效则分配一物理页作为页表并设置页目录项。
			//
			if (0 == MiGetPdeAddress(Vpn)->u.Hard.Valid) {

				ASSERT(Vpn < MI_VA_TO_VPN(MM_SYSTEM_RANGE_START));
				
				Status = MiAllocateZeroedPages(1, &Pfn);
				ASSERT(EOS_SUCCESS(Status));
				
				MiMapPageTable(Vpn, Pfn);
			}

			//
			// 分配一个物理页并将之映射到虚拟页。
			//
			Status = MiAllocateZeroedPages(1, &Pfn);
			ASSERT(EOS_SUCCESS(Status));

			MiMapPage(Vpn, Pfn);

			//
			// 增加页表对应的有效PTE计数器。
			// 注意：系统地址空间从不使用PTE计数器，页表从不被卸载或安装。
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

功能描述：
	释放映射在连续虚拟页框上的物理页框。

参数：
	StartingVpn -- 起始虚页框号。
	EndVpn -- 结束虚页框号。

返回值：
	如果成功则返回STATUS_SCCESS，否则表示失败。

--*/
{
	ULONG_PTR Vpn;
	ULONG_PTR Pfn;

	//
	// 检查每个虚拟页，如果虚拟页映射了物理页，则取消映射并释放物理页。
	//
	for (Vpn = StartingVpn; Vpn <= EndVpn; Vpn++) {

		if (1 == MiGetPdeAddress(Vpn)->u.Hard.Valid &&
			1 == MiGetPteAddress(Vpn)->u.Hard.Valid) {

			Pfn = MiGetPteAddress(Vpn)->u.Hard.PageFrameNumber;
			MiUnmapPage(Vpn);
	
			MiFreePages(1, &Pfn);

			//
			// 减小PTE计数器，如果为0则卸载并回收页表。
			// 注意：系统地址空间的页表从不被卸载。
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

功能描述：
	检查如果访问指定的虚拟地址是否会引起访问违规异常，也就是检查虚拟地址是否映射
	了物理内存。

参数：
	VirtualAddress -- 虚拟地址。

返回值：
	如果读写访问地址不会引起异常则返回TRUE。

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

功能描述：
	得到虚拟地址对应的物理地址.

参数：
	VirtualAddress -- 虚拟地址。
	PhysicalAddress -- 指针，指向用于输出物理地址的缓冲区。

返回值：
	如果成功则返回STATUS_SUCESS。

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
