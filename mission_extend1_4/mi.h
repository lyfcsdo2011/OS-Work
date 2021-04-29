/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: mi.h

描述: 内存管理器内部头文件。



*******************************************************************************/


#ifndef _MI_
#define _MI_

#include "ke.h"
#include "mm.h"

#ifdef _I386
#include "mi386.h"
#endif

#include "rtl.h"

//
// 得到地址所在页框的页框号。
//
#define MI_VA_TO_VPN(va)  ((ULONG_PTR)(va) >> PAGE_SHIFT)

//
// 由页框号得到页框的起始地址。
//
#define MI_VPN_TO_VA(vpn)  (PVOID)((vpn) << PAGE_SHIFT)

//
// 由页框号得到页框的结束地址。
//
#define MI_VPN_TO_VA_ENDING(vpn)  (PVOID)(((vpn) << PAGE_SHIFT) | (PAGE_SIZE - 1))

//
// 物理页状态定义。
//
typedef enum _PAGE_STATE {
	ZEROED_PAGE,	// 0
	FREE_PAGE,		// 1
	BUSY_PAGE,		// 2
} PAGE_STATE;

//
// 虚拟地址区域描述结构体。
//
typedef struct _MMVAD{
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;
	LIST_ENTRY VadListEntry;
}MMVAD, *PMMVAD;


typedef struct _MMVAD_LIST{
	ULONG_PTR StartingVpn;
	ULONG_PTR EndVpn;
	LIST_ENTRY VadListHead;
}MMVAD_LIST, *PMMVAD_LIST;

//
// 进程地址空间(Process Address VadList)结构体。
//
typedef struct _MMPAS {

	//
	// 可以给分配给进程使用的地址空间。
	//
	MMVAD_LIST VadList;

	//
	// 页目录和PTE计数器数据库的页框号。
	//
	ULONG_PTR PfnOfPageDirectory;
	ULONG_PTR PfnOfPteCounter;
}MMPAS;

//
// 内存池结构体。 
//
typedef struct _MEM_POOL {
	LIST_ENTRY FreeListHeads[32];
}MEM_POOL, *PMEM_POOL;

//
// 物理页总数量。
//
extern ULONG_PTR MiTotalPageFrameCount;
extern ULONG_PTR MiZeroedPageCount;
extern ULONG_PTR MiFreePageCount;

//
// 系统进程地址空间。
//
extern MMPAS MiSystemPas;

//
// 当前进程地址空间。
//
extern volatile PMMPAS MiCurrentPas;

//
// 初始化页框号数据库。
//
VOID
MiInitializePfnDatabase(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	);

//
// 得到可用的物理页面数量。
//
ULONG_PTR
MiGetAnyPageCount(
	VOID
	);

//
// 分配物理页。
//
STATUS
MiAllocateAnyPages(
	IN ULONG_PTR NumberOfPages,
	OUT PULONG_PTR PfnArray
	);

//
// 分配经过零初始化的物理页。
//
STATUS
MiAllocateZeroedPages(
	IN ULONG_PTR NumberOfPages,
	OUT PULONG_PTR PfnArray
	);

//
// 释放物理页。
//
STATUS
MiFreePages(
	IN ULONG_PTR NumberOfPages,
	IN PULONG_PTR PfnArray
	);

//
// 初始化内存池结构体。
//
VOID
PoolInitialize(
	IN PMEM_POOL Pool
	);

//
// 提交内存给内存池托管
//
STATUS
PoolCommitMemory(
	IN PMEM_POOL Pool,
	IN PVOID Address,
	IN SIZE_T Size
	);

//
// 从内存池分配内存。
//
PVOID
PoolAllocateMemory(
	IN PMEM_POOL Pool,
	IN OUT PSIZE_T Size
	);

//
// 释放由内存池分配的内存块。
//
STATUS
PoolFreeMemory(
	IN PMEM_POOL Pool,
	IN PVOID Address
	);

//
// 初始化系统内存池。
//
VOID
MiInitializeSystemPool(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	);

//
// 初始化系统PTE。
//
VOID
MiInitialzieSystemPte(
	VOID
	);

//
// 将物理页框映射到系统PTE区域。
//
PVOID
MiMapPageToSystemPte(
	IN ULONG_PTR Pfn
	);

//
// 释放系统PTE。
//
VOID
MiFreeSystemPte(
	IN PVOID Va 
	);

//
// 初始化系统进程地址空间。
//
VOID
MiInitializeSystemPas(
	VOID
	);

//
// 初始化MMVAD_LIST结构体。
//
VOID
MiInitializeVadList(
	IN PMMVAD_LIST VadList, 
	IN PVOID StartingAddress, 
	IN PVOID EndAddress
	);

//
// 保留一段虚拟地址区域。
//
STATUS
MiReserveAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PVOID BaseAddress,
	IN SIZE_T RegionSize,
	OUT PMMVAD *Vad
	);

//
// 查找已保留的虚拟地址区域。
//
STATUS
MiFindReservedAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PVOID BaseAddress,
	IN SIZE_T RegionSize,
	OUT PMMVAD *Vad
	);

//
// 释放虚拟地址区域。
//
VOID
MiFreeAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PMMVAD Vad
	);

//
// 清理VAD链表中的所有已保留区域。
//
VOID
MiCleanAddressRegion(
	IN PMMVAD_LIST VadList
	);

#endif // _MI_
