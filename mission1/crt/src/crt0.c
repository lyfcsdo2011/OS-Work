/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: crt0.c

描述: CRT 运行库的 CRT0 模块，负责应用程序的启动初始化。



*******************************************************************************/

#include <eos.h>

typedef void (*func_ptr) (void);

extern func_ptr __DTOR_LIST__[];

static void __do_global_dtors (void)
{
	func_ptr *p;

	//
	// 依次调用全局对象的析构函数。
	//
	for (p = __DTOR_LIST__ + 1; *p; p++) {
		(*(p)) ();
	}
}

extern int main(int argc, char* argv[]);

void _start (void)
{
	int retv;
	int argc;
	char *ptr;

	static char ImageName[MAX_PATH];
	static char CmdLine[1024];
	static char* argv[512];

	//
	// 得到命令行参数并分割为多个字符串，传递给main函数。
	//
	GetImageNameAndCmdLine(ImageName, CmdLine);

	argc = 1;
	argv[0] = ImageName;

	for (ptr = CmdLine; *ptr != 0; ) {

		//
		// 过滤空格。
		//
		while (*ptr != 0 && *ptr == ' ') {
			ptr++;
		}

		if (*ptr == 0) {
			break;
		}

		argv[argc++] = ptr;

		while (*ptr != 0 && *ptr != ' '){
			ptr++;
		}

		//
		// 隔断字符串。
		//
		if (' ' == *ptr) {
			*ptr++ = 0;
		}
	}

	//
	// 调用main函数。
	//
	retv = main(argc, argv);

	//
	// 调用全局对象析构函数。
	//
	__do_global_dtors();

	//
	// 退出进程。
	//
	ExitProcess(retv);
}
