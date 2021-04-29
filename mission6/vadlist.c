/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: vadlist.c

描述: 虚拟地址描述符链表管理模块。



*******************************************************************************/

#include "mi.h"

VOID
MiInitializeVadList(
	IN PMMVAD_LIST VadList, 
	IN PVOID StartingAddress, 
	IN PVOID EndAddress
	)
/*++

功能描述：
	初始化地址MMVAD_LIST。

参数：
	VadList -- 虚拟地址描述符链表指针。
	StartingAddress -- 起始虚拟地址。
	EndAddress -- 结束虚拟地址。

返回值：
	无。

--*/
{
	//
	// 确保地址范围有效且页面对齐。
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

功能描述：
	保留一段虚拟地址区域。

参数：
	VadList -- 虚拟地址描述符链表指针。
	BaseAddress --期望保留的地址区域的起始地址。
	RegionSize -- 期望保留的地址区域的大小。
	Vad -- 指针，指向用于输出虚拟地址描述符指针的缓冲区。

返回值：
	如果成功则返回STATUS_SUCCESS，否则失败。

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
		// 找到目标区域前的已保留区域P。
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
		// 如果P之后的已保留区域和目标区域重叠则返回失败。
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
		// 从地址空间的起始端向后查找第一个满足申请大小的未保留区域。
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
			// 跳过已保留区域向后移动。
			//
			StartingVpn = PointerVad->EndVpn + 1;
			EndVpn = StartingVpn + Size - 1;

			//
			// 如果地址范围溢出或者超出空间范围则失败！
			//
			if (EndVpn > VadList->EndVpn) {
				return STATUS_NO_MEMORY;
			}
		}
	}

	//
	// 从系统内存池中分配一个虚拟地址描述符结构体。
	//
	PointerVad = MmAllocateSystemPool(sizeof(MMVAD));

	if (NULL == PointerVad) {
		return STATUS_NO_MEMORY;
	}

	//
	// 将虚拟地址描述符插入已保留区域链表中。
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

功能描述：
	查找指定地址区域是否已经被保留。

参数：
	VadList -- 虚拟地址描述符链表指针。
	BaseAddress -- 查找目标的起始地址。
	RegionSize -- 查找目标的大小，为0时仅参考BaseAddress。
	Vad -- 指针，指向用于输出虚拟地址描述符指针的缓冲区。

返回值：
	如果成功则返回STATUS_SUCCESS，否则失败。

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

功能描述：
	释放保留地址区域。

参数：
	VadList -- 虚拟地址描述符链表指针。
	Vad -- 虚拟地址描述符指针。

返回值：
	无。

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

功能描述：
	清理VAD链表中的所有已保留区域，仅在销毁地址空间时被使用。

参数：
	VadList -- VAD链表指针。

返回值：
	无

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
