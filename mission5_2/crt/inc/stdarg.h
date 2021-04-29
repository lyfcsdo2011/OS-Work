/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: stdarg.h

描述: C 运行时库 stdarg 模块的头文件。



*******************************************************************************/

#ifndef _STDARG_
#define _STDARG_

typedef char* va_list;
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define va_arg(ap,t) (*(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define va_start(ap,v) (ap = (va_list)&v + _INTSIZEOF(v))
#define va_end(ap) ( ap = (va_list)0 )

#endif // _STDARG_
