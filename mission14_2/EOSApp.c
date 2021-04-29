/*
提供该示例代码是为了阐释一个概念，或者进行一个测试，并不代表着
最安全的编码实践，因此不应在应用程序或网站中使用该示例代码。对
于超出本示例代码的预期用途以外的使用所造成的偶然或继发性损失，
北京英真时代科技有限公司不承担任何责任。
*/

#include "EOSApp.h"


#define BUFFER_SIZE 1024
BYTE Buffer[BUFFER_SIZE];

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

	HANDLE hFileRead = INVALID_HANDLE_VALUE;
	HANDLE hFileWrite = INVALID_HANDLE_VALUE;
	HANDLE hOutput;
	ULONG m, n;
	int Result = 1;	// 返回值 1，表示执行失败

	//
	// 命令行语法：A:\EOSApp.exe read_file_name [write_file_name] [-a]
	// 举例：A:\EOSApp.exe A:\a.txt
	//       表示将 a.txt 文件中的内容输出到屏幕上
	//       A:\EOSApp.exe A:\a.txt A:\b.txt
	//       表示将 a.txt 文件中的内容写入 b.txt 文件中。b.txt 文件原有的内容会被覆盖。
	//       A:\EOSApp.exe A:\a.txt A:\b.txt -a
	//       表示将 a.txt 文件中的内容附加到 b.txt 文件的末尾。
	//
	if (argc < 2 || argc > 4)
	{
		printf("Error: Invalid argument count!\n"
		       "Valid command line: EOSApp.exe read_file_name [write_file_name] [-a]\n");
		goto RETURN;
	}
	
	//
	// 以只读的方式，打开要读取的文件
	//
	hFileRead = CreateFile(argv[1], GENERIC_READ, 0, OPEN_EXISTING, 0);
	if (INVALID_HANDLE_VALUE == hFileRead)
	{
		printf("Open file \"%s\" error: %d\n", argv[1], GetLastError());
		goto RETURN;
	}
	
	//
	// 如果命令行参数中有要写入的文件，就使用写入文件句柄做为输出句柄，
	// 否则使用标准输出（屏幕）句柄做为输出句柄。
	//
	if (argc > 2)
	{
		//
		// 以只写的方式，打开要写入的文件
		//
		hFileWrite = CreateFile(argv[2], GENERIC_WRITE, 0, OPEN_EXISTING, 0);
		if (INVALID_HANDLE_VALUE == hFileWrite)
		{
			printf("Open file \"%s\" error: %d\n", argv[2], GetLastError());
			goto RETURN;
		}
		
		//
		// 如果是增加写，则将文件指针移动到文件的末尾。
		//
		if (4 == argc && 0 == stricmp(argv[3], "-a"))
		{
			SetFilePointer(hFileWrite, GetFileSize(hFileWrite), FILE_BEGIN);
		}
		
		hOutput = hFileWrite;
	}
	else
	{
		hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	while (TRUE)
	{
		//
		// 尝试读取 BUFFER_SIZE 个字节，实际读取到的字节数由 m 返回。
		//
		ReadFile(hFileRead, Buffer, BUFFER_SIZE, &m);
		
		//
		// 将实际读取到的 m 个字节写入输出句柄。
		//
		if (!WriteFile(hOutput, Buffer, m, &n))
		{
			printf("Write file error: %d\n", GetLastError());
			goto RETURN;
		}
		
		//
		// 如果实际读取的字节数少于预期，说明文件读取完毕。
		//
		if (m < BUFFER_SIZE)
			break;
	}
	
	//
	// 返回值 0，表示执行成功
	//
	Result = 0;
	
RETURN:
	//
	// 关闭文件
	//
	if (hFileRead != INVALID_HANDLE_VALUE)
		CloseHandle(hFileRead);
	if (hFileWrite != INVALID_HANDLE_VALUE)
		CloseHandle(hFileWrite);
	
	return Result;
}
