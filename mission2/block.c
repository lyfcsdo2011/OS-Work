/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: block.c

����: �ڸ��ļ���ʵ���˿��豸�㣬���豸����һ���߼��㡣
	  
	  �ڵ��� EOS API ���� ReadFile �� WriteFile ��д�ļ�ʱ�����ջᱻ�ļ�ϵͳ
	  ��������ת��Ϊ�Դ��������Ķ�д���Դ��������Ķ�д��ͨ�����ñ��ļ��е�
	  IopReadWriteSector ����ʵ�ֵġ�

	  ͬʱʵ���������ȷ��� (FCFS) ���̵����㷨��



*******************************************************************************/

#include "iop.h"
#include "psp.h"


//
// ���豸���������д��̵��ȵ�һ�������
//
MUTEX DiskScheduleMutex;	// ���߳�ʹ�øû����ź�������������������
BOOL IsDeviceBusy;			// �����豸æ�ı�־��
ULONG CurrentCylinder;		// ���̵Ĵ�ͷ��ǰ���ڵĴŵ��š�
LIST_ENTRY RequestListHead;	// �Դ����豸��д����Ķ��С�

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

����������
	��ʼ�����豸�㡣

������
	�ޡ�

����ֵ��
	�ޡ�

--*/
{
	PsInitializeMutex(&DiskScheduleMutex, FALSE);
	IsDeviceBusy = FALSE;
	CurrentCylinder = 0;
	ListInitializeHead(&RequestListHead);
}


//
// �Դ��̵�����Ϣ����ͳ�Ƶ�һ�������
//
PRIVATE BOOL IsDiskScheduleWorking = FALSE;	// ���̵������ڹ����ı�־
PRIVATE ULONG TotalOffset = 0, GTotalOffset = 0;		// ��ͷ�ƶ���������
PRIVATE ULONG TransferTimes = 0, GTransferTimes = 0;	// ��ͷ�ƶ�����


PREQUEST
IopReceiveRequest(
	IN ULONG SectorNumber
	)
/*++

����������
	���̶߳Կ��豸�Ķ�д����ת��Ϊһ������

������
	SectorNumber -- �߳�Ҫ��д�������š�

����ֵ��
	���������ָ�롣

--*/
{
	LONG Offset;
	CHAR Direction;
	PREQUEST pRequest;

	//
	// ����һ�����󣨷����ڴ棩��
	//
	pRequest = (PREQUEST)MmAllocateSystemPool(sizeof(REQUEST));

	//
	// �����̶߳Կ��豸�Ķ�д��������ʼ������
	// ���������ż�����ŵ��ţ�Ŀǰ EOS ����Ĵ����豸ֻ��һ��������������
	// ����ֱ��ʹ��ÿ�ŵ���������18���ʹ�ͷ����2��������������Ӧ�Ĵŵ��š�
	//
	pRequest->Cylinder = SectorNumber / 18 / 2;
	PsInitializeEvent(&pRequest->Event, TRUE, TRUE);

	PsWaitForMutex(&DiskScheduleMutex, INFINITE);	// �����ٽ���

	if (IsDeviceBusy) {
		
		//
		// ��������豸æ��˵���������߳����ڷ��ʴ��̣����������е��¼�Ϊ��Ч
		// ״̬�����˳��ٽ����󣬵�ǰ�̻߳������ڸ��¼���ֱ�������̵����㷨ѡ�С�
		//
		PsResetEvent(&pRequest->Event);

		if (!IsDiskScheduleWorking) {

			//
			// ���̵����ɷǹ���״̬���빤��״̬��
			//
			IsDiskScheduleWorking = TRUE;
		}

	} else {

		//
		// ��ǰ�̶߳�ռ���ʴ����豸�����ô����豸æ��
		//
		IsDeviceBusy = TRUE;
	}

	//
	// ��������뵽������е�ĩβ��
	//
	ListInsertTail(&RequestListHead, &pRequest->ListEntry);

	PsReleaseMutex(&DiskScheduleMutex);		// �˳��ٽ���

	//
	// ��ǰ�̵߳ȴ����Ӧ�����е��¼�������������߳����ڷ��ʴ��̣�
	// ��ǰ�߳̾ͻ������ڸ��¼���ֱ�������̵����㷨ѡ�С�
	//
	PsWaitForEvent(&pRequest->Event, INFINITE);

	if (IsDiskScheduleWorking) {

		//
		// �����ͷ�ƶ����벢�жϴ�ͷ�ƶ�����
		//
		Offset = pRequest->Cylinder - CurrentCylinder;

		if (Offset > 0)
			Direction = '+';	// �ŵ������ӡ���ͷ�����ƶ���
		else if (Offset < 0)
			Direction = '-';	// �ŵ��ż�С����ͷ�����ƶ���
		else
			Direction = '=';	// �ŵ��Ų��䡣��ͷ���ƶ���
	
#ifdef _DEBUG	
		CurReqCy[ThreadSeq].CurCylinder = CurrentCylinder;
		CurReqCy[ThreadSeq].ReqCylinder = pRequest->Cylinder;
		CurReqCy[ThreadSeq].Offset = Offset;
		CurReqCy[ThreadSeq].CurThreadId = ObGetObjectId(PspCurrentThread);
		
		ThreadSeq++;
		ThreadCount = ThreadSeq;

#endif
		//
		// ���Ӵ�ͷ�ƶ��������������Ӵ�ͷ�ƶ�������
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

����������
	��������ǰ�����󣬲���ʼ������һ������

������
	pCurrentRequest -- ��ǰ������

����ֵ��
	�ޡ�

--*/
{
	PREQUEST pNextRequest;

	ASSERT(pCurrentRequest != NULL);

	PsWaitForMutex(&DiskScheduleMutex, INFINITE);	// �����ٽ���

	//
	// ��ǰ�����Ӧ���̸߳ո���ɶԴ��̵ķ��ʣ�
	// ��������ʵĴŵ��ž��Ǵ�ͷ���ڵĴŵ��š�
	//
	CurrentCylinder = pCurrentRequest->Cylinder;

	//
	// ���Ѵ�����ϵĵ�ǰ���������������Ƴ��������������ͷ��ڴ棩��
	//
	ListRemoveEntry(&pCurrentRequest->ListEntry);
	MmFreeSystemPool(pCurrentRequest);

	if (ListIsEmpty(&RequestListHead)) {

		//
		// ������б�Ϊ�գ������豸�˳�æ״̬��
		//
		IsDeviceBusy = FALSE;
		GTotalOffset = TotalOffset;		
		GTransferTimes = TransferTimes;	

		if (IsDiskScheduleWorking) {

			//
			// ���ù���״̬
			//
			IsDiskScheduleWorking = FALSE;
			TotalOffset = 0;
			TransferTimes = 0;
		}
	
	} else {

		//
		// ������в��գ��ɴ��̵����㷨ѡ��Ҫ�������һ������
		//
		pNextRequest = IopDiskSchedule();

		//
		// ����ѡ�е������е��¼�Ϊ��Ч�������ڸ��¼��ϵ��̼߳��ɷ��ʴ��̡�
		//
		PsSetEvent(&pNextRequest->Event);
	}

	PsReleaseMutex(&DiskScheduleMutex);		// �˳��ٽ���
}


//
// ���豸ʹ�õ�Ψһ��һ������������̬���䣩��
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

����������
	��дָ�����豸��������ע�⣬ÿ��ֻ�ܴ���һ�������ڵ����ݣ����ܿ�������
	
	�ڶ�ȡһ������ʱ��Ҫ�Ƚ������������뻺�棬Ȼ���ٴӻ����ж�ȡ��Ҫ�����ݡ���д
	һ������ʱ��Ҳ��Ҫ�Ƚ������������뻺�棬Ȼ�����޸Ļ����е����ݣ�����ٽ�����
	д���豸������
	
	ע�⣬��д����ʱ����ΪӲ���豸��д��������ÿ�α����������������д����������
	����Ҫ����Զ������ڵĲ������ݽ���д������Ϊ�˱�֤ͬһ�����ڵ������ֽڲ�����
	�󸲸ǣ�����Ҫ�ȶ�ȡ�������������棬Ȼ��ֻ�޸Ļ����еĲ������ݣ����������
	����д��������

������
	Device -- Ҫ��д�Ŀ��豸��Ӧ���豸�����ָ�롣
	SectorNumber -- ��дλ�����ڵ������š�
	ByteOffset -- ��дλ���������ڵ��ֽ�ƫ���������ɳ���������С(512)��
	Buffer -- ָ���д�������ڵĻ�������
	BytesToRw -- ������д���ֽ�����ByteOffset + BytesToRw �����Գ���������С��Ҳ��
				��˵�����Կ�������д��
	Read -- �Ƿ��������Ϊ TRUE ����ж��������������д������

����ֵ��
	����ɹ��򷵻� STATUS_SUCCESS�������ʾʧ�ܡ�

--*/
{
	STATUS Status;
	PREQUEST pCurrentRequest;

	ASSERT(NULL != Device && Device->IsBlockDevice);
	ASSERT(ByteOffset < 512);
	ASSERT(0 < BytesToRw && BytesToRw <= 512);
	ASSERT(ByteOffset + BytesToRw <= 512);

	//
	// ����ǰ�̶߳Դ��̵ķ���ת��Ϊһ���������������߳����ڷ��ʴ��̣�
	// �Ὣ���������������У��ȵ������̵����㷨ѡ�к��ٴ���
	//
	pCurrentRequest = IopReceiveRequest(SectorNumber);

	PsWaitForMutex(&Device->Mutex, INFINITE);	// �����ٽ���

	//
	// ���ÿ��豸��������� Read ���ܺ������������������뻺�档
	//
	Status = Device->DriverObject->Read(Device, NULL, BlockDeviceBuffer, SectorNumber, NULL);
	if (!EOS_SUCCESS(Status))
		goto RETURN;
	
	if (Read) {

		//
		// ������
		//
		memcpy(Buffer, BlockDeviceBuffer + ByteOffset, BytesToRw);

	} else {

		//
		// д����
		//
		memcpy(BlockDeviceBuffer + ByteOffset, Buffer, BytesToRw);

		//
		// ���ÿ��豸��������� Write ���ܺ�����������д��������
		//
		Status = Device->DriverObject->Write(Device, NULL, BlockDeviceBuffer, SectorNumber, NULL);
		if (!EOS_SUCCESS(Status))
			goto RETURN;
	}
	
RETURN:

	PsReleaseMutex(&Device->Mutex);		// �˳��ٽ���
	
	//
	// �ɴ��̵����㷨�����������ѡ��Ҫ�������һ�����󣬲���ʼ����
	//
	IopProcessNextRequest(pCurrentRequest);

	return Status;
}


//
// N-Step-SCAN ���̵����㷨ʹ�õ��Ӷ��г��� N
//
#define SUB_QUEUE_LENGTH 6

//
// ��¼ N-Step-SCAN ���̵����㷨��һ���Ӷ���ʣ��ĳ��ȡ�
// �Ӷ��г�ʼ����Ϊ N��ÿִ��һ�δ��̵����㷨����Ӷ������Ƴ�һ�������Ӷ���
// ���Ⱦ�Ҫ���� 1�������ȱ�Ϊ 0 ʱ���ٽ��������±�Ϊ N����ʼ������һ���Ӷ��С�
//
ULONG SubQueueRemainLength = SUB_QUEUE_LENGTH;

//
// ɨ���㷨�д�ͷ�ƶ��ķ��򡣲���ϵͳ����ʱ��ʼ��Ϊ��ͷ�����ƶ���
// TRUE����ͷ�����ƶ����ŵ������ӡ�
// FALSE����ͷ�����ƶ����ŵ��ż��١�
//
BOOL ScanInside = TRUE;


PREQUEST
IopDiskSchedule(
	VOID
	)
/*++

����������
	���̵��ȡ������ڱ�������ʵ�ֶ��ִ��̵����㷨
	������ FCFS��SSTF��SCAN��CSCAN��N-Step-SCAN �ȣ���

������
	�ޡ�

����ֵ��
	���ش����������ѡ�����һ��Ҫ������������ָ�롣

˵����
	�������̵����㷨���Ǹ��ݵ�ǰ��ͷ���ڵĴŵ��͸����߳�Ҫ���ʵĴŵ���
	���е��ȵġ����е�ǰ��ͷ���ڵĴŵ�������ȫ�ֱ��� CurrentCylinder �У�
	����������и�������� Cylinder �򱣴��˶�Ӧ�߳�Ҫ���ʵĴŵ���

	ע�⣬�ú���ֻ�Ǵ����������ѡ����һ��Ҫ����������󣬶�����Ҫ��ѡ��
	�����������������Ƴ���Ҳ����Ҫ�������Ӧ���̻߳��ѡ�

--*/
{
	PLIST_ENTRY pListEntry;
	PREQUEST pNextRequest;
	
	//
	// FCFS (First-Come,First-Served) ���̵����㷨��һ����򵥵Ĵ��̵����㷨��
	// ����ѡ����������еĵ�һ�����󣬴Ӷ������̷߳��ʴ��̵��Ⱥ�˳����е��ȡ�
	//
	pListEntry = RequestListHead.Next;	// ��������еĵ�һ������������ͷָ�����һ������
	
	//
	// ����������������ָ��
	//
	pNextRequest = CONTAINING_RECORD(pListEntry, REQUEST, ListEntry);
	
	return pNextRequest;
}
