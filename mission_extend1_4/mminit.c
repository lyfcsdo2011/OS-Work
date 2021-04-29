/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: mminit.c

描述: 初始化内存管理模块。



*******************************************************************************/

#include "mi.h"

VOID
MmInitializeSystem1(
	IN PVOID LoaderBlock
	)
/*++

功能描述：
	内存管理模块的第一步初始化。

参数：
	LoaderBlock - Loader传递的加载参数块结构体指针，内存管理器要使用。

返回值：
	无。

--*/
{
	PLOADER_PARAMETER_BLOCK lb = (PLOADER_PARAMETER_BLOCK)LoaderBlock;

	ASSERT(MM_SYSTEM_RANGE_START == (PVOID)lb->SystemVirtualBase);
	ASSERT(PTE_BASE == (PMMPTE)lb->PageTableVirtualBase);
	ASSERT(MM_KERNEL_IMAGE_BASE == (PVOID)lb->ImageVirtualBase);

	//
	// 初始化页框号数据库。
	//
	MiInitializePfnDatabase(lb);

	//
	// 初始化系统内存池。
	//
	MiInitializeSystemPool(lb);

	//
	// 设置ISR栈指针。ISR栈从已映射内存的最高处向下增长。
	//
	KeIsrStack = MM_SYSTEM_RANGE_START + lb->MappedMemorySize;

	//
	// 初始化系统PTE。
	//
	MiInitialzieSystemPte();

	//
	// 初始化系统进程地址空间。
	//
	MiInitializeSystemPas();
}

VOID
MmInitializeSystem2(
	VOID
	)
/*++

功能描述：
	内存管理器的第二步初始化，可以创建系统线程，可以调用阻塞函数。

参数：
	

返回值：
	

--*/
{

}
