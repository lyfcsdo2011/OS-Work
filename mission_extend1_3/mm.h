/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: mm.h

����: �ڴ����ģ�鹫���ӿ�ͷ�ļ���



*******************************************************************************/

#ifndef _MM_
#define _MM_

#include "eosdef.h"

//
// ���̵�ַ�ռ�ṹ��ָ�����͵�������
//
typedef struct _MMPAS *PMMPAS;

//
// ����ҳ���С��
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
// Size����Բ����ҳ���С����������
//
#define ROUND_TO_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

//
// Size���ֽ�ռ�ö��ٸ�ҳ�档
//
#define BYTES_TO_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> PAGE_SHIFT) + \
							   (((ULONG)(Size) & (PAGE_SIZE - 1)) != 0))

//
// �����ַ���ҳ��߽��ƫ������
//
#define BYTE_OFFSET(va) ((LONG_PTR)(va) & (PAGE_SIZE - 1))


//
// �����ַ���뵽ҳ��߽硣
//
#define PAGE_ALIGN(va) ((PVOID)((ULONG_PTR)(va) & ~(PAGE_SIZE - 1)))

//
// �����ַ�Ƿ���뵽ҳ��߽硣
//
#define IS_PAGE_ALIGNED(va) (0 == ((ULONG_PTR)(va) & (PAGE_SIZE - 1)))


//
// ��ʼ���ڴ����ģ���һ����
//
VOID
MmInitializeSystem1(
	IN PVOID LoaderBlock
	);

//
// ��ʼ���ڴ����ģ��ڶ�����
//
VOID
MmInitializeSystem2(
	VOID
	);

//
// ��ϵͳ�ڴ���з����ڴ档
//
PVOID
MmAllocateSystemPool(
	IN SIZE_T nSize
	);

//
// �ͷ�ϵͳ�ڴ���е��ڴ档
//
STATUS
MmFreeSystemPool(
	IN PVOID pMem
	);

//
// �õ�ϵͳ���̵�ַ�ռ䡣
//
PMMPAS
MmGetSystemProcessAddressSpace(
	VOID
	);

//
// ����һ�����̵�ַ�ռ䡣
//
PMMPAS
MmCreateProcessAddressSpace(
	VOID
	);

//
// ���ٽ��̵�ַ�ռ䡣
//
VOID
MmDeleteProcessAddressSpace(
	IN PMMPAS Pas
	);

//
// ������̵�ַ�ռ䡣
//
PMMPAS
MmSwapProcessAddressSpace(
	IN PMMPAS NewPas
	);

//
// �ڵ�ǰ���̵�ַ�ռ��з��������ڴ档
//
STATUS
MmAllocateVirtualMemory(
	IN OUT PVOID *BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG AllocationType,
	IN BOOL SystemVirtual
	);

//
// �ͷŵ�ǰ���̵�ַ�ռ��е������ڴ档
//
STATUS
MmFreeVirtualMemory(
	IN OUT PVOID *BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG FreeType,
	IN BOOL SystemVirtual
	);

//
// ����ǰ���̵�ַ�ռ��������Ѿ�����������ڴ档
//
VOID
MmCleanVirtualMemory(
	VOID
	);

//
// �õ���ǰ��ַ�ռ��������ַ��Ӧ�������ַ��
//
STATUS
MmGetPhysicalAddress(
	IN PVOID VirtualAddress,
	OUT PVOID* PhysicalAddress
	);

#endif // _MM_
