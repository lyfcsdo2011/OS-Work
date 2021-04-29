/*
提供该示例代码是为了阐释一个概念，或者进行一个测试，并不代表着
最安全的编码实践，因此不应在应用程序或网站中使用该示例代码。对
于超出本示例代码的预期用途以外的使用所造成的偶然或继发性损失，
北京英真时代科技有限公司不承担任何责任。
*/

#include "EOSApp.h"

//
// 缓冲池。
//
#define BUFFER_SIZE		10
int Buffer[BUFFER_SIZE];

//
// 产品数量。
//
#define PRODUCT_COUNT	30

//
// 用于生产者和消费者同步的对象句柄。
//
HANDLE MutexHandle;
HANDLE EmptySemaphoreHandle;
HANDLE FullSemaphoreHandle;

//
// 生产者和消费者的线程函数
//
ULONG Producer(PVOID Param);
ULONG Consumer(PVOID Param);

//
// main 函数参数的意义：
// argc - argv 数组的长度，大小至少为 1，argc - 1 为命令行参数的数量。
// argv - 字符串指针数组，数组长度为命令行参数个数 + 1。其中 argv[0] 固定指向当前
//        进程所执行的可执行文件的路径字符串，argv[1] 及其后面的指针指向各个命令行
//        参数。
//        例如通过命令行内容 "a:\hello.exe -a -b" 启动进程后，hello.exe 的 main 函
//        数的参数 argc 的值为 3，argv[0] 指向字符串 "a:\hello.exe"，argv[1] 指向
//        参数字符串 "-a"，argv[2] 指向参数字符串 "-b"。
//
int main(int argc, char* argv[])
{
	//
	// 启动调试 EOS 应用程序前要特别注意下面的问题：
	//
	// 1、如果要在调试应用程序时能够调试进入内核并显示对应的源码，
	//    必须使用 EOS 核心项目编译生成完全版本的 SDK 文件夹，然
	//    后使用此文件夹覆盖应用程序项目中的 SDK 文件夹，并且 EOS
	//    核心项目在磁盘上的位置不能改变。
	//
	
	HANDLE ProducerHandle;
	HANDLE ConsumerHandle;

	//
	// 创建用于互斥访问缓冲池的 Mutex 对象。
	//
	MutexHandle = CreateMutex(FALSE, NULL);
	if (NULL == MutexHandle) {
		return 1;
	}

	//
	// 创建 Empty 信号量，表示缓冲池中空缓冲区数量。初始计数和最大计数都为 BUFFER_SIZE。
	//
	EmptySemaphoreHandle = CreateSemaphore(BUFFER_SIZE, BUFFER_SIZE, NULL);
	if (NULL == EmptySemaphoreHandle) {
		return 2;
	}

	//
	// 创建 Full 信号量，表示缓冲池中满缓冲区数量。初始计数为 0，最大计数为 BUFFER_SIZE。
	//
	FullSemaphoreHandle = CreateSemaphore(0, BUFFER_SIZE, NULL);
	if (NULL == FullSemaphoreHandle) {
		return 3;
	}

	//
	// 创建生产者线程。
	//
	ProducerHandle = CreateThread( 0,			// 默认堆栈大小
								   Producer,	// 线程函数入口地址
								   NULL,		// 线程函数参数
								   0,			// 创建标志
								   NULL );		// 线程 ID

	if (NULL == ProducerHandle) {
		return 4;
	}

	//
	// 创建消费者线程。
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
	// 等待生产者线程和消费者线程结束。
	//
	WaitForSingleObject(ProducerHandle, INFINITE);
	WaitForSingleObject(ConsumerHandle, INFINITE);
	
	//
	// 关闭句柄
	//
	CloseHandle(MutexHandle);
	CloseHandle(EmptySemaphoreHandle);
	CloseHandle(FullSemaphoreHandle);
	CloseHandle(ProducerHandle);
	CloseHandle(ConsumerHandle);

	return 0;
}

//
// 生产者线程函数。
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
		// 休息一会。每 500 毫秒生产一个数。
		//
		Sleep(500);
	}
	
	return 0;
}

//
// 消费者线程函数。
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
		// 休息一会儿。让前 10 个数的消费速度比较慢，后面的较快。
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
		// 休息一会儿。让前 14 个数的消费速度比较慢，后面的较快。
		//
		if (i < 14) {
			Sleep(2000);
		} else {
			Sleep(100);
		}
	}
	
	return 0;
}