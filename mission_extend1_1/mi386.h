/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: mi386.h

����: i386 �ڴ���ء�



*******************************************************************************/

#ifndef _MI386_
#define _MI386_

#include "rtl.h"

/*++

    ���� 4G �����ַ�ռ��� I386 ƽ̨�ϵĲ���:

                 +------------------------------------+
        00000000 | ���ɷ��ʵ�64KB�����������ڲ�׽�� |
				 | ��ָ��ķǷ����ʡ�				  |
                 +------------------------------------+
        00010000 |                                    |
                 |                                    |
                 | �û����̵Ŀ��õ������ַ�ռ䡣     |
				 | �û����̵Ŀ�ִ��ӳ�񱻼��ص���ַ	  |
				 | 0x400000�����û����̵ĶѺ�ջλ��	  |
				 | ��������ڣ��û����̵���			  |
				 | VirtualAlloc���������ڴ����ʱ��Ҳ |
				 | ��������䡣						  |
                 |                                    |
                 |                                    |
                 +------------------------------------+
        7FFF0000 | ���ɷ��ʵ�64KB��������		      |
                 +------------------------------------+
        80000000 |                                    |
                 | �����ڴ��Լ1/8��ӳ�䵽�˴���ӳ����|
				 | ��������256MB��������4M��ַ���롣  |
				 | ��ӳ��������ڴ�ʹ�����£�		  |	
				 |   0-64KB������δ�ã�               |
				 |   64KB-640KB�����ڼ���kernel.dll�� |
				 |   640KB-1MB��BIOS���򣬲����á�	  |
				 |	 1MB-?������ҳ������ݿ⣬��С��  |
				 |			������ҳ������������	  |
                 |   ? - ?: ϵͳ�ڴ�ء�			  |
				 |   ? - ?: ���8K����ISRר��ջ��	  |
                 |                                    |
                 +------------------------------------+
		90000000 |                                    |
                 |									  |
                 | ����δ�á�						  |
				 |                                    |
                 |                                    |
                 +------------------------------------+
		A0000000 |                                    |
                 | ��ϵͳ��̬�������������ַ�ռ䣬 |
				 | �������Ĵ�СΪ�����ڴ��С��1/4, |
				 | ���512MB��						  |
				 | ��������ڳ�ʼ��ʱ����ȫ����װPTE��|
				 | ��������̵�ҳĿ¼�һ�¡�		  |
                 |                                    |
                 +------------------------------------+
        C0000000 |                                    |
                 | ����ҳ��ӳ�䵽��4MB����		  |
                 |                                    |
                 +------------------------------------+
		C0400000 | ҳ����ЧPTE������ӳ�䵽��4K����  |
				 +------------------------------------+
		C0401000 |									  |
				 |									  |
				 | ϵͳPTE�������ڿ��ٽ���������ҳ  |
				 | ��ӳ�����ˣ��������ڴ���г�ʼ���� |
				 |									  |
				 |									  |
                 +------------------------------------+
        C0800000 |                                    |
                 |                                    |
                 |                                    |
				 |									  |
				 | ����δ�á�						  |
                 |									  |
                 |									  |
				 |                                    |
        FFFFFFFF |                                    |
                 +------------------------------------+

--*/

//
// ���������顣
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
	ULONG Valid : 1;			// ����λ
	ULONG Writable : 1;			// ��д��־
	ULONG User : 1;				// �û�ҳ��־
	ULONG WriteThrough : 1;		// ��͸д��־
	ULONG CacheDisable : 1;		// ����ʹ�ñ�־
	ULONG Accessed : 1;			// �ѷ��ʱ�־
	ULONG Dirty : 1;			// ��ҳ��־
	ULONG LargePage : 1;		// ��ҳ���־
	ULONG Global : 1;			// ȫ��ҳ��־
	ULONG Unused : 3;
	ULONG PageFrameNumber : 20;	// ҳ���
}MMPTE_HARDWARE, *PMMPTE_HARDWARE;

//
// �����ַ����PTI_SHIFTλ�ɵõ�ҳ��š�
//
#define PAGE_SHIFT 12

//
// �����ַ����PDI_SHIFTλ�ɵõ�ҳĿ¼��������
//
#define PDI_SHIFT 22

//
// ҳĿ¼����ռ10λ
//
#define PDI_BITS 10

//
// �����ַ����PTI_SHIFTλ�ɵõ�ҳ����������
//
#define PTI_SHIFT 12

//
// ҳ������ռ10λ
//
#define PTI_BITS 10

//
// ҳĿ¼���� 1024 �� PDE��ҳ������ 1024 �� PTE
//
#define PTE_PER_TABLE	0x400

//
// ҳĿ¼���ҳ����� 4 �ֽ�
//
#define PTE_SIZE		0x4

//
// �ڴ������ʹ�õ�PTE��
//
typedef struct _MMPTE
{
	union{
		ULONG Long;
		MMPTE_HARDWARE Hard;
	}u;
}MMPTE, *PMMPTE;

//
// ҳ���(Page Frame Number)���ݿ���ṹ�塣
//
typedef struct _MMPFN
{
	ULONG Unused : 9;
	ULONG PageState : 3;
	ULONG Next : 20;
}MMPFN, *PMMPFN;

//
// �ڴ沼�ֲ������塣
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
// �õ������ַ��Ӧ��ҳĿ¼�
//
#define MiGetPdeAddressByVa(va)  (PDE_BASE + (((ULONG)(va)) >> PDI_SHIFT))

//
// �õ������ַ��Ӧ��ҳ���
//
#define MiGetPteAddressByVa(va) (PTE_BASE + (((ULONG)(va)) >> PTI_SHIFT))

//
// �õ���ҳ���Ӧ��ҳĿ¼�
//
#define MiGetPdeAddress(vpn)  (PDE_BASE + ((vpn) >> PTI_BITS))

//
// �õ���ҳ���Ӧ��ҳ���
//
#define MiGetPteAddress(vpn) (PTE_BASE + (vpn))

//
// ������ҳ���Ӧ��PTE������������֮ǰ��ֵ��
//
#define MiIncPteCounter(vpn) (PTE_COUNTER_DATABASE[(vpn) >> PTI_BITS]++)

//
// ��С��ҳ���Ӧ��PTE������������֮ǰ��ֵ��
//
#define MiDecPteCounter(vpn) (PTE_COUNTER_DATABASE[(vpn) >> PTI_BITS]--)

//
// �õ�ҳ��Ŷ�Ӧ��ҳ������ݿ��
//
#define MiGetPfnDatabaseEntry(pfn) (PFN_DATABASE + pfn)

//
// ����ҳӳ�䵽����ҳ��
//
#define MiMapPage(vpn, pfn) \
	do { \
		MiGetPteAddress(vpn)->u.Hard.PageFrameNumber = pfn; \
		MiGetPteAddress(vpn)->u.Hard.Writable = 1; \
		MiGetPteAddress(vpn)->u.Hard.Valid = 1; \
		MiFlushSingleTlb(MI_VPN_TO_VA(vpn));\
	} while (0)

//
// ȡ����ҳ������ҳ��ӳ�䡣
//
#define MiUnmapPage(vpn) \
	do { \
		MiGetPteAddress(vpn)->u.Long = 0; \
		MiFlushSingleTlb(MI_VPN_TO_VA(vpn)); \
	} while (0)

//
// Ϊ�����ַ��װһ��ҳ��
//
#define MiMapPageTable(vpn, pfn) \
	do {\
		MiGetPdeAddress(vpn)->u.Hard.PageFrameNumber = pfn; \
		MiGetPdeAddress(vpn)->u.Hard.Writable = 1; \
		MiGetPdeAddress(vpn)->u.Hard.Valid = 1; \
		MiFlushSingleTlb(MiGetPteAddress(vpn)); \
	} while (0)

//
// ж�������ַ��ҳ��
//
#define MiUnmapPageTable(vpn) \
	do { \
		MiGetPdeAddress(vpn)->u.Long = 0; \
		MiFlushSingleTlb(MiGetPteAddress(vpn)); \
	} while (0);

//
// дҳĿ¼�Ĵ�����
//
VOID
MiSetPageDirectory(
	ULONG_PTR Pfn
	);

//
// ˢ��TLB(Translation Lookaside Buffer)��
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
