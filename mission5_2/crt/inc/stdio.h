/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: stdio.h

描述: C 运行时库 stdio 模块的头文件。



*******************************************************************************/

#ifndef _STDIO_
#define _STDIO_

#include "stdarg.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define EOF     (-1);

int getchar();
char *gets(char *buffer);
int putchar(int c);
int printf(const char *format, ...);
int sprintf(char *buffer, const char *format, ...);
int	vsprintf(char *buffer, const char *format, va_list argptr);


#ifdef __cplusplus
}
#endif

#endif // _STDIO_
