/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: ptelist.c

描述: 系统 PTE 管理，实现了单页物理内存的快速映射。



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
	// 将系统PTE区域的所有PTE初始化为链表。
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

功能描述：
	将物理页框映射到系统PTE区域。

参数：
	Pfn -- 希望映射的物理页框的页框号。

返回值：
	映射后的虚拟地址。如果系统PTE已经用完则返回NULL（一般不会用完）。

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

功能描述：
	释放系统PTE。

参数：
	Va -- 由MiMapPageToSystemPte返回的虚拟地址。

返回值：
	无。

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
