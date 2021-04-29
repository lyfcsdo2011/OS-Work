/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: eosdef.h

描述: EOS 数据类型、结构体、常量以及宏定义。



*******************************************************************************/

#ifndef _EOSDEF_
#define _EOSDEF_

#ifndef EOS32
#define EOS32
#endif	/* EOS32 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EOSVER
#define EOSVER 0x0500
#endif /* EOSVER */


#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif

#ifdef _KERNEL_
#define EOSAPI	__declspec(dllexport)
#else
#define EOSAPI	__declspec(dllimport)
#endif

#define FALSE		0
#define TRUE		1

#define VOID		void

#define INFINITE	0xFFFFFFFF

#define IN
#define OUT
#define OPTIONAL

#define CONST		const
#define PRIVATE		static
#define PUBLIC

typedef char CHAR, *PCHAR, *PSTR;
typedef const char *PCSTR;
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned char BYTE;
typedef short SHORT, *PSHORT;
typedef unsigned short USHORT, *PUSHORT;
typedef int INT, *PINT;
typedef unsigned int UINT, *PUINT;
typedef long LONG, *PLONG;
typedef unsigned long ULONG, *PULONG;
typedef long long LONGLONG, *PLONGLONG;
typedef unsigned long long ULONGLONG, *PULONGLONG;
typedef int BOOL, *PBOOL;
typedef char BOOLEAN, *PBOOLEAN;
typedef float FLOAT, *PFLOAT;
typedef double DOUBLE, *PDOUBLE;
typedef void *PVOID;
typedef void *HANDLE;
typedef HANDLE *PHANDLE;

typedef union _LARGE_INTEGER {
	struct {
		ULONG LowPart;
		LONG HighPart;
	} u;
	LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

//
// Signed long type for pointer precision. Use when casting a pointer to a long to 
// perform pointer arithmetic. 
//
typedef long LONG_PTR, *PLONG_PTR;

//
// Unsigned LONG_PTR.
//
typedef unsigned long ULONG_PTR, *PULONG_PTR;

//
// The maximum number of bytes to which a pointer can point. Use for a count that must
// span the full range of a pointer. 
//
typedef ULONG_PTR SIZE_T, *PSIZE_T;

//
// Signed SIZE_T.
//
typedef LONG_PTR SSIZE_T, *PSSIZE_T;

//
// 32位的状态码类型。
//
typedef long STATUS, *PSTATUS;

//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+---------------------------+-------------------------------+
//  |Sev|        Reserved           |               Code            |
//  +---+---------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      Reserved - are reserved bits
//
//      Code - is the facility's status code
//

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//
#define EOS_SUCCESS(Status) ((STATUS)(Status) >= 0)

//
// Generic test for information on any status value.
//
#define EOS_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)

//
// Generic test for warning on any status value.
//
#define EOS_WARNING(Status) ((ULONG)(Status) >> 30 == 2)

//
// Generic test for error on any status value.
//
#define EOS_ERROR(Status) ((ULONG)(Status) >> 30 == 3)

//
// 各种类型的最大、最小值。
//
#define MINCHAR			0x80
#define MAXCHAR			0x7f
#define MAXUCHAR		0xff
#define MINSHORT		0x8000
#define MAXSHORT		0x7fff
#define MAXUSHORT		0Xffff
#define MINLONG			0x80000000
#define MAXLONG			0x7fffffff
#define MAXULONG		0xffffffff
#define MINLONG_PTR		0x80000000
#define MAXLONG_PTR		0x7fffffff
#define MAXULONG_PTR	0xffffffff

//
// 用于8字节地址对齐的结构体。
//
typedef struct _QUAD {              // QUAD 仅仅是用于8字节对齐的结构体，
	double  DoNotUseThisField;      // 并不是一个真正的浮点变量，当你需要使用浮点
} QUAD;                             // 变量时请使用double类型。

//
// 大小圆整到指定尺寸的整数倍 或者 地址向上对齐到指定尺寸的边界。
//
#define ROUND_TO_SIZE(SIZE,ALIGNMENT)	(((ULONG_PTR)(SIZE) + ((ULONG_PTR)(ALIGNMENT) - 1)) & ~((ULONG_PTR)(ALIGNMENT) - 1))

//
// 地址向下对齐到指定尺寸的边界。
//
#define ALIGN_TO_SIZE(VA,SIZE) ((PVOID)((ULONG_PTR)(VA) & ~((ULONG_PTR) (SIZE) - 1)))

//
// 判断是否对齐到指定尺寸的边界。
//
#define IS_ALIGNED_TO_SIZE(VA, SIZE) (0 == ((ULONG_PTR)(VA) & ((ULONG_PTR) (SIZE) - 1)))

//
// 大小圆整到双字的整数倍 或者 地址向上对齐到双字的边界。
//
#define ROUND_TO_QUAD(Size)	(((ULONG_PTR)(Size) + 7) & ~7)

//
// 地址向下对齐到双字的边界。
//
#define QUAD_ALIGN(VA)			((PVOID)((ULONG_PTR)(VA) & ~7))

//
// 根据结构体某个域的地址反推结构体的地址。
//
#define CONTAINING_RECORD(address, type, field) ((type *)( \
												(ULONG_PTR)(address) - \
												(ULONG_PTR)(&((type *)0)->field)))
//
// 判断一个整数是否是2的幂。
//
#define IS_POWER_OF_2(a)	(((a) & ((a) - 1)) == 0)

//
// 对指定位测试、置1和清零
//
#define BIT_TEST(va, offset) (va & (1 << (offset)))
#define BIT_SET(va, offset) va = va | (1 << (offset))
#define BIT_CLEAR(va, offset) va = va & (~(1 << (offset)))


#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

//
// AllocationType的可用掩码。用于分配虚拟页。
//
#define MEM_COMMIT			0x1000
#define MEM_RESERVE			0x2000

//
// FreeType的可用掩码。用于释放虚拟页。
//
#define MEM_DECOMMIT		0x4000     
#define MEM_RELEASE			0x8000

//
// 路径字符串最大长度。
//
#define MAX_PATH		260

#define INVALID_HANDLE_VALUE ((HANDLE)-1)

//
// 线程入口函数类型定义。
//
typedef ULONG (*PTHREAD_START_ROUTINE)(
	PVOID ThreadParameter
	);

//
// 进程入口函数类型定义。
//
typedef VOID (*PPROCESS_START_ROUTINE)(VOID);

//
// 进程创建标志。
//
#define DEBUG_PROCESS					0x00000001
#define DEBUG_ONLY_THIS_PROCESS			0x00000002
#define CREATE_SUSPENDED				0x00000004

//
// 进程创建参数结构体。
//
typedef struct _STARTUPINFO {
	HANDLE StdInput;
	HANDLE StdOutput;
	HANDLE StdError;
} STARTUPINFO, *PSTARTUPINFO;

//
// 进程创建结果结构体。
//
typedef struct _PROCESS_INFORMATION { 
	HANDLE ProcessHandle; 
	HANDLE ThreadHandle; 
	ULONG ProcessId; 
	ULONG ThreadId; 
} PROCESS_INFORMATION, *PPROCESS_INFORMATION;

//
// 标准输入输出句柄索引。
//
#define STD_INPUT_HANDLE	1
#define STD_OUTPUT_HANDLE	2
#define STD_ERROR_HANDLE	3

//
// 坐标结构体。
//
typedef struct _COORD {
	SHORT X;
	SHORT Y;
} COORD, *PCOORD;

//
// 键盘事件结构体。
//
typedef struct _KEY_EVENT_RECORD {
	BOOLEAN IsKeyDown;
	UCHAR VirtualKeyValue;
	ULONG ControlKeyState;
} KEY_EVENT_RECORD, *PKEY_EVENT_RECORD;

//
// 控制键状态掩码。
//
#define RIGHT_ALT_PRESSED		0x0001
#define LEFT_ALT_PRESSED		0x0002
#define RIGHT_CTRL_PRESSED		0x0004
#define LEFT_CTRL_PRESSED		0x0008
#define SHIFT_PRESSED			0x0010
#define NUMLOCK_ON				0x0020
#define SCROLLLOCK_ON			0x0040
#define CAPSLOCK_ON				0x0080
#define ENHANCED_KEY			0x0100

//
// 键盘虚键值定义。
// 和扫描码一样，键盘上的每个按键都对应一个唯一的虚键值。
//
#define VK_BACK           0x08
#define VK_TAB            0x09

#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D

#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14

#define VK_ESCAPE         0x1B

#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A

#define VK_INSERT         0x2D
#define VK_DELETE         0x2E

/*
* VK_0 - VK_9 与 ASCII '0' - '9' (0x30 - 0x39) 相同
* 0x40 : 未分配
* VK_A - VK_Z 与 ASCII 'A' - 'Z' (0x41 - 0x5A) 相同
*/

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D

#define VK_SLEEP          0x5F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B

/*
* 0x88 - 0x8F : 未分配
*/

#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91

#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_3          0xC0   // '`~' for US

/*
* 0xC1 - 0xD7 : 保留
*/

/*
* 0xD8 - 0xDA : 未分配
*/

#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_8          0xDF

//
// 控制台字体显示属性定义。
//
#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.

//
// 文件属性定义。
//

#define GENERIC_READ					0x80000000
#define GENERIC_WRITE					0x40000000

#define FILE_SHARE_READ					0x00000001  
#define FILE_SHARE_WRITE				0x00000002  

#define CREATE_NEW						1
#define CREATE_ALWAYS					2
#define OPEN_EXISTING					3
#define OPEN_ALWAYS						4
#define TRUNCATE_EXISTING				5

#define FILE_ATTRIBUTE_READONLY			0x00000001
#define FILE_ATTRIBUTE_HIDDEN			0x00000002
#define FILE_ATTRIBUTE_SYSTEM			0x00000004
#define FILE_ATTRIBUTE_DIRECTORY		0x00000010
#define FILE_ATTRIBUTE_ARCHIVE			0x00000020

#define FILE_BEGIN						0
#define FILE_CURRENT					1
#define FILE_END						2

typedef struct _FILETIME {
	ULONG LowDateTime;
	ULONG HighDateTime;
} FILETIME, *PFILETIME;

//
// PE文件结构相关结构体和常量的定义。
//

#define IMAGE_DOS_SIGNATURE		0x5A4D		// MZ
#define IMAGE_NT_SIGNATURE		0x00004550	// PE00

typedef struct _IMAGE_DOS_HEADER {		// DOS .EXE header
	USHORT	e_magic;					// Magic number
	USHORT	e_cblp;						// Bytes on last page of file
	USHORT	e_cp;						// Pages in file
	USHORT	e_crlc;						// Relocations
	USHORT	e_cparhdr;					// Size of header in paragraphs
	USHORT	e_minalloc;					// Minimum extra paragraphs needed
	USHORT	e_maxalloc;					// Maximum extra paragraphs needed
	USHORT	e_ss;						// Initial (relative) SS value
	USHORT	e_sp;						// Initial SP value
	USHORT	e_csum;						// Checksum
	USHORT	e_ip;						// Initial IP value
	USHORT	e_cs;						// Initial (relative) CS value
	USHORT	e_lfarlc;					// File address of relocation table
	USHORT	e_ovno;						// Overlay number
	USHORT	e_res[4];					// Reserved words
	USHORT	e_oemid;					// OEM identifier (for e_oeminfo)
	USHORT	e_oeminfo;					// OEM information; e_oemid specific
	USHORT	e_res2[10];					// Reserved words
	LONG	e_lfanew;					// File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

//
// File header format.
//
typedef struct _IMAGE_FILE_HEADER {
	USHORT	Machine;
	USHORT	NumberOfSections;
	ULONG	TimeDateStamp;
	ULONG	PointerToSymbolTable;
	ULONG	NumberOfSymbols;
	USHORT	SizeOfOptionalHeader;
	USHORT	Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_SIZEOF_FILE_HEADER			20

#define IMAGE_FILE_RELOCS_STRIPPED			0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE			0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED		0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED		0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM		0x0010  // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE		0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO		0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE			0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED			0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP	0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP		0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM					0x1000  // System File.
#define IMAGE_FILE_DLL						0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY			0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI		0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_I386				0x014c  // Intel 386.

//
// Directory format.
//
typedef struct _IMAGE_DATA_DIRECTORY {
	ULONG	VirtualAddress;
	ULONG	Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES	16

//
// Optional header format.
//
typedef struct _IMAGE_OPTIONAL_HEADER {

	//
	// Standard fields.
	//
	USHORT	Magic;
	UCHAR	MajorLinkerVersion;
	UCHAR	MinorLinkerVersion;
	ULONG	SizeOfCode;
	ULONG	SizeOfInitializedData;
	ULONG	SizeOfUninitializedData;
	ULONG	AddressOfEntryPoint;
	ULONG	BaseOfCode;
	ULONG	BaseOfData;

	//
	// NT additional fields.
	//
	ULONG	ImageBase;
	ULONG	SectionAlignment;
	ULONG	FileAlignment;
	USHORT	MajorOperatingSystemVersion;
	USHORT	MinorOperatingSystemVersion;
	USHORT	MajorImageVersion;
	USHORT	MinorImageVersion;
	USHORT	MajorSubsystemVersion;
	USHORT	MinorSubsystemVersion;
	ULONG	Win32VersionValue;
	ULONG	SizeOfImage;
	ULONG	SizeOfHeaders;
	ULONG	CheckSum;
	USHORT	Subsystem;
	USHORT	DllCharacteristics;
	ULONG	SizeOfStackReserve;
	ULONG	SizeOfStackCommit;
	ULONG	SizeOfHeapReserve;
	ULONG	SizeOfHeapCommit;
	ULONG	LoaderFlags;
	ULONG	NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

#define IMAGE_NT_OPTIONAL_HEADER_MAGIC		0x10b

typedef struct _IMAGE_NT_HEADERS {
	ULONG Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

// Subsystem Values

#define IMAGE_SUBSYSTEM_UNKNOWN				0	// Unknown subsystem.
#define IMAGE_SUBSYSTEM_NATIVE				1	// Image doesn't require a subsystem.
#define IMAGE_SUBSYSTEM_EOS_CUI				80	// Image runs in the EOS character subsystem.

// Directory Entries

#define IMAGE_DIRECTORY_ENTRY_EXPORT			0	// Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT			1	// Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE			2	// Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION			3	// Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY			4	// Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC			5	// Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG				6	// Debug Directory
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE		7	// Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR			8	// RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS				9	// TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG		10	// Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT		11	// Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT				12	// Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT		13	// Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR	14	// COM Runtime descriptor

//
// Section header format.
//

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
	UCHAR	Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		ULONG	PhysicalAddress;
		ULONG	VirtualSize;
    } Misc;
	ULONG	VirtualAddress;
	ULONG	SizeOfRawData;
	ULONG	PointerToRawData;
	ULONG	PointerToRelocations;
	ULONG	PointerToLinenumbers;
	USHORT	NumberOfRelocations;
	USHORT	NumberOfLinenumbers;
	ULONG	Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40

//
// Section characteristics.
//
#define IMAGE_SCN_CNT_CODE					0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA		0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA	0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER					0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO					0x00000200  // Section contains comments or some other type of information.
#define IMAGE_SCN_LNK_REMOVE				0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT				0x00001000  // Section contents comdat.

#define IMAGE_SCN_NO_DEFER_SPEC_EXC			0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL						0x00008000  // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA				0x00008000

#define IMAGE_SCN_MEM_PURGEABLE				0x00020000
#define IMAGE_SCN_MEM_16BIT					0x00020000
#define IMAGE_SCN_MEM_LOCKED				0x00040000
#define IMAGE_SCN_MEM_PRELOAD				0x00080000

#define IMAGE_SCN_ALIGN_1BYTES				0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES				0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES				0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES				0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES				0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES				0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES				0x00700000  //
#define IMAGE_SCN_ALIGN_128BYTES			0x00800000  //
#define IMAGE_SCN_ALIGN_256BYTES			0x00900000  //
#define IMAGE_SCN_ALIGN_512BYTES			0x00A00000  //
#define IMAGE_SCN_ALIGN_1024BYTES			0x00B00000  //
#define IMAGE_SCN_ALIGN_2048BYTES			0x00C00000  //
#define IMAGE_SCN_ALIGN_4096BYTES			0x00D00000  //
#define IMAGE_SCN_ALIGN_8192BYTES			0x00E00000  //
// Unused									0x00F00000
#define IMAGE_SCN_ALIGN_MASK				0x00F00000

#define IMAGE_SCN_LNK_NRELOC_OVFL			0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE			0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED			0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED				0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED				0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE				0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ					0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE					0x80000000  // Section is writeable.

//
// Relocation format.
//
typedef struct _IMAGE_RELOCATION {
	union {
		ULONG	VirtualAddress;
		ULONG	RelocCount;             // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
    } u;
    ULONG	SymbolTableIndex;
    USHORT	Type;
} IMAGE_RELOCATION, *PIMAGE_RELOCATION;

//
// I386 relocation types.
//
#define IMAGE_REL_I386_ABSOLUTE         0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_I386_DIR16            0x0001  // Direct 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_REL16            0x0002  // PC-relative 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32            0x0006  // Direct 32-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32NB          0x0007  // Direct 32-bit reference to the symbols virtual address, base not included
#define IMAGE_REL_I386_SEG12            0x0009  // Direct 16-bit reference to the segment-selector bits of a 32-bit virtual address
#define IMAGE_REL_I386_SECTION          0x000A
#define IMAGE_REL_I386_SECREL           0x000B
#define IMAGE_REL_I386_TOKEN            0x000C  // clr token
#define IMAGE_REL_I386_SECREL7          0x000D  // 7 bit offset from base of section containing target
#define IMAGE_REL_I386_REL32            0x0014  // PC-relative 32-bit reference to the symbols virtual address

//
// Export Format
//
typedef struct _IMAGE_EXPORT_DIRECTORY {
	ULONG	Characteristics;
	ULONG	TimeDateStamp;
	USHORT	MajorVersion;
	USHORT	MinorVersion;
	ULONG	Name;
	ULONG	Base;
	ULONG	NumberOfFunctions;
	ULONG	NumberOfNames;
	ULONG	AddressOfFunctions;     // RVA from base of image
	ULONG	AddressOfNames;         // RVA from base of image
	ULONG	AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

//
// Import Format
//
typedef struct _IMAGE_IMPORT_BY_NAME {
	USHORT	Hint;
	CHAR	Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_THUNK_DATA {
	union {
		ULONG ForwarderString;      // PBYTE 
		ULONG Function;             // PDWORD
		ULONG Ordinal;
		ULONG AddressOfData;        // PIMAGE_IMPORT_BY_NAME
	} u;
} IMAGE_THUNK_DATA, * PIMAGE_THUNK_DATA;

#define IMAGE_ORDINAL_FLAG 0x80000000
#define IMAGE_ORDINAL(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG) != 0)

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		ULONG	Characteristics;            // 0 for terminating null import descriptor
		ULONG	OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
	} u;
	ULONG   TimeDateStamp;                  // 0 if not bound,
                                            // -1 if bound, and real date\time stamp
                                            //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                            // O.W. date/time stamp of DLL bound to (Old BIND)

	ULONG	ForwarderChain;                 // -1 if no forwarders
	ULONG	Name;
	ULONG	FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

#ifdef __cplusplus
}
#endif

#endif // _EOSDEF_
