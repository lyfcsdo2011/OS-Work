/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: string.h

描述: C 运行时库 sting 模块的头文件。



*******************************************************************************/

#ifndef _STRING_
#define _STRING_

#ifndef __cplusplus
typedef unsigned long size_t;
#endif

int stricmp( const char *, const char *);
int strcmp( const char *, const char *);
int strncmp( const char *, const char *s, size_t);
int strnicmp( const char *, const char *s, size_t);
char* strcpy( char *, const char *);
char* strncpy( char *, const char *, size_t);
size_t strlen( const char *);
char* strcat( char *, const char *);
char* strncat( char *, const char *, size_t);
int memcmp(const void *, const void *, size_t);
void *memcpy(void *, const void *, size_t);
void *memccpy (void *, const void *, int, size_t);
void *memset(void *, int, size_t);
void *memmove (void *, const void *, size_t);

#endif	// _STRING_
