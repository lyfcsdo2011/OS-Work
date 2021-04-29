/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: syspool.c

描述: 系统内存池的管理，包括初始化、分配和回收。



*******************************************************************************/

#include "mi.h"

//
// 系统内存池结构体。
//
PRIVATE MEM_POOL MiSystemPool;

VOID
MiInitializeSystemPool(
	PLOADER_PARAMETER_BLOCK LoaderBlock
	)
/*++

功能描述：
	初始化系统内存池。系统内存池位于页框号数据库之后、ISR栈之前。

参数：
	LoaderBlock -- 加载参数结构体指针。

返回值：
	无。

--*/
{
	PVOID BaseOfPool;
	SIZE_T SizeOfPool;

	//
	// 系统内存池位于页框号数据库之后、ISR栈之前。
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

功能描述：
	从系统内存池中分配一块内存。

参数：
	Size -- 期望分配的内存块的大小。

返回值：
	如果成功则返回内存块的地址，否则返回NULL。

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

功能描述：
	释放从系统内存池中分配的内存块。

参数：
	Memory -- 期望释放的内存块的地址。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
