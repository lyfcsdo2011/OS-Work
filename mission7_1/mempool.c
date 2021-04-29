/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: mempool.c

描述: 内存池模块的实现。



*******************************************************************************/

#include "mi.h"

//
// 池中的内存块结构体。
//
typedef struct _MEM_BLOCK {
	USHORT K;
	USHORT Tag;
	ULONG Number;
	LIST_ENTRY ListEntry;					
}MEM_BLOCK, *PMEM_BLOCK;

VOID
PoolInitialize(
	IN PMEM_POOL Pool
	)
/*++

功能描述：
	初始化内存池结构体，初始化后内存池是空的。

参数：
	Pool -- 目标内存池结构体指针。

返回值：
	无。

--*/
{
	INT i;

	ASSERT(NULL != Pool);
	
	//
	// 初始化链表数组。
	//
	for(i = 0; i < 32; i++) {
		ListInitializeHead(&Pool->FreeListHeads[i]);
	}
}

STATUS
PoolCommitMemory(
	IN PMEM_POOL Pool,
	IN PVOID Address,
	IN SIZE_T Size
	)
/*++

功能描述：
	提交内存给内存池托管分配。

参数：
	Pool -- 内存池结构体指针。
	Address -- 提交托管的内存的基址。
	Size -- 提交托管内存的大小。

返回值：
	如果提交的内存有效则返回STATUS_SUCCESS，否则失败。

--*/
{
	ULONG k;
	PMEM_BLOCK Block;

	ASSERT(NULL != Pool);

	//
	// 提交给内存池托管的内存必须是8字节对齐的。
	//
	ASSERT(NULL != Address && IS_ALIGNED_TO_SIZE(Address, sizeof(QUAD)));
	ASSERT(0 != Size && IS_ALIGNED_TO_SIZE(Size, sizeof(QUAD)));

	//
	// 提交的大小至少要大于一个MEM_BLOCK，也不能溢出。
	//
	if (Size < sizeof(MEM_BLOCK) || Address + Size - 1 < Address) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 提交的内存不一定是2^k大小，递归将之划分为多个2^k大小的块，插入对应的链表。
	// 例如14MB内存可以划分为8MB、4MB、1MB、1MB共4块。
	//
	Block = (PMEM_BLOCK)Address;
	while(Size >= sizeof(MEM_BLOCK)) {

		//
		// 分割出一个大小为2^k的块并插入空闲队列k，k满足：2^k <= Size < 2^(k+1)
		//
		BitScanReverse(&k, Size);
		Block->K = (USHORT)k;
		Block->Tag = 0;
		Block->Number = 1;
		ListInsertTail(&Pool->FreeListHeads[k], &Block->ListEntry);

		//
		// 剩下的部分迭代分割。
		//
		Block = (PMEM_BLOCK)((ULONG_PTR)Block + (1 << k));
		Size = Size - (1 << k);
	}

	return STATUS_SUCCESS;
}

PVOID
PoolAllocateMemory(
	IN PMEM_POOL Pool,
	IN OUT PSIZE_T Size
	)
/*++

功能描述：
	从内存池中分配一块大小至少为Size的内存。

参数：
	Pool -- 内存池指针。
	Size -- 用作输入是输入期望分配的大小，用作输出时输出实际分配的大小。实际分配
			大小都大于等于期望大小，实际最少分配8字节，即输入值为0。

返回值：
	如果失败则返回NULL，否则返回内存块地址（至少4字节地址对齐）。

--*/
{
	ULONG k, i;
	SIZE_T AllocSize;
	PLIST_ENTRY ListEntry;
	PMEM_BLOCK Block;
	PMEM_BLOCK BuddyBlock;

	ASSERT(NULL != Pool);
	ASSERT(NULL != Size);

	//
	// 计算应分配块的大小，最小分配sizeof(LIST_ENTRY)。
	//
	if (*Size <= sizeof(LIST_ENTRY)) {
		AllocSize = sizeof(MEM_BLOCK);
	} else {
		AllocSize =  sizeof(MEM_BLOCK) - sizeof(LIST_ENTRY) + *Size;
	}

	BitScanReverse(&k, AllocSize); // 求满足：2^k <= AllocSize < 2^(k+1)的k值

	if (!IS_POWER_OF_2(AllocSize)) {
		k++; // 使：2^k >= AllocSize 成立
	}
	
	AllocSize = 1 << k; // BlockSize肯定满足需求大小了且是2的幂

	//
	// 查找块满足需求的、非空的、空闲块链表，如果不存在则返回NULL。
	//
	for (i = k; i < 32; i++) {
		if (!ListIsEmpty(&Pool->FreeListHeads[i])) {

			//
			// 从空闲块链表的首部取一个空闲块。
			//
			ListEntry = ListRemoveHead(&Pool->FreeListHeads[i]);
			Block = CONTAINING_RECORD(ListEntry, MEM_BLOCK, ListEntry);

			//
			// 对空闲快进行迭代拆分，直到大小刚好满足需求为止。
			// 例如需求一个2^4块，现在只有2^6块，可以拆分为1个2^5块和2个2^4块，
			// 将2^5块和其中1个2^4块插入对应的空闲链表，另外一个2^4块分配。
			//
			while (i-- > k) {

				Block->Number <<= 1; // 每对分一次编号乘2，相当于二叉树的节点ID。

				//
				// 将高地址的一半视为伙伴，将伙伴插入对应的空闲块链表中。
				//
				BuddyBlock = (PMEM_BLOCK)((ULONG_PTR)Block + (1 << i));
				BuddyBlock->Number = Block->Number + 1; // 右兄弟节点ID比左兄弟大1
				BuddyBlock->K = (USHORT)i;
				BuddyBlock->Tag = 0;

				ListInsertTail(&Pool->FreeListHeads[i], &BuddyBlock->ListEntry);
			}

			//
			// 记录分配块的K和Tag，Tag非0表示占用，设置为k值。
			//
			Block->K = (USHORT)k;
			Block->Tag = (USHORT)k;

			//
			// 返回分配的内存的地址，从ListEntry域开始，K、Tag、Number是额外开销，
			// 不能被使用，在释放回收内存时要使用这些参数。
			//
			*Size = AllocSize - (SIZE_T)&((PMEM_BLOCK)0)->ListEntry;
			return (PVOID)&Block->ListEntry;
		}
	}

	return NULL;
}

STATUS
PoolFreeMemory(
	IN PMEM_POOL Pool,
	IN PVOID Address
	)
/*++

功能描述：
	释放从内存池中分配的内存块。

参数：
	Pool -- 内存池结构体指针。
	Address -- 期望释放的内存块的地址。

返回值：
	如果成功则返回STATUS_SUCCESS，否则返回STATUS_MEMORY_NOT_ALLOCATED。

--*/
{
	PLIST_ENTRY ListEntry;
	PMEM_BLOCK Block;
	PMEM_BLOCK BuddyBlock;
	PMEM_BLOCK MergedBlock;

	ASSERT(NULL != Pool);
	ASSERT(NULL != Address);

	//
	// 根据地址反推MEM_BLOCK结构地址，检查Tag，确保是一个有效的已分配块。
	//
	Block = CONTAINING_RECORD(Address, MEM_BLOCK, ListEntry);

	if (Block->Tag != Block->K || 0 == Block->K || Block->K > 31) {
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	//
	// 标记空闲块。
	//
	Block->Tag = 0;

	//
	// 如果块的编号大于1，则说明可能存在空闲伙伴块，有则合并。
	//
	while(Block->Number > 1) {

		//
		// 如编号为奇数，则其空闲伙伴块一定在其左侧，否则一定在其右侧。
		//
		if(Block->Number & 1) {
			BuddyBlock = (PMEM_BLOCK)((ULONG_PTR)Block - (1 << Block->K));
			MergedBlock = BuddyBlock;
		} else {
			BuddyBlock = (PMEM_BLOCK)((ULONG_PTR)Block + (1 << Block->K));
			MergedBlock = Block;
		}

		//
		// 伙伴之间的编号应相差1并且大小相同，否则说明不存在伙伴(伙伴被拆分了)。
		//
		if ((Block->Number ^ BuddyBlock->Number) != 1 || Block->K != BuddyBlock->K ) {
			break;
		}

		//
		// 如果伙伴存在但不空闲，同样不能合并。
		//
		if(0 != BuddyBlock->Tag) {
			break;
		}
		
		//
		// 将空闲伙伴块从空闲块链表中取出。
		//
		ListRemoveEntry(&BuddyBlock->ListEntry);
		
		//
		// 设置合并块的大小和编号。
		//
		MergedBlock->K += 1;
		MergedBlock->Number >>= 1;

		//
		// 合并后的空闲块仍然可能有空闲伙伴存在，进行迭代合并操作。
		//
		Block = MergedBlock;
	}

	//
	// 空闲块插入对应的空闲链表中。
	//
	ListInsertTail(&Pool->FreeListHeads[Block->K], &Block->ListEntry);
	
	return STATUS_SUCCESS;
}
