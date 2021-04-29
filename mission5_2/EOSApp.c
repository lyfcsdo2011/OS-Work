/*
�ṩ��ʾ��������Ϊ�˲���һ��������߽���һ�����ԣ�����������
�ȫ�ı���ʵ������˲�Ӧ��Ӧ�ó������վ��ʹ�ø�ʾ�����롣��
�ڳ�����ʾ�������Ԥ����;�����ʹ������ɵ�żȻ��̷�����ʧ��
����Ӣ��ʱ���Ƽ����޹�˾���е��κ����Ρ�
*/

#include "EOSApp.h"

//
// ����ء�
//
#define BUFFER_SIZE		10
int Buffer[BUFFER_SIZE];

//
// ��Ʒ������
//
#define PRODUCT_COUNT	30

//
// ���������ߺ�������ͬ���Ķ�������
//
HANDLE MutexHandle;
HANDLE EmptySemaphoreHandle;
HANDLE FullSemaphoreHandle;

//
// �����ߺ������ߵ��̺߳���
//
ULONG Producer(PVOID Param);
ULONG Consumer(PVOID Param);

//
// main �������������壺
// argc - argv ����ĳ��ȣ���С����Ϊ 1��argc - 1 Ϊ�����в�����������
// argv - �ַ���ָ�����飬���鳤��Ϊ�����в������� + 1������ argv[0] �̶�ָ��ǰ
//        ������ִ�еĿ�ִ���ļ���·���ַ�����argv[1] ��������ָ��ָ�����������
//        ������
//        ����ͨ������������ "a:\hello.exe -a -b" �������̺�hello.exe �� main ��
//        ���Ĳ��� argc ��ֵΪ 3��argv[0] ָ���ַ��� "a:\hello.exe"��argv[1] ָ��
//        �����ַ��� "-a"��argv[2] ָ������ַ��� "-b"��
//
int main(int argc, char* argv[])
{
	//
	// �������� EOS Ӧ�ó���ǰҪ�ر�ע����������⣺
	//
	// 1�����Ҫ�ڵ���Ӧ�ó���ʱ�ܹ����Խ����ں˲���ʾ��Ӧ��Դ�룬
	//    ����ʹ�� EOS ������Ŀ����������ȫ�汾�� SDK �ļ��У�Ȼ
	//    ��ʹ�ô��ļ��и���Ӧ�ó�����Ŀ�е� SDK �ļ��У����� EOS
	//    ������Ŀ�ڴ����ϵ�λ�ò��ܸı䡣
	//
	
	HANDLE ProducerHandle;
	HANDLE ConsumerHandle;

	//
	// �������ڻ�����ʻ���ص� Mutex ����
	//
	MutexHandle = CreateMutex(FALSE, NULL);
	if (NULL == MutexHandle) {
		return 1;
	}

	//
	// ���� Empty �ź�������ʾ������пջ�������������ʼ��������������Ϊ BUFFER_SIZE��
	//
	EmptySemaphoreHandle = CreateSemaphore(BUFFER_SIZE, BUFFER_SIZE, NULL);
	if (NULL == EmptySemaphoreHandle) {
		return 2;
	}

	//
	// ���� Full �ź�������ʾ�����������������������ʼ����Ϊ 0��������Ϊ BUFFER_SIZE��
	//
	FullSemaphoreHandle = CreateSemaphore(0, BUFFER_SIZE, NULL);
	if (NULL == FullSemaphoreHandle) {
		return 3;
	}

	//
	// �����������̡߳�
	//
	ProducerHandle = CreateThread( 0,			// Ĭ�϶�ջ��С
								   Producer,	// �̺߳�����ڵ�ַ
								   NULL,		// �̺߳�������
								   0,			// ������־
								   NULL );		// �߳� ID

	if (NULL == ProducerHandle) {
		return 4;
	}

	//
	// �����������̡߳�
	//
	ConsumerHandle = CreateThread( 0,
								   Consumer,
								   NULL,
								   0,
								   NULL );

	if (NULL == ConsumerHandle) {
		return 5;
	}

	//
	// �ȴ��������̺߳��������߳̽�����
	//
	WaitForSingleObject(ProducerHandle, INFINITE);
	WaitForSingleObject(ConsumerHandle, INFINITE);
	
	//
	// �رվ��
	//
	CloseHandle(MutexHandle);
	CloseHandle(EmptySemaphoreHandle);
	CloseHandle(FullSemaphoreHandle);
	CloseHandle(ProducerHandle);
	CloseHandle(ConsumerHandle);

	return 0;
}

//
// �������̺߳�����
//
ULONG Producer(PVOID Param) 
{
	int i;
	int InIndex = 0;

	for (i = 0; i < PRODUCT_COUNT; i++) {

		//WaitForSingleObject(EmptySemaphoreHandle, INFINITE);
		
		while(WAIT_TIMEOUT == WaitForSingleObject(EmptySemaphoreHandle, 300)){
			printf("Producer wait for empty semaphore timeout\n");
		}
		WaitForSingleObject(MutexHandle, INFINITE);

		printf("Produce a %d\n", i);
		Buffer[InIndex] = i;
		InIndex = (InIndex + 1) % BUFFER_SIZE;

		ReleaseMutex(MutexHandle);
		ReleaseSemaphore(FullSemaphoreHandle, 1, NULL);

		//
		// ��Ϣһ�ᡣÿ 500 ��������һ������
		//
		Sleep(500);
	}
	
	return 0;
}

//
// �������̺߳�����
//
/*
ULONG Consumer(PVOID Param)
{
	int i;
	int OutIndex = 0;

	for (i = 0; i < PRODUCT_COUNT; i++) {

		//WaitForSingleObject(FullSemaphoreHandle, INFINITE);
		
		while(WAIT_TIMEOUT == WaitForSingleObject(FullSemaphoreHandle, 300)){
			printf("Consumer wait for full semaphore timeout\n");
		}
		WaitForSingleObject(MutexHandle, INFINITE);

		printf("\t\t\tConsume a %d\n", Buffer[OutIndex]);
		OutIndex = (OutIndex + 1) % BUFFER_SIZE;

		ReleaseMutex(MutexHandle);
		ReleaseSemaphore(EmptySemaphoreHandle, 1, NULL);

		//
		// ��Ϣһ�������ǰ 10 �����������ٶȱȽ���������ĽϿ졣
		//
		if (i < 10) {
			Sleep(2000);
		} else {
			Sleep(100);
		}
	}
	
	return 0;
}
*/
ULONG Consumer(PVOID Param)
{
	int i;
	int OutIndex = 0;

	for (i = 0; i < PRODUCT_COUNT; i += 2) {

		while(WAIT_TIMEOUT == WaitForSingleObject(FullSemaphoreHandle, 300)){
			printf("Consumer wait for full semaphore timeout\n");
		}
		while(WAIT_TIMEOUT == WaitForSingleObject(FullSemaphoreHandle, 300)){
			printf("Consumer wait for full semaphore timeout\n");
		}
		WaitForSingleObject(MutexHandle, INFINITE);

		printf("\t\t\tConsume a %d\n", Buffer[OutIndex]);
		OutIndex = (OutIndex + 1) % BUFFER_SIZE;
		printf("\t\t\tConsume a %d\n", Buffer[OutIndex]);
		OutIndex = (OutIndex + 1) % BUFFER_SIZE;

		ReleaseMutex(MutexHandle);
		ReleaseSemaphore(EmptySemaphoreHandle, 2, NULL);

		//
		// ��Ϣһ�������ǰ 14 �����������ٶȱȽ���������ĽϿ졣
		//
		if (i < 14) {
			Sleep(2000);
		} else {
			Sleep(100);
		}
	}
	
	return 0;
}