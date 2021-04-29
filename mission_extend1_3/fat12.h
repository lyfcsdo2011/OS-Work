/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: fat12.h

����: FAT12�ļ�ϵͳ����������ڲ�ͷ�ļ���



*******************************************************************************/

#ifndef _FAT12_
#define _FAT12_

#include "eosdef.h"


#pragma pack(1)		// ֪ͨ���������յ����ֽڷ�ʽ�������нṹ���еĳ�Ա

//
// BIOS Parameter Block (BPB) �ṹ�塣��СΪ 25 ���ֽڡ�
//
typedef struct _BIOS_PARAMETER_BLOCK {
	USHORT BytesPerSector;					// ÿ�����ֽ���
	UCHAR  SectorsPerCluster;				// ÿ��������
	USHORT ReservedSectors;					// ����������
	UCHAR  Fats;							// FAT�������
	USHORT RootEntries;						// ��Ŀ¼����
	USHORT Sectors;							// ��������
	UCHAR  Media;							// ��������
	USHORT SectorsPerFat;					// ÿFAT��ռ�õ�������
	USHORT SectorsPerTrack;					// ÿ�ŵ�������
	USHORT Heads;							// ��ͷ��
	ULONG  HiddenSectors;					// ��������ǰ��������
	ULONG  LargeSectors;					// ��������(Sectors Ϊ 0 ʱʹ��)
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

//
// ���������ṹ�塣ֻ��������������ʼ�� 62 ���ֽڣ����������µ� 450 ���ֽڡ�
//
typedef struct _BOOT_SECTOR {
	UCHAR Jump[3];							// ��תָ�			ƫ�� 00��
	UCHAR Oem[8];							// OEM��				ƫ�� 03��
	BIOS_PARAMETER_BLOCK Bpb;				// BPB��				ƫ�� 11��
	UCHAR DriveNumber;						// ������������š�		ƫ�� 36��
	UCHAR Reserved;							// ������				ƫ�� 37��
	UCHAR Signature;						// ��չ����������־��	ƫ�� 38��
	UCHAR Id[4];							// �����кš�			ƫ�� 39��
	UCHAR VolumeLabel[11];					// ��ꡣ				ƫ�� 43��
	UCHAR SystemId[8];						// �ļ�ϵͳ���͡�		ƫ�� 54��
} BOOT_SECTOR, *PBOOT_SECTOR;

#pragma pack()		// �ָ�Ĭ�϶��뷽ʽ

//
// �����ַ��ĺ�
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
//  Fat �ļ�ϵͳʹ�õ�ʱ��\���ڽṹ�塣Note that the
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
	USHORT Year          : 7; // �� 1980 ����ʼ��
} FAT_DATE, *PFAT_DATE;

//
// Ŀ¼��ṹ�� (Directory Entry)����СΪ 32 ���ֽڡ�
//
typedef struct _DIRENT {
	CHAR Name[11];							// �ļ��� 8 �ֽڣ���չ�� 3 �ֽ�
	UCHAR Attributes;						// �ļ�����
	UCHAR Reserved[10];						// ����δ��
	FAT_TIME LastWriteTime;					// �ļ�����޸�ʱ��
	FAT_DATE LastWriteDate;					// �ļ�����޸�����
	USHORT FirstCluster;					// �ļ��ĵ�һ���غ�
	ULONG FileSize;							// �ļ���С
} DIRENT, *PDIRENT;

//
// Fat12 �ļ�ϵͳ�豸�������չ�ṹ�塪������ƿ飨Volume Control Block)��
//
typedef struct _VCB {

	//
	// �ļ�ϵͳ�²�����̻���Ӳ�̾��豸����(Ŀǰ����������)��
	//
	PDEVICE_OBJECT DiskDevice;

	//
	// �ļ�ϵͳ�Ĳ�����
	//
	BIOS_PARAMETER_BLOCK Bpb;

	//
	// �ļ������File Allocation Table����������FAT12 �� FAT ����󲻹� 6KB������
	// ��ȫ���ص��ڴ��бȽϺ��ʡ�
	//
	PVOID Fat;

	//
	// ��Ŀ¼��ʼ��������Ŀ¼��С�Լ���Ŀ¼�ļ�����ͷ��
	//
	ULONG FirstRootDirSector;
	ULONG RootDirSize;
	LIST_ENTRY FileListHead;

	//
	// �ļ�����������ʼ�����Լ��ص�������
	//
	ULONG FirstDataSector;
	USHORT NumberOfClusters;
}VCB, *PVCB;

//
// �ļ����ƿ飨File Control Block����
//
typedef struct _FCB {

	//
	// �ļ����ַ�����
	//
	CHAR Name[13];

	//
	// �ļ����ԡ�
	//
	BOOLEAN AttrReadOnly;
	BOOLEAN AttrHidden;
	BOOLEAN AttrSystem;
	BOOLEAN AttrDirectory;

	//
	// �������ԡ�
	//
	BOOLEAN SharedRead;
	BOOLEAN SharedWrite;

	//
	// �ļ�����޸�ʱ�䡣
	//
	FAT_TIME LastWriteTime;
	FAT_DATE LastWriteDate;

	//
	// �ļ��ĵ�һ���غź��ļ��Ĵ�С��
	//
	USHORT FirstCluster;
	ULONG FileSize;

	//
	// Ŀ¼��ṹ����Ŀ¼�ļ��ڵ�ƫ�ơ�
	//
	ULONG DirEntryOffset;

	//
	// �ļ��򿪼��������鵵�ļ�ÿ����һ�Σ�OpenCount ���� 1��Ŀ¼��ÿ��һ����
	// ����Ŀ¼�ļ��� OpenCount ���� 1���ر��ļ�ʱ��OpenCount ��С 1�������Ϊ 0 
	// ���ͷ� FCB �ڵ㣬ͬʱ�ļ�����Ŀ¼�� OpenCount �ݹ��С 1��
	//
	ULONG OpenCount;

	//
	// �ļ�����Ŀ¼��ָ�룬���Ϊ NULL ˵���ļ�λ�ڸ�Ŀ¼��
	//
	struct _FCB *ParentDirectory;

	//
	// �ļ�����Ŀ¼���Ѵ��ļ������
	//
	LIST_ENTRY FileListEntry;

	//
	// Ŀ¼���Ѵ��ļ�����ͷ���� AttrDirectory Ϊ TRUE ʱ��Ч��
	//
	LIST_ENTRY FileListHead;
}FCB, *PFCB;

//
// �ļ����Ե�λ����
//
#define DIRENT_ATTR_READ_ONLY		0x01
#define DIRENT_ATTR_HIDDEN			0x02
#define DIRENT_ATTR_SYSTEM			0x04
#define DIRENT_ATTR_DIRECTORY		0x10
#define DIRENT_ATTR_ARCHIVE			0x20

//
// ����صĴ�С
//
#define FatBytesPerCluster(B) ((ULONG)((B)->BytesPerSector * (B)->SectorsPerCluster))

//
// ���� FAT ��Ĵ�С
//
#define FatBytesPerFat(B) ((ULONG)((B)->BytesPerSector * (B)->SectorsPerFat))

//
// �����Ŀ¼����ʼ����
//
#define FatFirstRootDirSector(B) ((B)->ReservedSectors + ((B)->Fats * (B)->SectorsPerFat))

//
// �����Ŀ¼�Ĵ�С
//
#define FatRootDirSize(B) ((ULONG)((B)->RootEntries * sizeof(DIRENT)))

//
// �����Ŀ¼����������
//
#define FatRootDirSectors(B) ((ULONG)((FatRootDirSize(B) + ((B)->BytesPerSector - 1)) / (B)->BytesPerSector))

//
// ��������������ʼ����
//
#define FatFirstDataSector(B) (FatFirstRootDirSector(B) + FatRootDirSectors(B))

//
// ����ص�����
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
// �ж��ļ������ļ���·�������Ƿ���Ч��
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
