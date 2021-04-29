/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: rtl.h

描述: 内核运行时库 (run-time library) 头文件。



*******************************************************************************/

#ifndef _RTL_
#define _RTL_

#include "eosdef.h"
#include "status.h"
#include "error.h"

//
// C库中size_t数据类型的定义。
//
#ifndef __cplusplus
typedef unsigned long size_t;
#endif

//
// C库中stdarg部分。
//
typedef char* va_list;
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define va_arg(ap,t) (*(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define va_start(ap,v) (ap = (va_list)&v + _INTSIZEOF(v))
#define va_end(ap) ( ap = (va_list)0 )

//
// C库中的字符串处理部分。
//
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
char* itoa(int value, char *str, int radix);
int vsprintn(char *buffer, int value, char radix, int precision);
int	vsprintf(char *buffer, const char *format, va_list argptr);
int sprintf(char *, const char *, ...);

//
// C库中的setjmp部分。
//
#ifdef _I386

//
// x86的jmp_buf只需保存6个寄存器:ebx,edi,esi,ebp,esp,eip
//
#define _JBTYPE long
#define _JBLEN 6

#endif

typedef _JBTYPE jmp_buf[_JBLEN];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int retval);

int fprintf(HANDLE h, const char *format, ...);
char *fgets(HANDLE h, char *buffer);
int abs(int n);

//
// 将虚键值转换为对应的可见ASCII码
//
CHAR
TranslateKeyToChar(
	IN UCHAR VirtualKeyValue,
	IN ULONG ControlKeyState,
	IN ULONG LocationCode
	);

//
// 将内部状态码转换为错误码
//
ULONG
TranslateStatusToError(
	CONST STATUS status
	);

//
// 双向链表数据结构。
//
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Next;
   struct _LIST_ENTRY *Prev;
} LIST_ENTRY, *PLIST_ENTRY;

//
// 双向链表操作函数。
//
PLIST_ENTRY ListInitializeHead(PLIST_ENTRY ListHead);
BOOL ListIsEmpty(PLIST_ENTRY ListHead);
ULONG ListGetCount (PLIST_ENTRY ListHead);
PLIST_ENTRY ListRemoveEntry(PLIST_ENTRY ListEntry);
PLIST_ENTRY ListRemoveHead(PLIST_ENTRY ListHead);
PLIST_ENTRY ListRemoveTail(PLIST_ENTRY ListHead);
PLIST_ENTRY ListInsertBefore(PLIST_ENTRY Pos, PLIST_ENTRY ListEntry);
PLIST_ENTRY ListInsertAfter(PLIST_ENTRY Pos, PLIST_ENTRY ListEntry);
PLIST_ENTRY ListInsertHead(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry);
PLIST_ENTRY ListInsertTail(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry);


//
// 单向链表数据结构。
//
typedef struct _SINGLE_LIST_ENTRY {
    struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

//
// 单向链表操作函数。
//
PSINGLE_LIST_ENTRY SListInitializeHead(PSINGLE_LIST_ENTRY ListHead);
PSINGLE_LIST_ENTRY SListPopEntry(PSINGLE_LIST_ENTRY ListHead);
PSINGLE_LIST_ENTRY SListPushEntry(PSINGLE_LIST_ENTRY ListHead,
								  PSINGLE_LIST_ENTRY ListEntry);


//
// 一些常用机器指令的抽象。
//
BOOL
BitScanForward(
	OUT ULONG *Index,
	IN ULONG Mask
	);

BOOL
BitScanReverse(
	OUT ULONG *Index,
	IN ULONG Mask
	);

//
// I/O地址空间读写。
//

#ifdef _I386

//
// READ_PORT_* / WRITE_PORT_* 函数操作参数PORT指定的I/O寄存器。
// 在这些函数中使用了x86输入输出指令。
//

UCHAR
READ_PORT_UCHAR(
	PUCHAR  Port
	);

USHORT
READ_PORT_USHORT(
	PUSHORT Port
	);

ULONG
READ_PORT_ULONG(
	PULONG  Port
	);

VOID
READ_PORT_BUFFER_UCHAR(
	PUCHAR  Port,
	PUCHAR  Buffer,
	ULONG   Count
	);

VOID
READ_PORT_BUFFER_USHORT(
	PUSHORT Port,
	PUSHORT Buffer,
	ULONG   Count
	);

VOID
READ_PORT_BUFFER_ULONG(
	PULONG  Port,
	PULONG  Buffer,
	ULONG   Count
	);

VOID
WRITE_PORT_UCHAR(
	PUCHAR  Port,
	UCHAR   Value
	);

VOID
WRITE_PORT_USHORT(
	PUSHORT Port,
	USHORT  Value
	);

VOID
WRITE_PORT_ULONG(
	PULONG  Port,
	ULONG   Value
	);

VOID
WRITE_PORT_BUFFER_UCHAR(
	PUCHAR  Port,
	PUCHAR  Buffer,
	ULONG   Count
	);

VOID
WRITE_PORT_BUFFER_USHORT(
	PUSHORT Port,
	PUSHORT Buffer,
	ULONG   Count
	);

VOID
WRITE_PORT_BUFFER_ULONG(
	PULONG  Port,
	PULONG  Buffer,
	ULONG   Count
	);

#endif

//
// 触发断点异常的函数。
//
#ifdef _I386

#define DbgBreakPoint()	__asm("int $3\n nop")

#endif

//
// 断言的宏定义。
//
#ifdef _DEBUG

#define ASSERT(x) if (!(x)) KeBugCheck("%s:%d:ASSERT!", __FILE__, __LINE__);

#else

#define ASSERT(x)

#endif

#endif // _RTL_
