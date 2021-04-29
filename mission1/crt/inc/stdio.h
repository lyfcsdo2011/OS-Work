/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: stdio.h

����: C ����ʱ�� stdio ģ���ͷ�ļ���



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
