/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: mi386.h

描述: i386 内存相关。



*******************************************************************************/

#ifndef _MI386_
#define _MI386_

#include "rtl.h"

/*++

    进程 4G 虚拟地址空间在 I386 平台上的布局:

                 +------------------------------------+
        00000000 | 不可访问的64KB缓冲区域。用于捕捉对 |
				 | 空指针的非法访问。				  |
                 +------------------------------------+
        00010000 |                                    |
                 |                                    |
                 | 用户进程的可用的虚拟地址空间。     |
				 | 用户进程的可执行映像被加载到地址	  |
				 | 0x400000处，用户进程的堆和栈位于	  |
				 | 这段区域内，用户进程调用			  |
				 | VirtualAlloc进行虚拟内存分配时，也 |
				 | 从这里分配。						  |
                 |                                    |
                 |                                    |
                 +------------------------------------+
        7FFF0000 | 不可访问的64KB缓冲区域。		      |
                 +------------------------------------+
        80000000 |                                    |
                 | 物理内存的约1/8被映射到此处，映射总|
				 | 量不超过256MB，且向上4M地址对齐。  |
				 | 被映射的物理内存使用如下：		  |	
				 |   0-64KB：保留未用；               |
				 |   64KB-640KB：用于加载kernel.dll； |
				 |   640KB-1MB：BIOS区域，不可用。	  |
				 |	 1MB-?：用于页框号数据库，大小根  |
				 |			据物理页面数量而定。	  |
                 |   ? - ?: 系统内存池。			  |
				 |   ? - ?: 最后8K用作ISR专用栈。	  |
                 |                                    |
                 +------------------------------------+
		90000000 |                                    |
                 |									  |
                 | 保留未用。						  |
				 |                                    |
                 |                                    |
                 +------------------------------------+
		A0000000 |                                    |
                 | 供系统动态管理分配的虚拟地址空间， |
				 | 这段区域的大小为物理内存大小的1/4, |
				 | 最大512MB。						  |
				 | 这段区域在初始化时，需全部安装PTE，|
				 | 避免各进程的页目录项不一致。		  |
                 |                                    |
                 +------------------------------------+
        C0000000 |                                    |
                 | 进程页表被映射到此4MB区域。		  |
                 |                                    |
                 +------------------------------------+
		C0400000 | 页表有效PTE计数器映射到此4K区域。  |
				 +------------------------------------+
		C0401000 |									  |
				 |									  |
				 | 系统PTE区域，用于快速将单个物理页  |
				 | 框映射至此，对物理内存进行初始化。 |
				 |									  |
				 |									  |
                 +------------------------------------+
        C0800000 |                                    |
                 |                                    |
                 |                                    |
				 |									  |
				 | 保留未用。						  |
                 |									  |
                 |									  |
				 |                                    |
        FFFFFFFF |                                    |
                 +------------------------------------+

--*/

//
// 引导参数块。
//
typedef struct _LOADER_PARAMETER_BLOCK {
	ULONG PhysicalMemorySize;
	ULONG MappedMemorySize;
	ULONG SystemVirtualBase;
	ULONG PageTableVirtualBase;
	ULONG FirstFreePageFrame;
	ULONG ImageVirtualBase;
	ULONG ImageSize;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

//
// I386 PTE \ PDE.
//
typedef struct _MMPTE_HARDWARE
{
	ULONG Valid : 1;			// 存在位
	ULONG Writable : 1;			// 可写标志
	ULONG User : 1;				// 用户页标志
	ULONG WriteThrough : 1;		// 穿透写标志
	ULONG CacheDisable : 1;		// 缓存使用标志
	ULONG Accessed : 1;			// 已访问标志
	ULONG Dirty : 1;			// 脏页标志
	ULONG LargePage : 1;		// 大页面标志
	ULONG Global : 1;			// 全局页标志
	ULONG Unused : 3;
	ULONG PageFrameNumber : 20;	// 页框号
}MMPTE_HARDWARE, *PMMPTE_HARDWARE;

//
// 虚拟地址右移PTI_SHIFT位可得到页框号。
//
#define PAGE_SHIFT 12

//
// 虚拟地址右移PDI_SHIFT位可得到页目录项索引。
//
#define PDI_SHIFT 22

//
// 页目录索引占10位
//
#define PDI_BITS 10

//
// 虚拟地址右移PTI_SHIFT位可得到页表项索引。
//
#define PTI_SHIFT 12

//
// 页表索引占10位
//
#define PTI_BITS 10

//
// 页目录中有 1024 个 PDE，页表中有 1024 个 PTE
//
#define PTE_PER_TABLE	0x400

//
// 页目录项和页表项都是 4 字节
//
#define PTE_SIZE		0x4

//
// 内存管理器使用的PTE。
//
typedef struct _MMPTE
{
	union{
		ULONG Long;
		MMPTE_HARDWARE Hard;
	}u;
}MMPTE, *PMMPTE;

//
// 页框号(Page Frame Number)数据库项结构体。
//
typedef struct _MMPFN
{
	ULONG Unused : 9;
	ULONG PageState : 3;
	ULONG Next : 20;
}MMPFN, *PMMPFN;

//
// 内存布局参数定义。
//
#define PFN_DATABASE				((PMMPFN)0x80100000)

#define MI_SYSTEM_VM_BASE			((PVOID)0xA0000000)

#define MI_SYSTEM_PTE_BASE			((PULONG_PTR)0xC0400000)

#define PTE_COUNTER_DATABASE		((PULONG_PTR)0xC0400000)

#define MI_START_VPN_OF_SYSTEM_PTE	0xC0401

#define MI_END_VPN_OF_SYSTEM_PTE	0xC04FF

#define PDE_BASE					((PMMPTE)0xC0300000)

#define PTE_BASE					((PMMPTE)0xC0000000)


//
// 得到虚拟地址对应的页目录项。
//
#define MiGetPdeAddressByVa(va)  (PDE_BASE + (((ULONG)(va)) >> PDI_SHIFT))

//
// 得到虚拟地址对应的页表项。
//
#define MiGetPteAddressByVa(va) (PTE_BASE + (((ULONG)(va)) >> PTI_SHIFT))

//
// 得到虚页框对应的页目录项。
//
#define MiGetPdeAddress(vpn)  (PDE_BASE + ((vpn) >> PTI_BITS))

//
// 得到虚页框对应的页表项。
//
#define MiGetPteAddress(vpn) (PTE_BASE + (vpn))

//
// 增加虚页框对应的PTE计数器并返回之前的值。
//
#define MiIncPteCounter(vpn) (PTE_COUNTER_DATABASE[(vpn) >> PTI_BITS]++)

//
// 减小虚页框对应的PTE计数器并返回之前的值。
//
#define MiDecPteCounter(vpn) (PTE_COUNTER_DATABASE[(vpn) >> PTI_BITS]--)

//
// 得到页框号对应的页框号数据库项。
//
#define MiGetPfnDatabaseEntry(pfn) (PFN_DATABASE + pfn)

//
// 将虚页映射到物理页。
//
#define MiMapPage(vpn, pfn) \
	do { \
		MiGetPteAddress(vpn)->u.Hard.PageFrameNumber = pfn; \
		MiGetPteAddress(vpn)->u.Hard.Writable = 1; \
		MiGetPteAddress(vpn)->u.Hard.Valid = 1; \
		MiFlushSingleTlb(MI_VPN_TO_VA(vpn));\
	} while (0)

//
// 取消虚页对物理页的映射。
//
#define MiUnmapPage(vpn) \
	do { \
		MiGetPteAddress(vpn)->u.Long = 0; \
		MiFlushSingleTlb(MI_VPN_TO_VA(vpn)); \
	} while (0)

//
// 为虚拟地址安装一个页表。
//
#define MiMapPageTable(vpn, pfn) \
	do {\
		MiGetPdeAddress(vpn)->u.Hard.PageFrameNumber = pfn; \
		MiGetPdeAddress(vpn)->u.Hard.Writable = 1; \
		MiGetPdeAddress(vpn)->u.Hard.Valid = 1; \
		MiFlushSingleTlb(MiGetPteAddress(vpn)); \
	} while (0)

//
// 卸载虚拟地址的页表。
//
#define MiUnmapPageTable(vpn) \
	do { \
		MiGetPdeAddress(vpn)->u.Long = 0; \
		MiFlushSingleTlb(MiGetPteAddress(vpn)); \
	} while (0);

//
// 写页目录寄存器。
//
VOID
MiSetPageDirectory(
	ULONG_PTR Pfn
	);

//
// 刷新TLB(Translation Lookaside Buffer)。
//
VOID
MiFlushSingleTlb(
	IN PVOID VirtualAddress
	);

VOID
MiFlushEntireTlb(
	VOID
	);

#endif // _MI386_
