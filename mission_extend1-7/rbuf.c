/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: rbuf.c

����: ���λ�������ʵ�֡�
	  ע�⣺��ʵ��δ�ṩ�κ�ͬ�����ƣ��Ի��λ�������дʱҪע���̼߳��Լ��߳�����
	  �ϴ������֮��Ļ��⡣



*******************************************************************************/

#include "iop.h"

PRING_BUFFER 
IopCreateRingBuffer(
	ULONG BufferSize
	)
/*++

����������
	�������λ�������

������
	BufferSize - ���λ������Ĵ�С��

����ֵ��
	����ɹ��򷵻ػ��λ������Ľṹ��ָ�룬���򷵻�NULL��

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

����������
	ɾ�����λ�������

������
	RingBuffer - Ҫɾ���Ļ��λ������Ľṹ��ָ�롣

����ֵ��
	�ޡ�

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

����������
	�򻺳�����д���ݡ�

������
	RingBuffer - ���λ������Ľṹ��ָ�룻
	Data -- ָ�룬ָ��Ҫд���ڴ����ݣ�
	NumberOfBytesToWrite -- ����д���ݵ��ֽ�����

����ֵ��
	ʵ��д�뻺�������ݵ��ֽ�������ֵ������NumberOfBytesToWrite����дǰ����������
	�ռ��С�����ƣ����дǰ�����������򷵻�0��

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

����������
	���������е����ݡ�

������
	RingBuffer - ���λ������Ľṹ��ָ�룻
	Data -- ָ�룬ָ��Ҫ��Ŷ�ȡ������ݵ��ڴ棻
	NumberOfBytesToRead -- ���������ݵ��ֽ�����

����ֵ��
	ʵ�ʶ������ݵ��ֽ�������ֵ������NumberOfBytesToRead���ܶ�ǰ�������������ֽ�
	�������ƣ������ǰ���������򷵻�0��

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
