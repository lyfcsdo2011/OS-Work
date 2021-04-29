/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: string.h

����: C ����ʱ�� sting ģ���ͷ�ļ���



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
