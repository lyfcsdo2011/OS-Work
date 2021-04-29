/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: mi.h

����: �ڴ�������ڲ�ͷ�ļ���



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
// �õ���ַ����ҳ���ҳ��š�
//
#define MI_VA_TO_VPN(va)  ((ULONG_PTR)(va) >> PAGE_SHIFT)

//
// ��ҳ��ŵõ�ҳ�����ʼ��ַ��
//
#define MI_VPN_TO_VA(vpn)  (PVOID)((vpn) << PAGE_SHIFT)

//
// ��ҳ��ŵõ�ҳ��Ľ�����ַ��
//
#define MI_VPN_TO_VA_ENDING(vpn)  (PVOID)(((vpn) << PAGE_SHIFT) | (PAGE_SIZE - 1))

//
// ����ҳ״̬���塣
//
typedef enum _PAGE_STATE {
	ZEROED_PAGE,	// 0
	FREE_PAGE,		// 1
	BUSY_PAGE,		// 2
} PAGE_STATE;

//
// �����ַ���������ṹ�塣
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
// ���̵�ַ�ռ�(Process Address VadList)�ṹ�塣
//
typedef struct _MMPAS {

	//
	// ���Ը����������ʹ�õĵ�ַ�ռ䡣
	//
	MMVAD_LIST VadList;

	//
	// ҳĿ¼��PTE���������ݿ��ҳ��š�
	//
	ULONG_PTR PfnOfPageDirectory;
	ULONG_PTR PfnOfPteCounter;
}MMPAS;

//
// �ڴ�ؽṹ�塣 
//
typedef struct _MEM_POOL {
	LIST_ENTRY FreeListHeads[32];
}MEM_POOL, *PMEM_POOL;

//
// ����ҳ��������
//
extern ULONG_PTR MiTotalPageFrameCount;
extern ULONG_PTR MiZeroedPageCount;
extern ULONG_PTR MiFreePageCount;

//
// ϵͳ���̵�ַ�ռ䡣
//
extern MMPAS MiSystemPas;

//
// ��ǰ���̵�ַ�ռ䡣
//
extern volatile PMMPAS MiCurrentPas;

//
// ��ʼ��ҳ������ݿ⡣
//
VOID
MiInitializePfnDatabase(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	);

//
// �õ����õ�����ҳ��������
//
ULONG_PTR
MiGetAnyPageCount(
	VOID
	);

//
// ��������ҳ��
//
STATUS
MiAllocateAnyPages(
	IN ULONG_PTR NumberOfPages,
	OUT PULONG_PTR PfnArray
	);

//
// ���侭�����ʼ��������ҳ��
//
STATUS
MiAllocateZeroedPages(
	IN ULONG_PTR NumberOfPages,
	OUT PULONG_PTR PfnArray
	);

//
// �ͷ�����ҳ��
//
STATUS
MiFreePages(
	IN ULONG_PTR NumberOfPages,
	IN PULONG_PTR PfnArray
	);

//
// ��ʼ���ڴ�ؽṹ�塣
//
VOID
PoolInitialize(
	IN PMEM_POOL Pool
	);

//
// �ύ�ڴ���ڴ���й�
//
STATUS
PoolCommitMemory(
	IN PMEM_POOL Pool,
	IN PVOID Address,
	IN SIZE_T Size
	);

//
// ���ڴ�ط����ڴ档
//
PVOID
PoolAllocateMemory(
	IN PMEM_POOL Pool,
	IN OUT PSIZE_T Size
	);

//
// �ͷ����ڴ�ط�����ڴ�顣
//
STATUS
PoolFreeMemory(
	IN PMEM_POOL Pool,
	IN PVOID Address
	);

//
// ��ʼ��ϵͳ�ڴ�ء�
//
VOID
MiInitializeSystemPool(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	);

//
// ��ʼ��ϵͳPTE��
//
VOID
MiInitialzieSystemPte(
	VOID
	);

//
// ������ҳ��ӳ�䵽ϵͳPTE����
//
PVOID
MiMapPageToSystemPte(
	IN ULONG_PTR Pfn
	);

//
// �ͷ�ϵͳPTE��
//
VOID
MiFreeSystemPte(
	IN PVOID Va 
	);

//
// ��ʼ��ϵͳ���̵�ַ�ռ䡣
//
VOID
MiInitializeSystemPas(
	VOID
	);

//
// ��ʼ��MMVAD_LIST�ṹ�塣
//
VOID
MiInitializeVadList(
	IN PMMVAD_LIST VadList, 
	IN PVOID StartingAddress, 
	IN PVOID EndAddress
	);

//
// ����һ�������ַ����
//
STATUS
MiReserveAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PVOID BaseAddress,
	IN SIZE_T RegionSize,
	OUT PMMVAD *Vad
	);

//
// �����ѱ����������ַ����
//
STATUS
MiFindReservedAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PVOID BaseAddress,
	IN SIZE_T RegionSize,
	OUT PMMVAD *Vad
	);

//
// �ͷ������ַ����
//
VOID
MiFreeAddressRegion(
	IN PMMVAD_LIST VadList,
	IN PMMVAD Vad
	);

//
// ����VAD�����е������ѱ�������
//
VOID
MiCleanAddressRegion(
	IN PMMVAD_LIST VadList
	);

#endif // _MI_
