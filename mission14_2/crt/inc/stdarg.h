/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: stdarg.h

����: C ����ʱ�� stdarg ģ���ͷ�ļ���



*******************************************************************************/

#ifndef _STDARG_
#define _STDARG_

typedef char* va_list;
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define va_arg(ap,t) (*(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define va_start(ap,v) (ap = (va_list)&v + _INTSIZEOF(v))
#define va_end(ap) ( ap = (va_list)0 )

#endif // _STDARG_
