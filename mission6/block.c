/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: block.c

描述: 在该文件中实现了块设备层，块设备层是一个逻辑层。
	  
	  在调用 EOS API 函数 ReadFile 或 WriteFile 读写文件时，最终会被文件系统
	  驱动程序转换为对磁盘扇区的读写，对磁盘扇区的读写是通过调用本文件中的
	  IopReadWriteSector 函数实现的。

	  同时实现了先来先服务 (FCFS) 磁盘调度算法。



*******************************************************************************/

#include "iop.h"
#include "psp.h"


//
// 块设备层用来进行磁盘调度的一组变量。
//
MUTEX DiskScheduleMutex;	// 多线程使用该互斥信号量互斥访问这组变量。
BOOL IsDeviceBusy;			// 磁盘设备忙的标志。
ULONG CurrentCylinder;		// 磁盘的磁头当前所在的磁道号。
LIST_ENTRY RequestListHead;	// 对磁盘设备读写请求的队列。

#ifdef _DEBUG

typedef struct _REQCURCYLINDER
{
	ULONG CurCylinder;
	ULONG ReqCylinder;
	ULONG CurThreadId;
	LONG Offset;
}REQCURCYLINDER,*PREQCURCYLINDER;

volatile INT ThreadSeq = 0;
INT ThreadCount = 0;

REQCURCYLINDER CurReqCy[20];

#endif


VOID
IopInitializeBlockDeviceLayer(
	VOID
	)
/*++

功能描述：
	初始化块设备层。

参数：
	无。

返回值：
	无。

--*/
{
	PsInitializeMutex(&DiskScheduleMutex, FALSE);
	IsDeviceBusy = FALSE;
	CurrentCylinder = 0;
	ListInitializeHead(&RequestListHead);
}


//
// 对磁盘调度信息进行统计的一组变量。
//
PRIVATE BOOL IsDiskScheduleWorking = FALSE;	// 磁盘调度正在工作的标志
PRIVATE ULONG TotalOffset = 0, GTotalOffset = 0;		// 磁头移动距离总数
PRIVATE ULONG TransferTimes = 0, GTransferTimes = 0;	// 磁头移动次数


PREQUEST
IopReceiveRequest(
	IN ULONG SectorNumber
	)
/*++

功能描述：
	将线程对块设备的读写操作转化为一个请求。

参数：
	SectorNumber -- 线程要读写的扇区号。

返回值：
	返回请求的指针。

--*/
{
	LONG Offset;
	CHAR Direction;
	PREQUEST pRequest;

	//
	// 创建一个请求（分配内存）。
	//
	pRequest = (PREQUEST)MmAllocateSystemPool(sizeof(REQUEST));

	//
	// 根据线程对块设备的读写操作来初始化请求。
	// 根据扇区号计算出磁道号，目前 EOS 管理的磁盘设备只有一个软盘驱动器，
	// 可以直接使用每磁道扇区数（18）和磁头数（2）来计算扇区对应的磁道号。
	//
	pRequest->Cylinder = SectorNumber / 18 / 2;
	PsInitializeEvent(&pRequest->Event, TRUE, TRUE);

	PsWaitForMutex(&DiskScheduleMutex, INFINITE);	// 进入临界区

	if (IsDeviceBusy) {
		
		//
		// 如果磁盘设备忙，说明有其它线程正在访问磁盘，设置请求中的事件为无效
		// 状态。在退出临界区后，当前线程会阻塞在该事件上直到被磁盘调度算法选中。
		//
		PsResetEvent(&pRequest->Event);

		if (!IsDiskScheduleWorking) {

			//
			// 磁盘调度由非工作状态进入工作状态。
			//
			IsDiskScheduleWorking = TRUE;
		}

	} else {

		//
		// 当前线程独占访问磁盘设备，设置磁盘设备忙。
		//
		IsDeviceBusy = TRUE;
	}

	//
	// 将请求插入到请求队列的末尾。
	//
	ListInsertTail(&RequestListHead, &pRequest->ListEntry);

	PsReleaseMutex(&DiskScheduleMutex);		// 退出临界区

	//
	// 当前线程等待其对应请求中的事件，如果有其它线程正在访问磁盘，
	// 当前线程就会阻塞在该事件上直到被磁盘调度算法选中。
	//
	PsWaitForEvent(&pRequest->Event, INFINITE);

	if (IsDiskScheduleWorking) {

		//
		// 计算磁头移动距离并判断磁头移动方向
		//
		Offset = pRequest->Cylinder - CurrentCylinder;

		if (Offset > 0)
			Direction = '+';	// 磁道号增加。磁头向内移动。
		else if (Offset < 0)
			Direction = '-';	// 磁道号减小。磁头向外移动。
		else
			Direction = '=';	// 磁道号不变。磁头不移动。
	
#ifdef _DEBUG	
		CurReqCy[ThreadSeq].CurCylinder = CurrentCylinder;
		CurReqCy[ThreadSeq].ReqCylinder = pRequest->Cylinder;
		CurReqCy[ThreadSeq].Offset = Offset;
		CurReqCy[ThreadSeq].CurThreadId = ObGetObjectId(PspCurrentThread);
		
		ThreadSeq++;
		ThreadCount = ThreadSeq;

#endif
		//
		// 增加磁头移动距离总数。增加磁头移动次数。
		//
		TotalOffset += abs(Offset);
		TransferTimes++;
	}

	return pRequest;
}


PREQUEST
IopDiskSchedule(
	VOID
	);


VOID
IopProcessNextRequest(
	IN PREQUEST pCurrentRequest
	)
/*++

功能描述：
	结束处理当前的请求，并开始处理下一个请求。

参数：
	pCurrentRequest -- 当前的请求。

返回值：
	无。

--*/
{
	PREQUEST pNextRequest;

	ASSERT(pCurrentRequest != NULL);

	PsWaitForMutex(&DiskScheduleMutex, INFINITE);	// 进入临界区

	//
	// 当前请求对应的线程刚刚完成对磁盘的访问，
	// 所以其访问的磁道号就是磁头所在的磁道号。
	//
	CurrentCylinder = pCurrentRequest->Cylinder;

	//
	// 将已处理完毕的当前请求从请求队列中移除，并销毁请求（释放内存）。
	//
	ListRemoveEntry(&pCurrentRequest->ListEntry);
	MmFreeSystemPool(pCurrentRequest);

	if (ListIsEmpty(&RequestListHead)) {

		//
		// 请求队列变为空，磁盘设备退出忙状态。
		//
		IsDeviceBusy = FALSE;
		GTotalOffset = TotalOffset;		
		GTransferTimes = TransferTimes;	

		if (IsDiskScheduleWorking) {

			//
			// 重置工作状态
			//
			IsDiskScheduleWorking = FALSE;
			TotalOffset = 0;
			TransferTimes = 0;
		}
	
	} else {

		//
		// 请求队列不空，由磁盘调度算法选择要处理的下一个请求。
		//
		pNextRequest = IopDiskSchedule();

		//
		// 设置选中的请求中的事件为有效，阻塞在该事件上的线程即可访问磁盘。
		//
		PsSetEvent(&pNextRequest->Event);
	}

	PsReleaseMutex(&DiskScheduleMutex);		// 退出临界区
}


//
// 块设备使用的唯一的一个缓冲区（静态分配）。
//
PRIVATE BYTE BlockDeviceBuffer[512];


STATUS
IopReadWriteSector(
	IN PDEVICE_OBJECT Device,
	IN ULONG SectorNumber,
	IN ULONG ByteOffset,
	IN OUT PVOID Buffer,
	IN ULONG BytesToRw,
	IN BOOL Read
	)
/*++

功能描述：
	读写指定块设备的扇区。注意，每次只能处理一个扇区内的数据，不能跨扇区。
	
	在读取一个扇区时，要先将整个扇区读入缓存，然后再从缓存中读取需要的数据。在写
	一个扇区时，也需要先将整个扇区读入缓存，然后再修改缓存中的数据，最后再将缓存
	写入设备扇区。
	
	注意，在写扇区时，因为硬件设备的写扇区命令每次必须对整个扇区进行写操作，而该
	函数要求可以对扇区内的部分内容进行写操作，为了保证同一扇区内的其它字节不被错
	误覆盖，所以要先读取整个扇区到缓存，然后只修改缓存中的部分内容，最后将整个缓
	存再写入扇区。

参数：
	Device -- 要读写的块设备对应的设备对象的指针。
	SectorNumber -- 读写位置所在的扇区号。
	ByteOffset -- 读写位置在扇区内的字节偏移量，不可超过扇区大小(512)。
	Buffer -- 指向读写内容所在的缓冲区。
	BytesToRw -- 期望读写的字节数，ByteOffset + BytesToRw 不可以超过扇区大小，也就
				是说不可以跨扇区读写。
	Read -- 是否读操作。为 TRUE 则进行读操作，否则进行写操作。

返回值：
	如果成功则返回 STATUS_SUCCESS，否则表示失败。

--*/
{
	STATUS Status;
	PREQUEST pCurrentRequest;

	ASSERT(NULL != Device && Device->IsBlockDevice);
	ASSERT(ByteOffset < 512);
	ASSERT(0 < BytesToRw && BytesToRw <= 512);
	ASSERT(ByteOffset + BytesToRw <= 512);

	//
	// 将当前线程对磁盘的访问转变为一个请求。如有其它线程正在访问磁盘，
	// 会将请求放入请求队列中，等到被磁盘调度算法选中后再处理。
	//
	pCurrentRequest = IopReceiveRequest(SectorNumber);

	PsWaitForMutex(&Device->Mutex, INFINITE);	// 进入临界区

	//
	// 调用块设备驱动程序的 Read 功能函数，将整个扇区读入缓存。
	//
	Status = Device->DriverObject->Read(Device, NULL, BlockDeviceBuffer, SectorNumber, NULL);
	if (!EOS_SUCCESS(Status))
		goto RETURN;
	
	if (Read) {

		//
		// 读缓存
		//
		memcpy(Buffer, BlockDeviceBuffer + ByteOffset, BytesToRw);

	} else {

		//
		// 写缓存
		//
		memcpy(BlockDeviceBuffer + ByteOffset, Buffer, BytesToRw);

		//
		// 调用块设备驱动程序的 Write 功能函数，将缓存写回扇区。
		//
		Status = Device->DriverObject->Write(Device, NULL, BlockDeviceBuffer, SectorNumber, NULL);
		if (!EOS_SUCCESS(Status))
			goto RETURN;
	}
	
RETURN:

	PsReleaseMutex(&Device->Mutex);		// 退出临界区
	
	//
	// 由磁盘调度算法从请求队列中选择要处理的下一个请求，并开始处理。
	//
	IopProcessNextRequest(pCurrentRequest);

	return Status;
}


//
// N-Step-SCAN 磁盘调度算法使用的子队列长度 N
//
#define SUB_QUEUE_LENGTH 6

//
// 记录 N-Step-SCAN 磁盘调度算法第一个子队列剩余的长度。
// 子队列初始长度为 N，每执行一次磁盘调度算法会从子队列中移除一个请求，子队列
// 长度就要减少 1，待长度变为 0 时，再将长度重新变为 N，开始处理下一个子队列。
//
ULONG SubQueueRemainLength = SUB_QUEUE_LENGTH;

//
// 扫描算法中磁头移动的方向。操作系统启动时初始化为磁头向内移动。
// TRUE，磁头向内移动，磁道号增加。
// FALSE，磁头向外移动，磁道号减少。
//
BOOL ScanInside = TRUE;


PREQUEST
IopDiskSchedule(
	VOID
	)
/*++

功能描述：
	磁盘调度。可以在本函数中实现多种磁盘调度算法
	（包括 FCFS、SSTF、SCAN、CSCAN、N-Step-SCAN 等）。

参数：
	无。

返回值：
	返回从请求队列中选择的下一个要被处理的请求的指针。

说明：
	多数磁盘调度算法都是根据当前磁头所在的磁道和各个线程要访问的磁道来
	进行调度的。其中当前磁头所在的磁道保存在全局变量 CurrentCylinder 中，
	而请求队列中各个请求的 Cylinder 域保存了对应线程要访问的磁道。

	注意，该函数只是从请求队列中选择下一个要被处理的请求，而不需要将选中
	的请求从请求队列中移除，也不需要将请求对应的线程唤醒。

--*/
{
	PLIST_ENTRY pListEntry;
	PREQUEST pNextRequest;
	
	//
	// FCFS (First-Come,First-Served) 磁盘调度算法是一种最简单的磁盘调度算法，
	// 总是选择请求队列中的第一个请求，从而按照线程访问磁盘的先后顺序进行调度。
	//
	pListEntry = RequestListHead.Next;	// 请求队列中的第一个请求是链表头指向的下一个请求。
	
	//
	// 根据链表项获得请求指针
	//
	pNextRequest = CONTAINING_RECORD(pListEntry, REQUEST, ListEntry);
	
	return pNextRequest;
}
