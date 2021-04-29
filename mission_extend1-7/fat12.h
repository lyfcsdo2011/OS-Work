/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: fat12.h

描述: FAT12文件系统驱动程序的内部头文件。



*******************************************************************************/

#ifndef _FAT12_
#define _FAT12_

#include "eosdef.h"


#pragma pack(1)		// 通知编译器按照单个字节方式对齐下列结构体中的成员

//
// BIOS Parameter Block (BPB) 结构体。大小为 25 个字节。
//
typedef struct _BIOS_PARAMETER_BLOCK {
	USHORT BytesPerSector;					// 每扇区字节数
	UCHAR  SectorsPerCluster;				// 每簇扇区数
	USHORT ReservedSectors;					// 保留扇区数
	UCHAR  Fats;							// FAT表的数量
	USHORT RootEntries;						// 根目录项数
	USHORT Sectors;							// 扇区总数
	UCHAR  Media;							// 介质描述
	USHORT SectorsPerFat;					// 每FAT表占用的扇区数
	USHORT SectorsPerTrack;					// 每磁道扇区数
	USHORT Heads;							// 磁头数
	ULONG  HiddenSectors;					// 引导扇区前的扇区数
	ULONG  LargeSectors;					// 扇区总数(Sectors 为 0 时使用)
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

//
// 引导扇区结构体。只包含引导扇区开始的 62 个字节，不包含余下的 450 个字节。
//
typedef struct _BOOT_SECTOR {
	UCHAR Jump[3];							// 跳转指令。			偏移 00。
	UCHAR Oem[8];							// OEM。				偏移 03。
	BIOS_PARAMETER_BLOCK Bpb;				// BPB。				偏移 11。
	UCHAR DriveNumber;						// 物理驱动器编号。		偏移 36。
	UCHAR Reserved;							// 保留。				偏移 37。
	UCHAR Signature;						// 扩展引导扇区标志。	偏移 38。
	UCHAR Id[4];							// 卷序列号。			偏移 39。
	UCHAR VolumeLabel[11];					// 卷标。				偏移 43。
	UCHAR SystemId[8];						// 文件系统类型。		偏移 54。
} BOOT_SECTOR, *PBOOT_SECTOR;

#pragma pack()		// 恢复默认对齐方式

//
// 复制字符的宏
//
#define CopyUchar1(dest, src)					  \
	*((PUCHAR)(dest)) = *((PUCHAR)(src))		  \

#define CopyUchar2(dest, src) {					  \
	*((PUCHAR)(dest)) = *((PUCHAR)(src));		  \
	*((PUCHAR)(dest) + 1) = *((PUCHAR)(src) + 1); \
}
	
#define CopyUchar3(dest, src) {					  \
	*((PUCHAR)(dest)) = *((PUCHAR)(src));		  \
	*((PUCHAR)(dest) + 1) = *((PUCHAR)(src) + 1); \
	*((PUCHAR)(dest) + 2) = *((PUCHAR)(src) + 2); \
}

#define CopyUchar4(dest, src) {					  \
	*((PUCHAR)(dest)) = *((PUCHAR)(src));		  \
	*((PUCHAR)(dest) + 1) = *((PUCHAR)(src) + 1); \
	*((PUCHAR)(dest) + 2) = *((PUCHAR)(src) + 2); \
	*((PUCHAR)(dest) + 3) = *((PUCHAR)(src) + 3); \
}

//
//  Fat 文件系统使用的时间\日期结构体。Note that the
//  following structure is a 32 bits long but USHORT aligned.
//
typedef struct _FAT_TIME {
	USHORT DoubleSeconds : 5;
	USHORT Minute        : 6;
	USHORT Hour          : 5;
} FAT_TIME, *PFAT_TIME;

typedef struct _FAT_DATE {
	USHORT Day           : 5;
	USHORT Month         : 4;
	USHORT Year          : 7; // 从 1980 年起始。
} FAT_DATE, *PFAT_DATE;

//
// 目录项结构体 (Directory Entry)。大小为 32 个字节。
//
typedef struct _DIRENT {
	CHAR Name[11];							// 文件名 8 字节，扩展名 3 字节
	UCHAR Attributes;						// 文件属性
	UCHAR Reserved[10];						// 保留未用
	FAT_TIME LastWriteTime;					// 文件最后修改时间
	FAT_DATE LastWriteDate;					// 文件最后修改日期
	USHORT FirstCluster;					// 文件的第一个簇号
	ULONG FileSize;							// 文件大小
} DIRENT, *PDIRENT;

//
// Fat12 文件系统设备对象的扩展结构体——卷控制块（Volume Control Block)。
//
typedef struct _VCB {

	//
	// 文件系统下层的软盘或者硬盘卷设备对象(目前仅仅是软盘)。
	//
	PDEVICE_OBJECT DiskDevice;

	//
	// 文件系统的参数。
	//
	BIOS_PARAMETER_BLOCK Bpb;

	//
	// 文件分配表（File Allocation Table）缓冲区。FAT12 的 FAT 表最大不过 6KB，所以
	// 完全加载到内存中比较合适。
	//
	PVOID Fat;

	//
	// 根目录起始扇区、根目录大小以及根目录文件链表头。
	//
	ULONG FirstRootDirSector;
	ULONG RootDirSize;
	LIST_ENTRY FileListHead;

	//
	// 文件数据区的起始扇区以及簇的总数。
	//
	ULONG FirstDataSector;
	USHORT NumberOfClusters;
}VCB, *PVCB;

//
// 文件控制块（File Control Block）。
//
typedef struct _FCB {

	//
	// 文件名字符串。
	//
	CHAR Name[13];

	//
	// 文件属性。
	//
	BOOLEAN AttrReadOnly;
	BOOLEAN AttrHidden;
	BOOLEAN AttrSystem;
	BOOLEAN AttrDirectory;

	//
	// 共享属性。
	//
	BOOLEAN SharedRead;
	BOOLEAN SharedWrite;

	//
	// 文件最后修改时间。
	//
	FAT_TIME LastWriteTime;
	FAT_DATE LastWriteDate;

	//
	// 文件的第一个簇号和文件的大小。
	//
	USHORT FirstCluster;
	ULONG FileSize;

	//
	// 目录项结构体在目录文件内的偏移。
	//
	ULONG DirEntryOffset;

	//
	// 文件打开计数器。归档文件每被打开一次，OpenCount 增加 1；目录内每打开一个文
	// 件，目录文件的 OpenCount 增加 1。关闭文件时，OpenCount 减小 1。如果变为 0 
	// 则释放 FCB 节点，同时文件所在目录的 OpenCount 递归减小 1。
	//
	ULONG OpenCount;

	//
	// 文件所在目录的指针，如果为 NULL 说明文件位于根目录。
	//
	struct _FCB *ParentDirectory;

	//
	// 文件所在目录的已打开文件链表项。
	//
	LIST_ENTRY FileListEntry;

	//
	// 目录的已打开文件链表头，当 AttrDirectory 为 TRUE 时有效。
	//
	LIST_ENTRY FileListHead;
}FCB, *PFCB;

//
// 文件属性的位定义
//
#define DIRENT_ATTR_READ_ONLY		0x01
#define DIRENT_ATTR_HIDDEN			0x02
#define DIRENT_ATTR_SYSTEM			0x04
#define DIRENT_ATTR_DIRECTORY		0x10
#define DIRENT_ATTR_ARCHIVE			0x20

//
// 计算簇的大小
//
#define FatBytesPerCluster(B) ((ULONG)((B)->BytesPerSector * (B)->SectorsPerCluster))

//
// 计算 FAT 表的大小
//
#define FatBytesPerFat(B) ((ULONG)((B)->BytesPerSector * (B)->SectorsPerFat))

//
// 计算根目录的起始扇区
//
#define FatFirstRootDirSector(B) ((B)->ReservedSectors + ((B)->Fats * (B)->SectorsPerFat))

//
// 计算根目录的大小
//
#define FatRootDirSize(B) ((ULONG)((B)->RootEntries * sizeof(DIRENT)))

//
// 计算根目录的扇区数量
//
#define FatRootDirSectors(B) ((ULONG)((FatRootDirSize(B) + ((B)->BytesPerSector - 1)) / (B)->BytesPerSector))

//
// 计算数据区的起始扇区
//
#define FatFirstDataSector(B) (FatFirstRootDirSector(B) + FatRootDirSectors(B))

//
// 计算簇的数量
//
#define FatNumberOfClusters(B)											\
	((((B)->Sectors ? (B)->Sectors : (B)->LargeSectors)					\
																		\
        -	((B)->ReservedSectors +										\
			(B)->Fats * (B)->SectorsPerFat +                            \
			FatRootDirSectors(B) ) )									\
																		\
									/									\
																		\
						(B)->SectorsPerCluster)							\


STATUS
FatCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN ULONG CreationDisposition,
	IN OUT PFILE_OBJECT FileObject
	);

VOID
FatClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PFILE_OBJECT FileObject
	);

STATUS
FatRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result
	);

STATUS
FatWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result
	);

STATUS
FatQuery(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PFILE_INFO FileInfo
	);

STATUS
FatSet(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PSET_FILE_INFO FileInfo
	);

STATUS
FatAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT NextLayerDevice,
	IN USHORT DeviceNumber,
	OUT PDEVICE_OBJECT *DeviceObject
	);

//
// 判断文件或者文件夹路径名称是否有效。
//
BOOL
FatCheckPath(
	IN PCSTR PathName,
	IN BOOL IsDirectoryName
	);

USHORT
FatGetFatEntryValue(
	IN PVCB Vcb,
	IN USHORT Index
	);

STATUS
FatSetFatEntryValue(
	IN PVCB Vcb,
	IN USHORT Index,
	IN USHORT Value12
	);

STATUS
FatOpenExistingFile(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN OUT PFILE_OBJECT FileObject
	);

STATUS
FatWriteDirEntry(
	IN PVCB Vcb,
	IN PFCB Fcb
	);

VOID
FatCloseFile(
	IN PFCB Fcb
	);

STATUS
FatReadFile(
	IN PVCB Vcb,
	IN PFCB Fcb,
	IN ULONG Offset,
	IN ULONG BytesToRead,
	OUT PVOID Buffer,
	OUT PULONG BytesRead
	);

STATUS
FatWriteFile(
	IN PVCB Vcb,
	IN PFCB File,
	IN ULONG Offset,
	IN ULONG BytesToWrite,
	OUT PVOID Buffer,
	OUT PULONG BytesWriten
	);

#endif // _FAT12_
