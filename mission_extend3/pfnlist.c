/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: pfnlist.c

描述: 物理内存管理，包括物理页的分配、回收以及零页线程。



*******************************************************************************/

#include "mi.h"

//
// 物理页总数量。
//
ULONG_PTR MiTotalPageFrameCount;

//
// 零页链表头以及零页数量。
//
ULONG_PTR MiZeroedPageListHead;
ULONG_PTR MiZeroedPageCount;

//
// 空闲页链表头以及空闲页数量。
//
ULONG_PTR MiFreePageListHead;
ULONG_PTR MiFreePageCount;

VOID
MiInitializePfnDatabase(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	)
/*++

功能描述：
	初始化页框号数据库。
	页框号数据库是一个MMPFN结构体数组，元素数量和物理页框数量一致，其中下标为
	n的MMPFN结构体对应于页框号为n的物理页框。MMPFN结构体中的PageState描述了对
	应的物理页框的状态，目前有三种状态ZEROED_PAGE、FREE_PAGE和BUSY_PAGE。结构
	体中的Next为下一个同状态页框的页框号，这样就构成了同状态页框链表。

参数：
	LoaderBlock -- 加载参数结构体指针。

返回值：
	无。

--*/
{
	ULONG_PTR i;

	MiTotalPageFrameCount = LoaderBlock->PhysicalMemorySize / PAGE_SIZE;

	//
	// 初始化物理页框号数据库。
	// 页框号数据库是一个MMPFN结构体数组，数组长度和物理页框数量一致，其中下标为
	// n的MMPFN结构体对应于页框号为n的物理页框。MMPFN结构体中的PageState描述了对
	// 应的物理页框的状态，目前有三种状态ZEROED_PAGE、FREE_PAGE和BUSY_PAGE。结构
	// 体中的Next为另一个同状态页框的页框号，这样就构成了同状态页框链表。
	//

	//
	// 初始化使用页对应的数据库项。
	//
	for (i = 0; i < LoaderBlock->FirstFreePageFrame; i++) {
		MiGetPfnDatabaseEntry(i)->PageState = BUSY_PAGE;
	}

	//
	// 初始化空闲页对应的数据库项，并将之组成链表。
	//
	MiFreePageListHead = LoaderBlock->FirstFreePageFrame;
	MiFreePageCount = MiTotalPageFrameCount - MiFreePageListHead;

	for (i = MiFreePageListHead; i < MiTotalPageFrameCount; i++) {
		MiGetPfnDatabaseEntry(i)->PageState = FREE_PAGE;
		MiGetPfnDatabaseEntry(i)->Next = i + 1;
	}

	MiGetPfnDatabaseEntry(i - 1)->Next = -1;

	//
	// 零页链表为空。
	//
	MiZeroedPageListHead = -1;
	MiZeroedPageCount = 0;
}


ULONG_PTR
MiGetAnyPageCount(
	VOID
	)
/*++

功能描述：
	得到可用的空闲物理页面的数量。

参数：
	无。

返回值：
	目前可用的空闲物理页面的数量。

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

功能描述：
	分配物理页。首先从空闲页链表中分配，如果空闲页链表不足则再从零页链表分配。

参数：
	NumberOfPages -- 期望分配的物理页的数量。
	PfnArray -- 指针，指向用于输出物理页框号的缓冲区。

返回值：
	如果成功则返回STATUS_SUCCESS，否则返回STATUS_NO_MEMORY。

--*/
{
	BOOL IntState;
	ULONG_PTR Pfn;
	ULONG_PTR i;

	IntState = KeEnableInterrupts(FALSE);

	if (NumberOfPages <= MiZeroedPageCount + MiFreePageCount) {

		//
		// 先从空闲链表中分配。
		//
		for (i = 0; i < NumberOfPages && MiFreePageCount > 0; i++) {

			Pfn = MiFreePageListHead;
			MiFreePageListHead = MiGetPfnDatabaseEntry(Pfn)->Next;
			MiFreePageCount--;

			MiGetPfnDatabaseEntry(Pfn)->PageState = BUSY_PAGE;

			PfnArray[i] = Pfn;
		}

		//
		// 如果空闲链表不足则继续从零页链表分配。
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

功能描述：
	首先从零页链表中分配，如果零页链表不足则再从空闲页
	链表分配（会先清零）。

参数：
	NumberOfPages -- 期望分配的物理页的数量。
	PfnArray -- 指针，指向用于输出物理页框号的缓冲区。

返回值：
	如果成功则返回STATUS_SUCCESS，否则返回STATUS_NO_MEMORY。

--*/
{
	BOOL IntState;
	ULONG_PTR Pfn;
	PVOID ZeroBuffer;
	ULONG_PTR i;

	IntState = KeEnableInterrupts(FALSE);

	if (NumberOfPages <= MiZeroedPageCount + MiFreePageCount) {

		//
		// 先从零页链表分配。
		//
		for (i = 0; i < NumberOfPages && MiZeroedPageCount > 0; i++) {

			Pfn = MiZeroedPageListHead;
			MiZeroedPageListHead = MiGetPfnDatabaseEntry(Pfn)->Next;
			MiZeroedPageCount--;

			MiGetPfnDatabaseEntry(Pfn)->PageState = BUSY_PAGE;

			PfnArray[i] = Pfn;
		}

		//
		// 如果零页链表不足，则继续从空闲页链表分配。
		//
		for (; i < NumberOfPages; i++) {

			Pfn = MiFreePageListHead;
			MiFreePageListHead = MiGetPfnDatabaseEntry(Pfn)->Next;
			MiFreePageCount--;

			MiGetPfnDatabaseEntry(Pfn)->PageState = BUSY_PAGE;

			//
			// 将物理页映射到系统PTE区域进行清零。
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

功能描述：
	释放物理页面。

参数：
	NumberOfPages -- 释放的页面数量。
	PfnArray -- 指向页框号缓冲区的指针。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	ULONG_PTR Pfn;
	ULONG_PTR i;

	//
	// 检查所有待回收的物理页，确保其为有效的已使用物理页。
	//
	for (i = 0; i < NumberOfPages; i++) {

		Pfn = PfnArray[i];

		if (Pfn >= MiTotalPageFrameCount || MiGetPfnDatabaseEntry(Pfn)->PageState != BUSY_PAGE) {
			ASSERT(FALSE);
			return STATUS_MEMORY_NOT_ALLOCATED;
		}
	}

	//
	// 修改这些物理页的状态为空闲，并将它们插入空闲页链表头部。
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
