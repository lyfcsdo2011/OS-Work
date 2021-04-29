/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: mm.h

描述: 内存管理模块公开接口头文件。



*******************************************************************************/

#ifndef _MM_
#define _MM_

#include "eosdef.h"

//
// 进程地址空间结构体指针类型的声明。
//
typedef struct _MMPAS *PMMPAS;

//
// 定义页面大小。
//
#ifdef _I386

#define PAGE_SIZE 0x1000 // 4096

#define MM_LOWEST_USER_ADDRESS		(PVOID)0x00010000

#define MM_HIGHEST_USER_ADDRESS		(PVOID)0x7FFEFFFF

#define MM_SYSTEM_RANGE_START		(PVOID)0x80000000

#define MM_KERNEL_IMAGE_BASE		(PVOID)0x80010000

#define MM_ISR_STACK_SIZE			(ULONG_PTR)0x2000

#endif

//
// Size向上圆整到页面大小的整数倍。
//
#define ROUND_TO_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

//
// Size个字节占用多少个页面。
//
#define BYTES_TO_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> PAGE_SHIFT) + \
							   (((ULONG)(Size) & (PAGE_SIZE - 1)) != 0))

//
// 虚拟地址相对页面边界的偏移量。
//
#define BYTE_OFFSET(va) ((LONG_PTR)(va) & (PAGE_SIZE - 1))


//
// 虚拟地址对齐到页面边界。
//
#define PAGE_ALIGN(va) ((PVOID)((ULONG_PTR)(va) & ~(PAGE_SIZE - 1)))

//
// 虚拟地址是否对齐到页面边界。
//
#define IS_PAGE_ALIGNED(va) (0 == ((ULONG_PTR)(va) & (PAGE_SIZE - 1)))


//
// 初始化内存管理模块第一步。
//
VOID
MmInitializeSystem1(
	IN PVOID LoaderBlock
	);

//
// 初始化内存管理模块第二步。
//
VOID
MmInitializeSystem2(
	VOID
	);

//
// 从系统内存池中分配内存。
//
PVOID
MmAllocateSystemPool(
	IN SIZE_T nSize
	);

//
// 释放系统内存池中的内存。
//
STATUS
MmFreeSystemPool(
	IN PVOID pMem
	);

//
// 得到系统进程地址空间。
//
PMMPAS
MmGetSystemProcessAddressSpace(
	VOID
	);

//
// 创建一个进程地址空间。
//
PMMPAS
MmCreateProcessAddressSpace(
	VOID
	);

//
// 销毁进程地址空间。
//
VOID
MmDeleteProcessAddressSpace(
	IN PMMPAS Pas
	);

//
// 换入进程地址空间。
//
PMMPAS
MmSwapProcessAddressSpace(
	IN PMMPAS NewPas
	);

//
// 在当前进程地址空间中分配虚拟内存。
//
STATUS
MmAllocateVirtualMemory(
	IN OUT PVOID *BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG AllocationType,
	IN BOOL SystemVirtual
	);

//
// 释放当前进程地址空间中的虚拟内存。
//
STATUS
MmFreeVirtualMemory(
	IN OUT PVOID *BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG FreeType,
	IN BOOL SystemVirtual
	);

//
// 清理当前进程地址空间中所有已经分配的虚拟内存。
//
VOID
MmCleanVirtualMemory(
	VOID
	);

//
// 得到当前地址空间中虚拟地址对应的物理地址。
//
STATUS
MmGetPhysicalAddress(
	IN PVOID VirtualAddress,
	OUT PVOID* PhysicalAddress
	);

#endif // _MM_
