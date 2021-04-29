/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: syspool.c

����: ϵͳ�ڴ�صĹ���������ʼ��������ͻ��ա�



*******************************************************************************/

#include "mi.h"

//
// ϵͳ�ڴ�ؽṹ�塣
//
PRIVATE MEM_POOL MiSystemPool;

VOID
MiInitializeSystemPool(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	)
/*++

����������
	��ʼ��ϵͳ�ڴ�ء�ϵͳ�ڴ��λ��ҳ������ݿ�֮��ISRջ֮ǰ��

������
	LoaderBlock -- ���ز����ṹ��ָ�롣

����ֵ��
	�ޡ�

--*/
{
	PVOID BaseOfPool;
	SIZE_T SizeOfPool;

	//
	// ϵͳ�ڴ��λ��ҳ������ݿ�֮��ISRջ֮ǰ��
	//
	BaseOfPool = (PVOID)MiGetPfnDatabaseEntry(MiTotalPageFrameCount);
	SizeOfPool = (ULONG_PTR)MM_SYSTEM_RANGE_START + LoaderBlock->MappedMemorySize -
				MM_ISR_STACK_SIZE - (ULONG_PTR)BaseOfPool;

	PoolInitialize(&MiSystemPool);
	PoolCommitMemory(&MiSystemPool, BaseOfPool, SizeOfPool);
}

PVOID
MmAllocateSystemPool(
	SIZE_T Size
	)
/*++

����������
	��ϵͳ�ڴ���з���һ���ڴ档

������
	Size -- ����������ڴ��Ĵ�С��

����ֵ��
	����ɹ��򷵻��ڴ��ĵ�ַ�����򷵻�NULL��

--*/
{
	PVOID Result;
	BOOL IntState;
	
	IntState = KeEnableInterrupts(FALSE);

	Result = PoolAllocateMemory(&MiSystemPool, &Size);

	KeEnableInterrupts(IntState);

	return Result;
}

STATUS
MmFreeSystemPool(
	PVOID Memory
	)
/*++

����������
	�ͷŴ�ϵͳ�ڴ���з�����ڴ�顣

������
	Memory -- �����ͷŵ��ڴ��ĵ�ַ��

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	BOOL Status;
	BOOL IntState;

	ASSERT(NULL != Memory);
	
	IntState = KeEnableInterrupts(FALSE);

	Status = PoolFreeMemory(&MiSystemPool, Memory);

	ASSERT(EOS_SUCCESS(Status));

	KeEnableInterrupts(IntState);

	return Status;
}
