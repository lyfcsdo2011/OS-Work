/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: rbuf.c

描述: 环形缓冲区的实现。
	  注意：本实现未提供任何同步机制，对环形缓冲区读写时要注意线程间以及线程与中
	  断处理程序之间的互斥。



*******************************************************************************/

#include "iop.h"

PRING_BUFFER 
IopCreateRingBuffer(
	ULONG BufferSize
	)
/*++

功能描述：
	创建环形缓冲区。

参数：
	BufferSize - 环形缓冲区的大小。

返回值：
	如果成功则返回环形缓冲区的结构体指针，否则返回NULL。

--*/
{
	PRING_BUFFER RingBuffer;

	if (BufferSize < 4) {
		BufferSize = 4;
	}

	RingBuffer = (PRING_BUFFER)MmAllocateSystemPool(sizeof(RING_BUFFER) - 4 + BufferSize);

	if (NULL != RingBuffer) {
		RingBuffer->Size = BufferSize;
		RingBuffer->Start = 0;
		RingBuffer->FillCount = 0;
	}

	return RingBuffer;
}

VOID
IopDeleteRingBuffer(
	PRING_BUFFER RingBuffer
	)
/*++

功能描述：
	删除环形缓冲区。

参数：
	RingBuffer - 要删除的环形缓冲区的结构体指针。

返回值：
	无。

--*/
{
	MmFreeSystemPool(RingBuffer);
}

ULONG
IopWriteRingBuffer(
	IN PRING_BUFFER RingBuffer,
	IN PVOID Data,
	IN ULONG NumberOfBytesToWrite
	)
/*++

功能描述：
	向缓冲区中写数据。

参数：
	RingBuffer - 环形缓冲区的结构体指针；
	Data -- 指针，指向要写的内存数据；
	NumberOfBytesToWrite -- 期望写数据的字节数。

返回值：
	实际写入缓冲区数据的字节数，其值不大于NumberOfBytesToWrite，受写前缓冲区空闲
	空间大小的限制，如果写前缓冲区已满则返回0。

--*/
{
	ULONG Count;

	for (Count = 0;
		 Count < NumberOfBytesToWrite && RingBuffer->FillCount < RingBuffer->Size;
		 Count++, RingBuffer->FillCount++)
	{
		RingBuffer->Buffer[(RingBuffer->Start + RingBuffer->FillCount) % RingBuffer->Size] = ((PCHAR)Data)[Count];
	}

	return Count;
}

ULONG
IopReadRingBuffer(
	IN PRING_BUFFER RingBuffer,
	OUT PVOID Data,
	IN ULONG NumberOfBytesToRead
	)
/*++

功能描述：
	读缓冲区中的数据。

参数：
	RingBuffer - 环形缓冲区的结构体指针；
	Data -- 指针，指向要存放读取结果数据的内存；
	NumberOfBytesToRead -- 期望读数据的字节数。

返回值：
	实际读出数据的字节数，其值不大于NumberOfBytesToRead，受读前缓冲区中数据字节
	数的限制，如果读前缓冲区空则返回0。

--*/
{
	ULONG Count;

	for (Count = 0;
		 Count < NumberOfBytesToRead && RingBuffer->FillCount > 0;
		 Count++, RingBuffer->FillCount--)
	{
		((PCHAR)Data)[Count] = RingBuffer->Buffer[RingBuffer->Start];
		RingBuffer->Start = (RingBuffer->Start + 1) % RingBuffer->Size;
	}

	return Count;
}
