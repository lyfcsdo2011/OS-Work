/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: crt.c

描述: 内核 C 运行库函数。



*******************************************************************************/

#include "rtl.h"
#include "ob.h"


#define UPPER( c ) ( ( (c) >= 'a' && (c) <= 'z' ) ? ((c) - 'a' + 'A') : (c) )
#define LOWER( c ) ( ( (c) >= 'A' && (c) <= 'Z' ) ? ((c) - 'A' + 'a') : (c) )

int stricmp( const char *string1, const char *string2 )
{
	register const char *p1 = string1;
	register const char *p2 = string2;

	while(UPPER(*p1) == UPPER(*p2) && 0 != *p1) {
		p1++;
		p2++;
	}

	return UPPER(*p1) - UPPER(*p2);
}

int strcmp(const char * string1, const char * string2)
{
	register const char *p1 = string1;
	register const char *p2 = string2;
	
	while(*p1 == *p2 && 0 != *p1) {
		p1++;
		p2++;
	}

	return *p1 - *p2;
}

int strnicmp( const char *string1, const char *string2, size_t count)
{
	register size_t i;
	register const char *p1 = string1;
	register const char *p2 = string2;
	
	for (i = count; i > 0; i--) {

		if (UPPER(*p1) != UPPER(*p2) || 0 == *p1) {
			return UPPER(*p1) - UPPER(*p2);
		}

		p1++;
		p2++;
	}

	return 0;
}

int strncmp( const char *string1, const char *string2, size_t count)
{
	register size_t i;
	register const char *p1 = string1;
	register const char *p2 = string2;

	for (i = count; i > 0; i--) {

		if (*p1 != *p2 || 0 == *p1) {
			return *p1 - *p2;
		}

		p1++;
		p2++;
	}

	return 0;
}

char* strcpy( char *dst, const char *src )
{
	register char *p1 = dst;
	register const char *p2 = src;

	while(0 != *p2) {
		*p1++ = *p2++;
	}
	*p1 = 0;

	return dst;
}

char* strncpy( char *dst, const char *src, size_t count )
{
	register size_t i;
	register char *p1 = dst;
	register const char *p2 = src;

	for (i = count; i > 0 && *p2 != 0; i--) {
		*p1++ = *p2++;
	}
	*p1 = 0;

	return dst;
}

size_t strlen( const char *string )
{
	register const char *p = string;

	while(0 != *p) {
		p++;
	}

	return p - string;
}

char *strcat( char *dst, const char *src )
{
	register char *p1 = dst;
	register const char *p2 = src;

	while(0 != *p1) {
		p1++;
	}

	while(0 != *p2) {
		*p1++ = *p2++;
	}

	*p1 = 0;

	return dst;
}

char *strncat( char *dst, const char *src, size_t count )
{
	register size_t i;
	register char *p1 = dst;
	register const char *p2 = src;

	while(0 != *p1) {
		p1++;
	}

	for (i = count; i > 0 && *p2 != 0; i--) {
		*p1++ = *p2++;
	}

	*p1 = 0;

	return dst;
}

int sprintf(char *buffer, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	return vsprintf(buffer, format, argptr);
}

int memcmp(const void *buf1, const void *buf2, size_t count)
{
	if(!count) {
		return 0;
	}

	while (--count && *(char *)buf1 == *(char *)buf2) {
		buf1 = (char *)buf1 + 1;
		buf2 = (char *)buf2 + 1;
	}

	return *((unsigned char *)buf1) - *((unsigned char *)buf2);
}

void * memccpy (void * dst, const void * src, int c, size_t count)
{
	while ( count && (*((char *)(dst = (char *)dst + 1) - 1) =
		*((char *)(src = (char *)src + 1) - 1)) != (char)c )
		count--;

	return count ? dst : (void*)0;
}


void* memcpy (void * dst, const void * src, size_t count)
{
	void * ret = dst;

	while (count--)  {
		*(char *)dst = *(char *)src;
		dst = (char *)dst + 1;
		src = (char *)src + 1;
	}

	return ret;
}

void * memmove (void * dst, const void * src, size_t count)
{
	void * ret = dst;


	if (dst <= src || (char *)dst >= ((char *)src + count)) {

		while (count--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst + 1;
			src = (char *)src + 1;
		}

	} else {
		
		dst = (char *)dst + count - 1;
		src = (char *)src + count - 1;

		while (count--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst - 1;
			src = (char *)src - 1;
		}
	}

	return ret;
}

void * memset (void *dst, int val, size_t count)
{
	void *start = dst;

	while (count--) {
		*(char *)dst = (char)val;
		dst = (char *)dst + 1;
	}

	return start;
}

char* itoa(int value, char *str, int radix)
{
	register char *p1;
	register char *p2;
	char c = 0;

	//
	// 如果基数超出范围 [2，36]，则设置字符串为空串并返回。
	//
	if(radix < 2 || 36 < radix ) {
		*str = 0;
		return str;
	}

	p1 = str;
	if(0 == value) {

		*p1++ = '0';

	} else {
		//
		// 如果以10为基负数，则暂存其符号并取其绝对值。
		//
		if(10 == radix && value < 0) {
			c = '-';
			value = -value;
		}

		while(value) {

			if((*p1 = '0' + (unsigned int)value % radix) > '9')
				*p1 += 'A' - '9' - 1;
			
			value = (unsigned int)value / radix;
			p1++;
		}

		if('-' == c) {
			*p1++ = c;
		}
	}
	*p1 = '\0';		// 结束字符串

	//
	// reverse
	//
	p2 = str;
	while(--p1 > p2) {

		c = *p1;
		*p1 = *p2;
		*p2 = c;

		p2++;
	}

	return str;
}

long atol(const char *str)
{
	int c;
	long total;
	int sign;
	
	//
	// 忽略开始的空白
	//
	while ( ' ' == (int)(unsigned char)*str
			|| '\t' == (int)(unsigned char)*str )
		++str;
	
	//
	// 处理符号
	//
	c = (int)(unsigned char)*str++;
	sign = c;
	if ('-' == c || '+' == c)
		c = (int)(unsigned char)*str++;
	
	total = 0;
	while (c >= '0' && c <= '9') {
		total = 10 * total + (c - '0');
		c = (int)(unsigned char)*str++;
	}
	
	if ('-' == sign)
		return -total;
	else
		return total;
}

int atoi(const char *str){ return (int)atol(str); }

int vsprintn(char *buffer, int value, char radix, int precision)
{
	register char *p1;
	register char *p2;
	char c = 0;
	int len = 0;

	switch(radix) {
	case 'o':
		radix = 8;
		if (precision > 11) {
			precision = 11;
		}
		break;

	case 'u':
		radix = 10;
		if (precision > 10) {
			precision = 10;
		}
		break;

	case 'i':
	case 'd':
		radix = 10;
		if (precision > 10) {
			precision = 10;
		}
		if(value < 0) {
			value = -value;
			c = '-';
		}
		break;
	
	case 'x':
		radix = 16;
		if (precision > 8) {
			precision = 8;
		}
		c = 'a' - '9' - 1;
		break;

	case 'X':
		if (precision > 8) {
			precision = 8;
		}
		radix = 16;
		c = 'A' - '9' - 1;
		break;

	default:
		return 0;
	}

	p1 = buffer;
	if(0 == value) {

		*p1++ = '0';

		for (precision--; precision > 0; precision--) {
			*p1++ = '0';
		}

	} else {

		while(value) {

			if((*p1 = '0' + (unsigned int)value % radix) > '9') {
				*p1 += c;
			}

			value = (unsigned int)value / radix;
			p1++;
		}

		for (precision -= (p1 - buffer); precision > 0; precision--) {
			*p1++ = '0';
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

int vsprintf(char *buffer, const char *format, va_list argptr)
{
	char *pos;
	char *str;
	char flag;
	int width;
	int precision;
	char digit[12];

	for(pos = buffer; *format; format++) {

		if (*format != '%') {
			*pos++ = *format;

		} else {

			format++;

			flag = '+';
			if ('-' == *format || '+' == *format || '0' == *format ||
				' ' == *format || '#' == *format) {
				flag = *format++;
			}

			width = 0;
			while ('0' <= *format && *format <= '9') {
				width = width * 10 + (*format++ - '0');
			}

			precision = 0;
			if ('.' == *format) {
				for (format++; '0' <= *format && *format <= '9'; format++) {
					precision = precision * 10 + (*format - '0');
				}
			}

			if('s' == *format) {

				str = va_arg(argptr, char*);

			} else if('c' == *format) {

				digit[0] = va_arg(argptr, char);
				digit[1] = 0;
				str = digit;

			} else if ( 'o' == *format || 'u' == *format || 'i' == *format ||
						'd' == *format || 'x' == *format || 'X' == *format){

				vsprintn(digit, va_arg(argptr, int), *format, precision);
				str = digit;

			} else {

				str = NULL;
			}

			if (NULL != str) {

				if ('+' == flag) {
					for (width -= strlen(str); width > 0; width--) {
						*pos++ = ' ';
					}
				}

				while(0 != *str) {
					*pos++ = *str++;
				}

				if ('-' == flag) {
					for (width -= strlen(str); width > 0; width--) {
						*pos++ = ' ';
					}
				}
			}
		}
	}

	*pos = '\0';		// 结束字符串

	va_end(argptr);

	return pos - buffer;
}

int fprintf(HANDLE h, const char *format, ...)
{
	va_list argptr;
	char flag;
	int width;
	int precision;
	char buffer[13];
	const char* ptr;	// 写指针
	ULONG n;			// 写字节数
	ULONG r;			// 实际写字节数
	ULONG count = 0;
	STATUS status;

	va_start(argptr, format);

	while (*format) {

		width = 0;

		if (*format != '%') {

			if ('\n' == *format) {
				// 将C中的换行符\n替换为EOS中的换行符\r\n
				buffer[0] = '\r';
				buffer[1] = '\n';
				ptr = buffer;
				n = 2;
			} else {
				ptr = format;
				n = 1;
			}

		} else {

			format++;	// 跳过‘%’

			flag = '+';
			if ('-' == *format || '+' == *format || '0' == *format ||
				' ' == *format || '#' == *format) {
				flag = *format++;
			}

			while ('0' <= *format && *format <= '9') {
				width = width * 10 + (*format++ - '0');
			}

			precision = 0;
			if ('.' == *format) {
				for (format++; '0' <= *format && *format <= '9'; format++) {
					precision = precision * 10 + (*format - '0');
				}
			}

			if('s' == *format) {

				ptr = va_arg(argptr, char*);
				n = strlen(ptr);

			} else if('c' == *format) {

				buffer[0] = va_arg(argptr, int);
				ptr = buffer;
				n = 1;

			} else if ( 'o' == *format || 'u' == *format || 'i' == *format ||
						'd' == *format || 'x' == *format || 'X' == *format) {

				ptr = buffer;
				n = vsprintn(buffer, va_arg(argptr, int), *format, precision);

			} else {

				n = 0;
			}
		}

		if (n > 0) {

			width -= n;

			if ('+' == flag && width > 0) {

				while (width > 0) {
					
					status = ObWrite(h, " ", 1, &r);
					if (!EOS_SUCCESS(status) || 1 != r) {
						return count;
					}
					count++;
					width--;
				}
			}

			status = ObWrite(h, (PVOID)ptr, n, &r);
			if (!EOS_SUCCESS(status) || n != r) {
				break;
			}
			count += n;

			if ('-' == flag && width > 0) {

				while (width > 0) {

					status = ObWrite(h, " ", 1, &r);
					if (!EOS_SUCCESS(status) || 1 != r) {
						return count;
					}
					count++;
					width--;
				}
			}
		}

		format++;
	}

	va_end(argptr);
	return count;
}

char *fgets(HANDLE h, char *buffer)
{
	char *BufferPtr;
	ULONG BytesRead;
	STATUS Status;

	for (BufferPtr = buffer; ; BufferPtr++) {

		//
		// 读取一个字符，如果读取错误或这文件结束则中止读取。
		//
		Status = ObRead(h, BufferPtr, 1, &BytesRead);

		if (!EOS_SUCCESS(Status) || 0 == BytesRead) {
			*BufferPtr = '\0';
			return BufferPtr == buffer ? NULL : buffer; // 如果一个字符也没有读则返回NULL。
		}

		//
		// 如果遇到换行符\n或组合\r\n则中止读取，将\n或组合\r\n替换为\0。
		//
		if ('\n' == *BufferPtr) {
			
			if (BufferPtr > buffer && '\r' == *(BufferPtr - 1)) {
				BufferPtr--;
			}

			*BufferPtr = '\0';
			return buffer;
		}
	}
}

int abs(int n)
{
	//
	// 取 n 的绝对值
	//
	return n < 0 ? -n : n;
}
