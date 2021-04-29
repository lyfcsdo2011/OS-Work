/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: stdio.c

����: C ����ʱ��ı�׼�������ģ�顣



*******************************************************************************/

#include "stdio.h"
#include "stdarg.h"
#include <eos.h>

static int vsprintn(char *buffer, int value, char radix)
{
	register char *p1;
	register char *p2;
	char c = 0;
	int len = 0;

	switch(radix) {
	case 'o':
		radix = 8;
		break;

	case 'u':
		radix = 10;
		break;

	case 'i':
	case 'd':
		radix = 10;
		if(value < 0) {
			value = -value;
			c = '-';
		}
		break;
	
	case 'x':
		radix = 16;
		c = 'a' - '9' - 1;
		break;

	case 'X':
		radix = 16;
		c = 'A' - '9' - 1;
		break;

	default:
		return 0;
	}

	p1 = buffer;
	if(0 == value) {

		*p1++ = '0';

	} else {

		while(value) {

			if((*p1 = '0' + (unsigned int)value % radix) > '9') {
				*p1 += c;
			}

			value = (unsigned int)value / radix;
			p1++;
		}

		if('-' == c) {
			*p1++ = c;
		}
	}

	*p1 = '\0';
	len = p1 - buffer;

	//
	// reverse
	//
	p2 = buffer;
	while(--p1 > p2) {

		c = *p1;
		*p1 = *p2;
		*p2 = c;

		p2++;
	}

	return len;
}

int __cdecl vsprintf(char *buffer, const char *format, va_list argptr)
{
	char *pos;
	char *str;

	for(pos = buffer; *format; format++) {

		if (*format != '%') {
			*pos++ = *format;

		} else {

			format++;	// ������%��

			if('s' == *format) {

				str = va_arg(argptr, char*);
				while(0 != *str) {
					*pos++ = *str++;
				}

			} else if('c' == *format) {

				*pos++ = va_arg(argptr, int);

			} else {

				pos += vsprintn(pos, va_arg(argptr, int), *format);
			}
		}
	}

	*pos = '\0';		// �����ַ���

	va_end(argptr);

	return pos - buffer;
}

int __cdecl sprintf(char *buffer, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	return vsprintf(buffer, format, argptr);
}

int __cdecl putchar(int c)
{
	ULONG n;
	
	if ('\n' == c) {
	
		c = '\r';
		
		if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &c, 1, &n) || n != 1) {
			return EOF;
		}
		
		c = '\n';
	}

	if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &c, 1, &n) || n != 1) {
		return EOF;
	}

	return c;
}

int __cdecl printf(const char *format, ...)
{
	va_list argptr;
	char buffer[33];
	const char* ptr;	// дָ��
	ULONG n;			// д�ֽ���
	ULONG r;			// ʵ��д�ֽ���
	ULONG count = 0;
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

	va_start(argptr, format);

	while (*format) {

		if (*format != '%') {

			if ('\n' == *format) {
				// ��C�еĻ��з�\n�滻ΪEOS�еĻ��з�\r\n
				buffer[0] = '\r';
				buffer[1] = '\n';
				ptr = buffer;
				n = 2;
			} else {
				ptr = format;
				n = 1;
			}

		} else {

			format++;	// ������%��

			if('s' == *format) {

				ptr = va_arg(argptr, char*);
				n = strlen(ptr);

			} else if('c' == *format) {
			
				buffer[0] = va_arg(argptr, int);
				ptr = buffer;
				n = 1;

			} else {

				ptr = buffer;
				n = vsprintn(buffer, va_arg(argptr, int), *format);
			}
		}

		if (!WriteFile(handle, (PVOID)ptr, n, &r) || n != r) {
			break;
		}

		format++;
		count += r;
	}

	va_end(argptr);
	return count;
}

int __cdecl getchar()
{
	char c;
	ULONG n;

	if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &n) || 0 == n) {
		return EOF;
	}

	return c;
}

char* __cdecl gets(char *buffer)
{
	char *ptr;
	ULONG n;
	HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);

	for (ptr = buffer; ; ptr++) {

		//
		// ��ȡһ���ַ��������ȡ��������ļ���������ֹ��ȡ��
		//
		if (!ReadFile(handle, ptr, 1, &n) || 0 == n) {
			*ptr = '\0';
			return ptr == buffer ? NULL : buffer; // ���һ���ַ�Ҳû�ж��򷵻�NULL��
		}

		//
		// ����������з�\n�����\r\n����ֹ��ȡ����\n�����\r\n�滻Ϊ\0��
		//
		if ('\n' == *ptr) {
			
			if (ptr > buffer && '\r' == *(ptr - 1)) {
				ptr--;
			}

			*ptr = '\0';
			return buffer;
		}
	}
}
