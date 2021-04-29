/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: rtl.h

����: �ں�����ʱ�� (run-time library) ͷ�ļ���



*******************************************************************************/

#ifndef _RTL_
#define _RTL_

#include "eosdef.h"
#include "status.h"
#include "error.h"

//
// C����size_t�������͵Ķ��塣
//
#ifndef __cplusplus
typedef unsigned long size_t;
#endif

//
// C����stdarg���֡�
//
typedef char* va_list;
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define va_arg(ap,t) (*(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define va_start(ap,v) (ap = (va_list)&v + _INTSIZEOF(v))
#define va_end(ap) ( ap = (va_list)0 )

//
// C���е��ַ��������֡�
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
// C���е�setjmp���֡�
//
#ifdef _I386

//
// x86��jmp_bufֻ�豣��6���Ĵ���:ebx,edi,esi,ebp,esp,eip
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
// �����ֵת��Ϊ��Ӧ�Ŀɼ�ASCII��
//
CHAR
TranslateKeyToChar(
	IN UCHAR VirtualKeyValue,
	IN ULONG ControlKeyState,
	IN ULONG LocationCode
	);

//
// ���ڲ�״̬��ת��Ϊ������
//
ULONG
TranslateStatusToError(
	CONST STATUS status
	);

//
// ˫���������ݽṹ��
//
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Next;
   struct _LIST_ENTRY *Prev;
} LIST_ENTRY, *PLIST_ENTRY;

//
// ˫���������������
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
// �����������ݽṹ��
//
typedef struct _SINGLE_LIST_ENTRY {
    struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

//
// �����������������
//
PSINGLE_LIST_ENTRY SListInitializeHead(PSINGLE_LIST_ENTRY ListHead);
PSINGLE_LIST_ENTRY SListPopEntry(PSINGLE_LIST_ENTRY ListHead);
PSINGLE_LIST_ENTRY SListPushEntry(PSINGLE_LIST_ENTRY ListHead,
								  PSINGLE_LIST_ENTRY ListEntry);


//
// һЩ���û���ָ��ĳ���
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
// I/O��ַ�ռ��д��
//

#ifdef _I386

//
// READ_PORT_* / WRITE_PORT_* ������������PORTָ����I/O�Ĵ�����
// ����Щ������ʹ����x86�������ָ�
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
// �����ϵ��쳣�ĺ�����
//
#ifdef _I386

#define DbgBreakPoint()	__asm("int $3\n nop")

#endif

//
// ���Եĺ궨�塣
//
#ifdef _DEBUG

#define ASSERT(x) if (!(x)) KeBugCheck("%s:%d:ASSERT!", __FILE__, __LINE__);

#else

#define ASSERT(x)

#endif

#endif // _RTL_
