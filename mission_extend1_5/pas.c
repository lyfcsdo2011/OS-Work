/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: pas.c

����: ���̵�ַ�ռ�Ĵ�����ɾ�����л���



*******************************************************************************/

#include "mi.h"

//
// ϵͳ���̵�ַ�ռ䡣
//
MMPAS MiSystemPas;

//
// ��ǰ���̵�ַ�ռ� 
//
volatile PMMPAS MiCurrentPas = &MiSystemPas;

VOID
MiInitializeSystemPas(
	VOID
	)
/*++

����������
	��ʼ��ϵͳ���̵�ַ�ռ䡣
	ϵͳ���̵�ַ�ռ��У���MI_SYSTEM_VM_BASE��ʼ������ɽ��������ڴ���䣬���С
	Ϊ�����ڴ��1/4����󲻳���512MB��������4MBȡ�����������λ�����н��̵�ַ��
	�䶼�����ϵͳ��ַ�ռ��У�Ϊ��ʹ��������ҳĿ¼������ӳ�乲��ϵͳ��ַ�ռ��
	ҳĿ¼�����һ�£��������ȫ����װ��ҳ���ڷ��䡢���������ڴ�ʱ������ҳ
	��Ķ�̬��װ��ж�ء�

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	ULONG_PTR n;
	ULONG_PTR pfn;
	ULONG_PTR vpn;

	//
	// ����ϵͳ��ַ�ռ������ҳ��������
	//
	n = (MiTotalPageFrameCount + 4095) / 4096;

	if (n > 128) {
		n = 128;
	}

	//
	// ��ʼ���ɷ��������ڴ�������ַ�ռ�����������
	//
	MiInitializeVadList( &MiSystemPas.VadList,
						 MI_SYSTEM_VM_BASE,
						 MI_SYSTEM_VM_BASE + (n << PDI_SHIFT) - 1);

	//
	// Ϊ�ɷ��������ڴ�����ȫ����װҳ��
	//
	vpn = MI_VA_TO_VPN(MI_SYSTEM_VM_BASE);

	for (; n > 0; n--) {

		MiAllocateZeroedPages(1, &pfn);
		MiMapPageTable(vpn, pfn);

		//
		// һ��ҳĿ¼���� (1 << PDI_BITS) ��ҳĿ¼��
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

����������
	�õ�ϵͳ���̵�ַ�ռ䡣

������
	�ޡ�

����ֵ��
	ϵͳ���̵�ַ�ռ�ָ�롣

--*/
{
	return &MiSystemPas;
}

PMMPAS
MmCreateProcessAddressSpace(
	VOID
	)
/*++

����������
	����һ�����̵�ַ�ռ䡣

������
	�ޡ�

����ֵ��
	����ɹ��򷵻��´����Ľ��̵�ַ�ռ������ṹ���ָ�룬���򷵻�NULL��

--*/
{
	PMMPAS Pas;
	PMMPTE PageDirectory;
	ULONG_PTR PfnArray[2];
	ULONG_PTR i;

	//
	// ��ϵͳ�ڴ���з���MMPAS�ṹ�塣
	//
	Pas = (PMMPAS)MmAllocateSystemPool(sizeof(MMPAS));

	if (NULL == Pas) {
		return NULL;
	}

	//
	// ������������ҳ�򣬷ֱ�����ҳĿ¼��PTE���������ݿ⡣
	//
	if (!EOS_SUCCESS(MiAllocateZeroedPages(2, PfnArray))) {

		MmFreeSystemPool(Pas);

		return NULL;
	}

	//
	// ��ʼ���ɷ��������ڴ�������ַ�ռ�����������
	//
	MiInitializeVadList( &Pas->VadList,
						 MM_LOWEST_USER_ADDRESS,
						 MM_HIGHEST_USER_ADDRESS );

	Pas->PfnOfPageDirectory = PfnArray[0];
	Pas->PfnOfPteCounter = PfnArray[1];

	//
	// ��ҳĿ¼ӳ�䵽ϵͳPTE�����Զ�֮���г�ʼ����
	//
	PageDirectory = (PMMPTE)MiMapPageToSystemPte(Pas->PfnOfPageDirectory);

	//
	// ʹ���̹���ϵͳ��ַ�ռ䣬��������ӳ��ϵͳ��ַ�ռ��ҳĿ¼�
	//
	for (i = ((ULONG_PTR)MM_SYSTEM_RANGE_START >> PDI_SHIFT); i <= (MAXULONG_PTR >> PDI_SHIFT); i++) {
		PageDirectory[i] = PDE_BASE[i];
	}
	
	//
	// ҳĿ¼����Ҳ��һ��ҳ������ӳ����̵�����ҳ��PTE_BASE����
	//
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Long = 0;
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Hard.PageFrameNumber = Pas->PfnOfPageDirectory;
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Hard.Writable = 1;
	PageDirectory[(ULONG_PTR)PTE_BASE >> PDI_SHIFT].u.Hard.Valid = 1;

	//
	// ҳĿ¼��ʼ����ϣ��ͷ�ϵͳPTE��
	//
	MiFreeSystemPte(PageDirectory);

	return Pas;
}

VOID
MmDeleteProcessAddressSpace(
	IN PMMPAS Pas
	)
/*++

����������
	ɾ�����̵�ַ�ռ䡣

������
	Pas -- ���̵�ַ�ռ������ṹ��ָ�롣

����ֵ��
	�ޡ�

ע�⣺
	��ǰ���̵�ַ�ռ��ϵͳ��ַ�ռ䲻���Ա�ɾ�������Ҫɾ��һ�����̵�ַ�ռ�������
	��ǰ��ַ�ռ䣬�����Ƚ�֮������ɾ��֮��

--*/
{
	BOOL IntState;

	IntState = KeEnableInterrupts(FALSE);

	//
	// ������ɾ����ǰ���̵�ַ�ռ��ϵͳ��ַ�ռ䡣
	//
	ASSERT(Pas != MiCurrentPas && Pas != &MiSystemPas);

	//
	// ���̵�ַ�ռ�Ӧ�ñ��������
	//
	ASSERT(ListIsEmpty(&Pas->VadList.VadListHead));

	//
	// �ͷ�ҳĿ¼��MMPAS�ṹ�塣
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

����������
	������̵�ַ�ռ䣬������̵�ַ�ռ��Ѿ��������򲻽����κβ���������һ��ʱ�̣�
	ֻ��һ�����̵�ַ�ռ䴦�ڻ���״̬��ֻ�н��̵�ַ�ռ䴦�ڻ���״̬ʱ�����ܶԽ���
	���û���ַ�ռ���ж�д���ʡ�

������
	NewPas -- ����������Ľ��̵�ַ�ռ�������ṹ���ָ�롣

����ֵ��
	�ޡ�

ע�⣺
	�������ֻ�����жϻ����±����ã���Ϊ�ڽ��̻����£���ǰ���̵ĵ�ַ�ռ��ǲ�����
	�������ġ�

--*/
{
	BOOL IntState;
	PMMPAS OldPas;

	IntState = KeEnableInterrupts(FALSE);

	OldPas = MiCurrentPas;

	if (NewPas != MiCurrentPas) {

		MiSetPageDirectory(NewPas->PfnOfPageDirectory);

		//
		// ��PTE������ҳӳ����PTE_COUNTER_DATABASE����
		// ע�⣺ϵͳ���̵�ַ�ռ䲻ʹ��PTE����������Ϊϵͳ���̵�ַ�ռ��пɷ�����
		// ���ڴ��ҳ���ǹ̶��ģ������ж�̬�İ�װ��ж�ء�
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
