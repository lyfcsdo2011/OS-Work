/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: iop.h

����: IO ģ��˽��ͷ�ļ���



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
// �ļ����Խṹ�塣
//
typedef struct _FILE_INFO {
	FILETIME CreationTime;
	FILETIME LastAccessTime;
	FILETIME LastWriteTime;
	ULONG FileAttributes;
	ULONG FileSize;
} FILE_INFO, *PFILE_INFO;

//
// �ļ��������ö�ٳ������塣
//
typedef enum _FILE_INFO_CLASS {
	FileAttributesInfo,		// 0
	FileRenameInfo,			// 1
	FileDispositionInfo,	// 2
} FILE_INFO_CLASS, *PFILE_INFO_CLASS;

//
// �����ļ����Խṹ�塣
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
// ���ڷ��������ļ����ԵĽṹ�塣
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
// �����������ṹ�嶨�塣
// ÿ���������򶼶�Ӧ��һ������������������������ṹ���¼���������������
// �豸�Լ������������ṩ�ĸ������ܺ�������ڵ�ַ��
//
typedef struct _DRIVER_OBJECT {

	//
	// �豸��������ͷ��ʹһ���������������Է����ڶ����ͬ���͵��豸��
	// ���紮���������������Է����� PC ���ϵ�COM1��COM2������ࣩ�����豸��Ϊ
	// ������COM1��COM2���ֱ�ΪCOM1��COM2����һ����֮��Ӧ���豸����ͬʱ������
	// �豸������봮���������������豸���������С�������ͨ������������������
	// ������豸���������Ϳ��Ի�֪�������������������д����豸�ˡ�
	//
	LIST_ENTRY DeviceListHead;

	//
	// ���涨�庯��ָ�룬���ڼ�¼���������ṩ�ĸ������ܺ�������ڵ�ַ��
	//

	//
	// AddDeviceָ������豸�����ܺ�����ϵͳ��⵽�豸�󣬽��������������ṩ��
	// �˹��ܺ������˹��ܺ���Ӧ�����豸��Ӧ���豸���󲢳�ʼ���豸��
	//
	STATUS (*AddDevice) (
		IN PDRIVER_OBJECT DriverObject,		// �����������ָ��
		IN PDEVICE_OBJECT NextLayerDevice,	// �²��豸����ָ�룬�����ļ�ϵͳ�豸�²��Ǵ����豸
		IN USHORT DeviceNumber,				// �豸��ţ���������ʶ���豸
		OUT PDEVICE_OBJECT *DeviceObject	// ���ڷ����½��豸�����ָ��
		);

	//
	// Createָ���˴���/���ļ����ܺ����������ļ�ϵͳ�������򣬴˺���Ӧ�����
	// �������ļ���������ļ��Ĺ��ܣ����ڴ��ڡ����̵��ַ��豸���˺���Ӧ�����
	// ���豸��Ӧ���ļ����󣻶��ڴ��̵ȿ��豸�����ṩ�˹��ܺ�����
	//
	STATUS (*Create) (
		IN PDEVICE_OBJECT DeviceObject,	// �豸����ָ��
		IN PCSTR FileName,				// ϣ���򿪻򴴽����ļ����ַ����������ַ��豸�����ĳ��ȱ���Ϊ0
		IN ULONG CreationDisposition,	// �μ� CreateFile �� CreationDisposition ����
		IN OUT PFILE_OBJECT FileObject	// ����ʼ���ļ������ָ�룬������������ʼ���ļ�������FsContext��
		);

	//
	// Closeָ��ر��ļ����ܺ����������ļ�ϵͳ�������򣬴˺���Ӧ����ɹر��ļ�
	// �����Ӧ���Ѵ򿪵Ĵ����ļ������ڴ��ڡ����̵��ַ��豸��������Ӧ����ɹ�
	// ���ļ������Ӧ���豸�����ڴ��̵ȿ��豸�������򣬲��ṩ�˹��ܺ�����
	//
	VOID (*Close) (
		IN PDEVICE_OBJECT DeviceObject,	// �豸����ָ��
		IN OUT PFILE_OBJECT FileObject	// �ļ������ָ��
		);

	//
	// Readָ������ܺ����������ļ�ϵͳ�������򣬴˺���Ӧ����ɶ�ȡָ���ļ�
	// �����Ӧ�Ĵ����ļ������ڴ��ڡ����̵��ַ��豸�������򣬴˺���Ӧ����ɶ�ȡ
	// ָ���ļ������Ӧ���ַ��豸�����ڴ��̵ȿ��豸�������򣬴˺���Ӧ����ɶ�ȡ
	// ָ�����豸��ָ��������
	//
	STATUS (*Read) (
		IN PDEVICE_OBJECT DeviceObject,	// �豸����ָ��
		IN PFILE_OBJECT FileObject,		// �ļ�����ָ��
		OUT PVOID Buffer,				// ָ�򱣴��ȡ����Ļ�����
		IN ULONG Request,				// ���ڴ����ļ����ַ��豸�ļ���Request��Ҫ��ȡ���ֽ��������ڿ��豸��Request��Ҫ��ȡ��������
		OUT PULONG Result OPTIONAL		// ���ڴ����ļ����ַ��豸�ļ���Resultָ�����ڱ���ʵ�ʶ�ȡ�ֽ����ı��������ڿ��豸����
		);

	//
	// Writeָ��д���ܺ����������ļ�ϵͳ�������򣬴˺���Ӧ�����дָ���ļ�����
	// ��Ӧ�Ĵ����ļ������ڴ��ڵ��ַ��豸�������򣬴˺���Ӧ�����дָ���ļ�����
	// ��Ӧ���ַ��豸�����ڴ��̵ȿ��豸�������򣬴˺���Ӧ�����дָ�����豸��
	// ָ��������
	//
	STATUS (*Write) (
		IN PDEVICE_OBJECT DeviceObject, // �豸����ָ��
		IN PFILE_OBJECT FileObject,		// �ļ�����ָ��
		IN PVOID Buffer,				// ָ�����д�����ݵĻ�����
		IN ULONG Request,				// ���ڴ����ļ����ַ��豸�ļ���Request��Ҫд���ֽ��������ڿ��豸��Request��Ҫд��������
		OUT PULONG Result OPTIONAL		// ���ڴ����ļ����ַ��豸�ļ���Resultָ�����ڱ���ʵ��д���ֽ����ı��������ڿ��豸����
		);

	//
	// Queryָ���ѯ���ܺ����������ļ�ϵͳ����������Ч�����ڲ�ѯָ���ļ�����
	// ��Ӧ�Ĵ����ļ���������Ϣ���������д��ʱ�䡢ֻ�����ԡ��ļ���С�ȡ�
	//
	STATUS (*Query) (
		IN PDEVICE_OBJECT DeviceObject,	// �豸����ָ��
		IN PFILE_OBJECT FileObject,		// �ļ�����ָ��
		OUT PFILE_INFO FileInfo			// ָ�룬ָ�����ڱ����ѯ�����FILE_INFO�ṹ�塣
		);

	//
	// Setָ�����ù��ܺ����������ļ�ϵͳ����������Ч����������ָ���ļ������
	// Ӧ�Ĵ����ļ��Ĵ�С���Ƿ�ɾ�����ļ��������ԡ�
	//
	STATUS (*Set) (
		IN PDEVICE_OBJECT DeviceObject,	// �豸����ָ��
		IN PFILE_OBJECT FileObject,		// �ļ�����ָ��
		IN PSET_FILE_INFO FileInfo		// ָ�룬ָ��SET_FILE_INFO�ṹ�壬������Ҫ���õ��ļ�������Ϣ��
		);
} DRIVER_OBJECT;

//
// �豸����ṹ�嶨�塣
// ÿ���豸����Ӧ��һ���豸���󣬲����������豸�����߼��豸��
//
typedef struct _DEVICE_OBJECT {

	//
	// ���豸��־��
	//
	BOOLEAN IsBlockDevice;

	//
	// �豸���,��������ͬ���͵Ĳ�ͬ�豸��
	// �Կ��豸�ļ�ϵͳ���ʱ�������־�����FAT12��FAT32��EXT2�ȣ���ͳһ��š���
	// ������0������1��FAT12�ļ�ϵͳ��ŷֱ���0��1���豸���ֱ���A:��B:�������
	// ��װ��Ӳ�̣���Ӳ�̷������ļ�ϵͳ���˳����ȥ������Ӳ��0�ĵ�һ����������
	// ��ϵͳ���ӦΪ3��������ʲô�ļ�ϵͳ���豸��ΪC:��
	// �����豸��ŵ���������ͬһ���豸֮�ڡ����紮��0��1���豸��ŷֱ���0��1��
	// �豸���ֱ���COM1��COM2������0��1���豸���Ҳ��0��1���豸���ֱ���FLOPPY0��
	// FLOPPY1��
	//
	USHORT DeviceNumber;

	//
	// ������������ָ�롣
	//
	PDRIVER_OBJECT DriverObject;

	//
	// �豸����Ľڵ㡣
	//
	LIST_ENTRY DeviceListEntry;

	ULONG OpenCount;
	BOOLEAN ShareRead;
	BOOLEAN ShareWrite;

	//
	// �豸������չ��ָ�롣
	//
	PVOID DeviceExtension;

	//
	// ���ڻ�������豸��mutex��
	// Ŀǰ���豸�����κη���֮ǰ�����Ȼ�ȡ��Ψһmutex����Ŀǰ��֧��ȫ˫��������
	//
	MUTEX Mutex;
} DEVICE_OBJECT;

//
// �ļ�����ṹ�塣
//
typedef struct _FILE_OBJECT {

	//
	// �ļ��������豸�����ָ�롣
	//
	PDEVICE_OBJECT DeviceObject;

	//
	// �ļ��������Ļ�����ָ�룬����������ʹ�á�
	// ��������ʵ�ֵ�FAT12�ļ�ϵͳ�У�ָ���˶�Ӧ���ļ����ƿ�ṹ�塣
	//
	PVOID FsContext;

	//
	// �ļ������Ȩ�����ԡ�
	//
	BOOLEAN ReadAccess;
	BOOLEAN WriteAccess;

	//
	// �ļ�����Ĺ������ԡ�
	//
	BOOLEAN SharedRead;
	BOOLEAN SharedWrite;

	//
	// ��־λ��
	//
	ULONG FlagsAndAttributes;

	//
	// �ļ���ǰ��дƫ��λ�á�
	//
	ULONG CurrentByteOffset;

	//
	// ���ڻ�������ļ������mutex��
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
// �����ǻ��λ�������ݽṹ�Ķ������ز���������������
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
// �̶߳Կ��豸��д����Ľṹ��
//
typedef struct _REQUEST {
	ULONG Cylinder;			// �߳�Ҫ���ʵĴŵ���
	LIST_ENTRY ListEntry;	// �������������
	EVENT Event;			// ����豸�������߳�ռ�ã���ǰ�߳�Ҫ�����ڸ��¼��ϡ�
}REQUEST, *PREQUEST;

//
// ��ʼ�����豸�㡣
//
VOID
IopInitializeBlockDeviceLayer(
	VOID
	);


#endif // _IOP_
