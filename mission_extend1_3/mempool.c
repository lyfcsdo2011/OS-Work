/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: mempool.c

����: �ڴ��ģ���ʵ�֡�



*******************************************************************************/

#include "mi.h"

//
// ���е��ڴ��ṹ�塣
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

����������
	��ʼ���ڴ�ؽṹ�壬��ʼ�����ڴ���ǿյġ�

������
	Pool -- Ŀ���ڴ�ؽṹ��ָ�롣

����ֵ��
	�ޡ�

--*/
{
	INT i;

	ASSERT(NULL != Pool);
	
	//
	// ��ʼ���������顣
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

����������
	�ύ�ڴ���ڴ���йܷ��䡣

������
	Pool -- �ڴ�ؽṹ��ָ�롣
	Address -- �ύ�йܵ��ڴ�Ļ�ַ��
	Size -- �ύ�й��ڴ�Ĵ�С��

����ֵ��
	����ύ���ڴ���Ч�򷵻�STATUS_SUCCESS������ʧ�ܡ�

--*/
{
	ULONG k;
	PMEM_BLOCK Block;

	ASSERT(NULL != Pool);

	//
	// �ύ���ڴ���йܵ��ڴ������8�ֽڶ���ġ�
	//
	ASSERT(NULL != Address && IS_ALIGNED_TO_SIZE(Address, sizeof(QUAD)));
	ASSERT(0 != Size && IS_ALIGNED_TO_SIZE(Size, sizeof(QUAD)));

	//
	// �ύ�Ĵ�С����Ҫ����һ��MEM_BLOCK��Ҳ���������
	//
	if (Size < sizeof(MEM_BLOCK) || Address + Size - 1 < Address) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// �ύ���ڴ治һ����2^k��С���ݹ齫֮����Ϊ���2^k��С�Ŀ飬�����Ӧ������
	// ����14MB�ڴ���Ի���Ϊ8MB��4MB��1MB��1MB��4�顣
	//
	Block = (PMEM_BLOCK)Address;
	while(Size >= sizeof(MEM_BLOCK)) {

		//
		// �ָ��һ����СΪ2^k�Ŀ鲢������ж���k��k���㣺2^k <= Size < 2^(k+1)
		//
		BitScanReverse(&k, Size);
		Block->K = (USHORT)k;
		Block->Tag = 0;
		Block->Number = 1;
		ListInsertTail(&Pool->FreeListHeads[k], &Block->ListEntry);

		//
		// ʣ�µĲ��ֵ����ָ
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

����������
	���ڴ���з���һ���С����ΪSize���ڴ档

������
	Pool -- �ڴ��ָ�롣
	Size -- ����������������������Ĵ�С���������ʱ���ʵ�ʷ���Ĵ�С��ʵ�ʷ���
			��С�����ڵ���������С��ʵ�����ٷ���8�ֽڣ�������ֵΪ0��

����ֵ��
	���ʧ���򷵻�NULL�����򷵻��ڴ���ַ������4�ֽڵ�ַ���룩��

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
	// ����Ӧ�����Ĵ�С����С����sizeof(LIST_ENTRY)��
	//
	if (*Size <= sizeof(LIST_ENTRY)) {
		AllocSize = sizeof(MEM_BLOCK);
	} else {
		AllocSize =  sizeof(MEM_BLOCK) - sizeof(LIST_ENTRY) + *Size;
	}

	BitScanReverse(&k, AllocSize); // �����㣺2^k <= AllocSize < 2^(k+1)��kֵ

	if (!IS_POWER_OF_2(AllocSize)) {
		k++; // ʹ��2^k >= AllocSize ����
	}
	
	AllocSize = 1 << k; // BlockSize�϶����������С������2����

	//
	// ���ҿ���������ġ��ǿյġ����п���������������򷵻�NULL��
	//
	for (i = k; i < 32; i++) {
		if (!ListIsEmpty(&Pool->FreeListHeads[i])) {

			//
			// �ӿ��п�������ײ�ȡһ�����п顣
			//
			ListEntry = ListRemoveHead(&Pool->FreeListHeads[i]);
			Block = CONTAINING_RECORD(ListEntry, MEM_BLOCK, ListEntry);

			//
			// �Կ��п���е�����֣�ֱ����С�պ���������Ϊֹ��
			// ��������һ��2^4�飬����ֻ��2^6�飬���Բ��Ϊ1��2^5���2��2^4�飬
			// ��2^5�������1��2^4������Ӧ�Ŀ�����������һ��2^4����䡣
			//
			while (i-- > k) {

				Block->Number <<= 1; // ÿ�Է�һ�α�ų�2���൱�ڶ������Ľڵ�ID��

				//
				// ���ߵ�ַ��һ����Ϊ��飬���������Ӧ�Ŀ��п������С�
				//
				BuddyBlock = (PMEM_BLOCK)((ULONG_PTR)Block + (1 << i));
				BuddyBlock->Number = Block->Number + 1; // ���ֵܽڵ�ID�����ֵܴ�1
				BuddyBlock->K = (USHORT)i;
				BuddyBlock->Tag = 0;

				ListInsertTail(&Pool->FreeListHeads[i], &BuddyBlock->ListEntry);
			}

			//
			// ��¼������K��Tag��Tag��0��ʾռ�ã�����Ϊkֵ��
			//
			Block->K = (USHORT)k;
			Block->Tag = (USHORT)k;

			//
			// ���ط�����ڴ�ĵ�ַ����ListEntry��ʼ��K��Tag��Number�Ƕ��⿪����
			// ���ܱ�ʹ�ã����ͷŻ����ڴ�ʱҪʹ����Щ������
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

����������
	�ͷŴ��ڴ���з�����ڴ�顣

������
	Pool -- �ڴ�ؽṹ��ָ�롣
	Address -- �����ͷŵ��ڴ��ĵ�ַ��

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS�����򷵻�STATUS_MEMORY_NOT_ALLOCATED��

--*/
{
	PLIST_ENTRY ListEntry;
	PMEM_BLOCK Block;
	PMEM_BLOCK BuddyBlock;
	PMEM_BLOCK MergedBlock;

	ASSERT(NULL != Pool);
	ASSERT(NULL != Address);

	//
	// ���ݵ�ַ����MEM_BLOCK�ṹ��ַ�����Tag��ȷ����һ����Ч���ѷ���顣
	//
	Block = CONTAINING_RECORD(Address, MEM_BLOCK, ListEntry);

	if (Block->Tag != Block->K || 0 == Block->K || Block->K > 31) {
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	//
	// ��ǿ��п顣
	//
	Block->Tag = 0;

	//
	// �����ı�Ŵ���1����˵�����ܴ��ڿ��л��飬����ϲ���
	//
	while(Block->Number > 1) {

		//
		// ����Ϊ������������л���һ��������࣬����һ�������Ҳࡣ
		//
		if(Block->Number & 1) {
			BuddyBlock = (PMEM_BLOCK)((ULONG_PTR)Block - (1 << Block->K));
			MergedBlock = BuddyBlock;
		} else {
			BuddyBlock = (PMEM_BLOCK)((ULONG_PTR)Block + (1 << Block->K));
			MergedBlock = Block;
		}

		//
		// ���֮��ı��Ӧ���1���Ҵ�С��ͬ������˵�������ڻ��(��鱻�����)��
		//
		if ((Block->Number ^ BuddyBlock->Number) != 1 || Block->K != BuddyBlock->K ) {
			break;
		}

		//
		// ��������ڵ������У�ͬ�����ܺϲ���
		//
		if(0 != BuddyBlock->Tag) {
			break;
		}
		
		//
		// �����л���ӿ��п�������ȡ����
		//
		ListRemoveEntry(&BuddyBlock->ListEntry);
		
		//
		// ���úϲ���Ĵ�С�ͱ�š�
		//
		MergedBlock->K += 1;
		MergedBlock->Number >>= 1;

		//
		// �ϲ���Ŀ��п���Ȼ�����п��л����ڣ����е����ϲ�������
		//
		Block = MergedBlock;
	}

	//
	// ���п�����Ӧ�Ŀ��������С�
	//
	ListInsertTail(&Pool->FreeListHeads[Block->K], &Block->ListEntry);
	
	return STATUS_SUCCESS;
}
