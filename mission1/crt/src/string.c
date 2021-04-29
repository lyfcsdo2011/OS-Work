/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: string.c

描述: C 运行时库的 string 模块。



*******************************************************************************/

#include "string.h"

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
