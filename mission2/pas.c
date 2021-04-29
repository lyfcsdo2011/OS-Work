/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: pas.c

描述: 进程地址空间的创建、删除和切换。



*******************************************************************************/

#include "mi.h"

//
// 系统进程地址空间。
//
MMPAS MiSystemPas;

//
// 当前进程地址空间 
//
volatile PMMPAS MiCurrentPas = &MiSystemPas;

VOID
MiInitializeSystemPas(
	VOID
	)
/*++

功能描述：
	初始化系统进程地址空间。
	系统进程地址空间中，从MI_SYSTEM_VM_BASE开始的区域可进行虚拟内存分配，其大小
	为物理内存的1/4，最大不超过512MB，且向上4MB取整。这个区域位于所有进程地址空
	间都共享的系统地址空间中，为了使各个进程页目录中用于映射共享系统地址空间的
	页目录项都保持一致，这段区域全部安装了页表，在分配、回收虚拟内存时不进行页
	表的动态安装、卸载。

参数：
	无。

返回值：
	无。

--*/
{
	ULONG_PTR n;
	ULONG_PTR pfn;
	ULONG_PTR vpn;

	//
	// 计算系统地址空间所需的页表数量。
	//
	n = (MiTotalPageFrameCount + 4095) / 4096;

	if (n > 128) {
		n = 128;
	}

	//
	// 初始化可分配虚拟内存的虚拟地址空间描述符链表。
	//
	MiInitializeVadList( &MiSystemPas.VadList,
						 MI_SYSTEM_VM_BASE,
						 MI_SYSTEM_VM_BASE + (n << PDI_SHIFT) - 1);

	//
	// 为可分配虚拟内存区域全部安装页表。
	//
	vpn = MI_VA_TO_VPN(MI_SYSTEM_VM_BASE);

	for (; n > 0; n--) {

		MiAllocateZeroedPages(1, &pfn);
		MiMapPageTable(vpn, pfn);

		//
		// 一个页目录包含 (1 << PDI_BITS) 个页目录项
		//
		vpn += 1 << PDI_BITS;
	}

	MiSystemPas.PfnOfPageDirectory = MiGetPdeAddressByVa(PTE_BASE)->u.Hard.PageFrameNumber;
	MiSystemPas.PfnOfPteCounter = 0;
}

PMMPAS
MmGetSystemProcessAddressSpace(
	VOID
	)
/*++

功能描述：
	得到系统进程地址空间。

参数：
	无。

返回值：
	系统进程地址空间指针。

--*/
{
	return &MiSystemPas;
}

PMMPAS
MmCreateProcessAddressSpace(
	VOID
	)
/*++

功能描述：
	分配一个进程地址空间。

参数：
	无。

返回值：
	如果成功则返回新创建的进程地址空间描述结构体的指针，否则返回NULL。

--*/
{
	PMMPAS Pas;
	PMMPTE PageDirectory;
	ULONG_PTR PfnArray[2];
	ULONG_PTR i;

	//
	// 从系统内存池中分配MMPAS结构体。
	//
	Pas = (PMMPAS)MmAllocateSystemPool(sizeof(MMPAS));

	if (NULL == Pas) {
		return NULL;
	}

	//
	// 分配两个物理页框，分别用作页目录和PTE计数器数据库。
	//
	if (!EOS_SUCCESS(MiAllocateZeroedPages(2, PfnArray))) {

		MmFreeSystemPool(Pas);

		return NULL;
	}

	//
	// 初始化可分配虚拟内存的虚拟地址空间描述符链表。
	//
	MiInitializeVadList( &Pas->VadList,
						 MM_LOWEST_USER_ADDRESS,
						 MM_HIGHEST_USER_ADDRESS );

	Pas->PfnOfPageDirectory = PfnArray[0];
	Pas->PfnOfPteCounter = PfnArray[1];

	//
	// 将页目录映射到系统PTE区域，以对之进行初始化。
	//
	PageDirectory = (PMMPTE)MiMapPageToSystemPte(Pas->PfnOfPageDirectory);

	//
	// 使进程共享系统地址空间，拷贝用于映射系统地址空间的页目录项。
	//
	for (i = ((ULONG_PTR)MM_SYSTEM_RANGE_START >> PDI_SHIFT); i <= (MAXULONG_PTR >> PDI_SHIFT); i++) {
		PageDirectory[i] = PDE_BASE[i];
	}
	
	//
	// 页目录本身也是一张页表，用于映射进程的所有页表到PTE_BASE处。
	//
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Long = 0;
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Hard.PageFrameNumber = Pas->PfnOfPageDirectory;
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Hard.Writable = 1;
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Hard.Valid = 1;

	//
	// 页目录初始化完毕，释放系统PTE。
	//
	MiFreeSystemPte(PageDirectory);

	return Pas;
}

VOID
MmDeleteProcessAddressSpace(
	IN PMMPAS Pas
	)
/*++

功能描述：
	删除进程地址空间。

参数：
	Pas -- 进程地址空间描述结构体指针。

返回值：
	无。

注意：
	当前进程地址空间和系统地址空间不可以被删除。如果要删除一个进程地址空间且其是
	当前地址空间，可以先将之换出再删除之。

--*/
{
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	//
	// 不可以删除当前进程地址空间和系统地址空间。
	//
	ASSERT(Pas != MiCurrentPas && Pas != &MiSystemPas);

	//
	// 进程地址空间应该被清理过。
	//
	ASSERT(ListIsEmpty(&Pas->VadList.VadListHead));

	//
	// 释放页目录和MMPAS结构体。
	//
	MiFreePages(1, &Pas->PfnOfPageDirectory);
	MiFreePages(1, &Pas->PfnOfPteCounter);

	MmFreeSystemPool(Pas);

	KeEnableInterrupts(IntState);
}

PMMPAS
MmSwapProcessAddressSpace(
	IN PMMPAS NewPas
	)
/*++

功能描述：
	换入进程地址空间，如果进程地址空间已经被换入则不进行任何操作。任意一个时刻，
	只有一个进程地址空间处于换入状态。只有进程地址空间处于换入状态时，才能对进程
	的用户地址空间进行读写访问。

参数：
	NewPas -- 期望被换入的进程地址空间的描述结构体的指针。

返回值：
	无。

注意：
	这个函数只能在中断环境下被调用，因为在进程环境下，当前进程的地址空间是不可以
	被换出的。

--*/
{
	BOOL IntState;
	PMMPAS OldPas;

	IntState = KeEnableInterrupts(FALSE);

	OldPas = MiCurrentPas;

	if (NewPas != MiCurrentPas) {

		MiSetPageDirectory(NewPas->PfnOfPageDirectory);

		//
		// 将PTE计数器页映射至PTE_COUNTER_DATABASE处。
		// 注意：系统进程地址空间不使用PTE计数器，因为系统进程地址空间中可分配虚
		// 拟内存的页表是固定的，不进行动态的安装、卸载。
		//
		if (NewPas != &MiSystemPas) {
			MiMapPage(MI_VA_TO_VPN(PTE_COUNTER_DATABASE), NewPas->PfnOfPteCounter);
		} else {
			MiUnmapPage(MI_VA_TO_VPN(PTE_COUNTER_DATABASE));
		}

		MiCurrentPas = NewPas;
	}

	KeEnableInterrupts(IntState);

	return OldPas;
}
