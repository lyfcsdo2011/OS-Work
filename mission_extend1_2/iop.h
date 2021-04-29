/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: iop.h

描述: IO 模块私有头文件。



*******************************************************************************/

#ifndef _IOP_
#define _IOP_

#include "ke.h"
#include "mm.h"
#include "ob.h"
#include "ps.h"
#include "io.h"
#include "psp.h"

typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;
typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT;
typedef struct _FILE_OBJECT *PFILE_OBJECT;
typedef struct _CONSOLE *PCONSOLE;

//
// 文件属性结构体。
//
typedef struct _FILE_INFO {
	FILETIME CreationTime;
	FILETIME LastAccessTime;
	FILETIME LastWriteTime;
	ULONG FileAttributes;
	ULONG FileSize;
} FILE_INFO, *PFILE_INFO;

//
// 文件属性类别枚举常量定义。
//
typedef enum _FILE_INFO_CLASS {
	FileAttributesInfo,		// 0
	FileRenameInfo,			// 1
	FileDispositionInfo,	// 2
} FILE_INFO_CLASS, *PFILE_INFO_CLASS;

//
// 分类文件属性结构体。
//
typedef struct _FILE_ATTRIBUTES_INFO {
	ULONG FileAttributes;
} FILE_ATTRIBUTES_INFO, *PFILE_ATTRIBUTES_INFO;

typedef struct _FILE_RENAME_INFO {
	BOOL ReplaceIfExists;
	CHAR FileName[1];
} FILE_RENAME_INFO, *PFILE_RENAME_INFO;

typedef struct _FILE_DISPOSITION_INFO {
	BOOL DeleteFile;
} FILE_DISPOSITION_INFO, *PFILE_DISPOSITION_INFO;

//
// 用于分类设置文件属性的结构体。
//
typedef struct _SET_FILE_INFO {
	FILE_INFO_CLASS FileInfoClass;
	union {
		FILE_ATTRIBUTES_INFO AttributesInfo;
		FILE_RENAME_INFO RenameInfo;
		FILE_DISPOSITION_INFO DispositionInfo;
	}u;
}SET_FILE_INFO, *PSET_FILE_INFO;


//
// 驱动程序对象结构体定义。
// 每个驱动程序都对应于一个驱动程序对象，驱动程序对象结构体记录驱动程序所服务的
// 设备以及驱动程序所提供的各个功能函数的入口地址。
//
typedef struct _DRIVER_OBJECT {

	//
	// 设备对象链表头。使一个驱动程序对象可以服务于多个相同类型的设备。
	// 例如串口驱动程序对象可以服务于 PC 机上的COM1、COM2（或更多）串口设备，为
	// 了区分COM1和COM2，分别为COM1和COM2建立一个与之对应的设备对象，同时将两个
	// 设备对象插入串口驱动程序对象的设备对象链表中。这样，通过遍历串口驱动程序
	// 对象的设备对象链表，就可以获知串口驱动程序服务的所有串口设备了。
	//
	LIST_ENTRY DeviceListHead;

	//
	// 下面定义函数指针，用于记录驱动程序提供的各个功能函数的入口地址。
	//

	//
	// AddDevice指向添加设备对象功能函数。系统侦测到设备后，将调用驱动程序提供的
	// 此功能函数，此功能函数应创建设备对应的设备对象并初始化设备。
	//
	STATUS (*AddDevice) (
		IN PDRIVER_OBJECT DriverObject,		// 驱动程序对象指针
		IN PDEVICE_OBJECT NextLayerDevice,	// 下层设备对象指针，例如文件系统设备下层是磁盘设备
		IN USHORT DeviceNumber,				// 设备编号，用于区分识别设备
		OUT PDEVICE_OBJECT *DeviceObject	// 用于返回新建设备对象的指针
		);

	//
	// Create指向了创建/打开文件功能函数。对于文件系统驱动程序，此函数应能完成
	// 创建新文件或打开已有文件的功能；对于串口、键盘等字符设备，此函数应能完成
	// 打开设备对应的文件对象；对于磁盘等块设备，不提供此功能函数。
	//
	STATUS (*Create) (
		IN PDEVICE_OBJECT DeviceObject,	// 设备对象指针
		IN PCSTR FileName,				// 希望打开或创建的文件名字符串，对于字符设备，串的长度必须为0
		IN ULONG CreationDisposition,	// 参见 CreateFile 的 CreationDisposition 参数
		IN OUT PFILE_OBJECT FileObject	// 待初始化文件对象的指针，驱动程序仅需初始化文件对象中FsContext域
		);

	//
	// Close指向关闭文件功能函数。对于文件系统驱动程序，此函数应能完成关闭文件
	// 对象对应的已打开的磁盘文件；对于串口、键盘等字符设备驱动程序，应能完成关
	// 闭文件对象对应的设备；对于磁盘等块设备驱动程序，不提供此功能函数。
	//
	VOID (*Close) (
		IN PDEVICE_OBJECT DeviceObject,	// 设备对象指针
		IN OUT PFILE_OBJECT FileObject	// 文件对象的指针
		);

	//
	// Read指向读功能函数。对于文件系统驱动程序，此函数应能完成读取指定文件
	// 对象对应的磁盘文件；对于串口、键盘等字符设备驱动程序，此函数应能完成读取
	// 指定文件对象对应的字符设备。对于磁盘等块设备驱动程序，此函数应能完成读取
	// 指定块设备的指定扇区。
	//
	STATUS (*Read) (
		IN PDEVICE_OBJECT DeviceObject,	// 设备对象指针
		IN PFILE_OBJECT FileObject,		// 文件对象指针
		OUT PVOID Buffer,				// 指向保存读取结果的缓冲区
		IN ULONG Request,				// 对于磁盘文件和字符设备文件，Request是要读取的字节数，对于块设备，Request是要读取的扇区号
		OUT PULONG Result OPTIONAL		// 对于磁盘文件和字符设备文件，Result指向用于保存实际读取字节数的变量，对于块设备无用
		);

	//
	// Write指向写功能函数。对于文件系统驱动程序，此函数应能完成写指定文件对象
	// 对应的磁盘文件；对于串口等字符设备驱动程序，此函数应能完成写指定文件对象
	// 对应的字符设备。对于磁盘等块设备驱动程序，此函数应能完成写指定块设备的
	// 指定扇区。
	//
	STATUS (*Write) (
		IN PDEVICE_OBJECT DeviceObject, // 设备对象指针
		IN PFILE_OBJECT FileObject,		// 文件对象指针
		IN PVOID Buffer,				// 指向包含写入数据的缓冲区
		IN ULONG Request,				// 对于磁盘文件和字符设备文件，Request是要写的字节数，对于块设备，Request是要写的扇区号
		OUT PULONG Result OPTIONAL		// 对于磁盘文件和字符设备文件，Result指向用于保存实际写入字节数的变量，对于块设备无用
		);

	//
	// Query指向查询功能函数。仅对文件系统驱动程序有效，用于查询指定文件对象
	// 对应的磁盘文件的属性信息，包括最后写入时间、只读属性、文件大小等。
	//
	STATUS (*Query) (
		IN PDEVICE_OBJECT DeviceObject,	// 设备对象指针
		IN PFILE_OBJECT FileObject,		// 文件对象指针
		OUT PFILE_INFO FileInfo			// 指针，指向用于保存查询结果的FILE_INFO结构体。
		);

	//
	// Set指向设置功能函数。仅对文件系统驱动程序有效，用于设置指定文件对象对
	// 应的磁盘文件的大小、是否删除、文件名等属性。
	//
	STATUS (*Set) (
		IN PDEVICE_OBJECT DeviceObject,	// 设备对象指针
		IN PFILE_OBJECT FileObject,		// 文件对象指针
		IN PSET_FILE_INFO FileInfo		// 指针，指向SET_FILE_INFO结构体，包含了要设置的文件属性信息。
		);
} DRIVER_OBJECT;

//
// 设备对象结构体定义。
// 每个设备都对应于一个设备对象，不管是物理设备还是逻辑设备。
//
typedef struct _DEVICE_OBJECT {

	//
	// 块设备标志。
	//
	BOOLEAN IsBlockDevice;

	//
	// 设备编号,用于区分同类型的不同设备。
	// 对块设备文件系统编号时，不区分具体是FAT12、FAT32或EXT2等，都统一编号。例
	// 如软驱0和软驱1的FAT12文件系统编号分别是0和1，设备名分别是A:、B:、如果还
	// 安装有硬盘，则硬盘分区的文件系统编号顺延下去，所以硬盘0的第一个分区的文
	// 件系统编号应为3而不管是什么文件系统，设备名为C:。
	// 其它设备编号的作用域都在同一类设备之内。例如串口0、1的设备编号分别是0和1，
	// 设备名分别是COM1和COM2；软驱0、1的设备编号也是0和1，设备名分别是FLOPPY0和
	// FLOPPY1。
	//
	USHORT DeviceNumber;

	//
	// 驱动程序对象的指针。
	//
	PDRIVER_OBJECT DriverObject;

	//
	// 设备链表的节点。
	//
	LIST_ENTRY DeviceListEntry;

	ULONG OpenCount;
	BOOLEAN ShareRead;
	BOOLEAN ShareWrite;

	//
	// 设备对象扩展块指针。
	//
	PVOID DeviceExtension;

	//
	// 用于互斥访问设备的mutex。
	// 目前对设备进行任何访问之前，都先获取此唯一mutex，故目前不支持全双工操作。
	//
	MUTEX Mutex;
} DEVICE_OBJECT;

//
// 文件对象结构体。
//
typedef struct _FILE_OBJECT {

	//
	// 文件关联的设备对象的指针。
	//
	PDEVICE_OBJECT DeviceObject;

	//
	// 文件的上下文环境块指针，由驱动程序使用。
	// 例如在已实现的FAT12文件系统中，指向了对应的文件控制块结构体。
	//
	PVOID FsContext;

	//
	// 文件对象的权限属性。
	//
	BOOLEAN ReadAccess;
	BOOLEAN WriteAccess;

	//
	// 文件对象的共享属性。
	//
	BOOLEAN SharedRead;
	BOOLEAN SharedWrite;

	//
	// 标志位。
	//
	ULONG FlagsAndAttributes;

	//
	// 文件当前读写偏移位置。
	//
	ULONG CurrentByteOffset;

	//
	// 用于互斥访问文件对象的mutex。
	//
	MUTEX Mutex;
} FILE_OBJECT;

extern POBJECT_TYPE IopDriverObjectType;
extern POBJECT_TYPE IopDeviceObjectType;
extern POBJECT_TYPE IopFileObjectType;
extern POBJECT_TYPE IopConsoleType;

STATUS
IopCreateDriver(
	IN PCSTR DriverName,
	OUT PDRIVER_OBJECT *DriverObject
	);

STATUS
IopCreateDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN ULONG DeviceExtensionSize,
	IN PCSTR DeviceName OPTIONAL,
	IN USHORT DeviceNumber,
	IN BOOL IsBlockDevice,
	OUT PDEVICE_OBJECT *DeviceObject
	);

STATUS
IopReadWriteSector(
	IN PDEVICE_OBJECT Device,
	IN ULONG SectorNumber,
	IN ULONG ByteOffset,
	IN OUT PVOID Buffer,
	IN ULONG BytesToRw,
	IN BOOL Read
	);

STATUS 
IopCreateFileObject(
	IN PSTR FileName, 
	IN ULONG DesiredAccess, 
	IN ULONG ShareMode,
	IN ULONG CreationDisposition, 
	IN ULONG FlagsAndAttributes, 
	OUT PFILE_OBJECT *FileObject
	);

VOID
IopCloseFileObject(
	IN PFILE_OBJECT FileObject
	);

STATUS
IopReadFileObject(
	IN PFILE_OBJECT File,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	);

STATUS
IopWriteFileObject(
	IN PFILE_OBJECT File,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	);

STATUS
IopQueryFileObjectInfo(
	IN PFILE_OBJECT FileObject,
	OUT PFILE_INFO FileInfo
	);

STATUS
IopSetFileObjectInfo(
	IN PFILE_OBJECT FileObject,
	IN PSET_FILE_INFO FileInfo
	);

//
// 下面是环形缓冲池数据结构的定义和相关操作函数的声明。
//
typedef struct _RING_BUFFER {
	ULONG Size;
	ULONG Start;
	ULONG FillCount;
	CHAR Buffer[4];
}RING_BUFFER, *PRING_BUFFER;


PRING_BUFFER 
IopCreateRingBuffer(
	ULONG BufferSize
	);

VOID
IopDeleteRingBuffer(
	PRING_BUFFER RingBuffer
	);

#define IopIsRingBufferEmpty(buf) ((buf)->FillCount == 0)

#define IopIsRingBufferFull(buf) ((buf)->FillCount == (buf)->Size)

#define IopGetRingBufferCount

ULONG
IopWriteRingBuffer(
	IN PRING_BUFFER RingBuffer,
	IN PVOID Data,
	IN ULONG NumberOfBytesToWrite
	);

ULONG
IopReadRingBuffer(
	IN PRING_BUFFER RingBuffer,
	OUT PVOID Data,
	IN ULONG NumberOfBytesToRead
	);

#define IopClearRingBuffer(buf) (buf)->FillCount = 0

//
// 线程对块设备读写请求的结构体
//
typedef struct _REQUEST {
	ULONG Cylinder;			// 线程要访问的磁道号
	LIST_ENTRY ListEntry;	// 请求队列链表项
	EVENT Event;			// 如块设备被其它线程占用，则当前线程要阻塞在该事件上。
}REQUEST, *PREQUEST;

//
// 初始化块设备层。
//
VOID
IopInitializeBlockDeviceLayer(
	VOID
	);


#endif // _IOP_
