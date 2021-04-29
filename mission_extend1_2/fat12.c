/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: fat12.c

����: FAT12 �ļ�ϵͳ������ʵ�֡�



*******************************************************************************/

#include "iop.h"
#include "fat12.h"

VOID
FatInitializeDriver(
	PDRIVER_OBJECT DriverObject
	)
{
	DriverObject->AddDevice = FatAddDevice;
	DriverObject->Create = FatCreate;
	DriverObject->Close = FatClose;
	DriverObject->Read = FatRead;
	DriverObject->Write = FatWrite;
	DriverObject->Query = FatQuery;
	DriverObject->Set = FatSet;
}

//
// ��������� AddDevice ���ܺ�����
//
STATUS
FatAddDevice(
	 IN PDRIVER_OBJECT DriverObject,
	 IN PDEVICE_OBJECT NextLayerDevice,
	 IN USHORT DeviceNumber,
	 OUT PDEVICE_OBJECT *DeviceObject
	 )
{
	STATUS Status;
	BOOT_SECTOR BootSector;
	PVCB Vcb;
	PUCHAR Fat;
	PDEVICE_OBJECT FatDevice;
	USHORT i;
	static char DeviceName[] = "A:";

	//
	// FAT12 �ļ�ϵͳ�²������һ�����豸��
	//
	ASSERT(NULL != NextLayerDevice && NextLayerDevice->IsBlockDevice);

	//
	// ��ȡ����������0��������ʼ�� 62 ���ֽڣ����� BPB��
	//
	Status = IopReadWriteSector( NextLayerDevice,
								0,
								0,
								&BootSector,
								sizeof(BOOT_SECTOR),
								TRUE );

	if (!EOS_SUCCESS(Status)) {
		return STATUS_WRONG_VOLUME;
	}

	//
	// ������� FAT12 �ļ�ϵͳ�򷵻�ʧ�ܡ�
	//
	if (0 != strnicmp((PCHAR)BootSector.SystemId, "FAT12   ", 8)) {
		return STATUS_WRONG_VOLUME;
	}

	//
	// Ŀǰֻ֧��������СΪ 512 �ֽڵ�����, FAT12�ļ�ϵͳ�Ĵص����������ܴﵽ 4085,
	// FAT��Ҳ�����ܳ��� 12 ��������
	//
	if (BootSector.Bpb.BytesPerSector != 512
		|| BootSector.Bpb.Media != 0xF0
		|| FatNumberOfClusters(&BootSector.Bpb) >= 4085
		|| BootSector.Bpb.SectorsPerFat > 12 ) {
			return STATUS_WRONG_VOLUME;
	}

	//
	// ���� FAT ���沢�������� FAT��ǰ 3 ���ֽ�ӦΪ�̶�ֵ 0xF0,0xFF,0xFF��
	//
	Fat = MmAllocateSystemPool(FatBytesPerFat(&BootSector.Bpb));

	if (NULL == Fat) {
		return STATUS_NO_MEMORY;
	}

	for (i = 0; i < BootSector.Bpb.SectorsPerFat; i++) {

		Status = IopReadWriteSector( NextLayerDevice,
									BootSector.Bpb.ReservedSectors + i,
									0,
									Fat + i * 512,
									512,
									TRUE );

		if (!EOS_SUCCESS(Status)) {
			MmFreeSystemPool(Fat);
			return STATUS_WRONG_VOLUME;
		}
	}

	if (0xF0 != Fat[0] || 0xFF != Fat[1] || 0xFF != Fat[2]) {
		MmFreeSystemPool(Fat);
		return STATUS_WRONG_VOLUME;
	}

	//
	// ���� FAT12 �ļ�ϵͳ�豸�����豸��չ��Ϊ VCB �ṹ�塣
	//
	DeviceName[0] += DeviceNumber;
	Status = IopCreateDevice( DriverObject,
							sizeof(VCB),
							DeviceName,
							DeviceNumber,
							FALSE,
							&FatDevice );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	//
	// ��ʼ�� VCB��
	//
	Vcb = (PVCB)FatDevice->DeviceExtension;
	Vcb->DiskDevice = NextLayerDevice;
	Vcb->Bpb = BootSector.Bpb;
	Vcb->Fat = Fat;
	Vcb->FirstRootDirSector = FatFirstRootDirSector(&BootSector.Bpb);
	Vcb->RootDirSize = FatRootDirSize(&BootSector.Bpb);
	Vcb->FirstDataSector = FatFirstDataSector(&BootSector.Bpb);
	Vcb->NumberOfClusters = (USHORT)FatNumberOfClusters(&BootSector.Bpb);
	ListInitializeHead(&Vcb->FileListHead);

	return STATUS_SUCCESS;
}

//
// ��������� Create ���ܺ�����
//
STATUS
FatCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN ULONG CreationDisposition,
	IN OUT PFILE_OBJECT FileObject
	)
{
	//
	// ���·���Ƿ���Ч��
	//
	if (!FatCheckPath(FileName, FALSE)) {
		return STATUS_PATH_SYNTAX_BAD;
	}

	//
	// Ŀǰ��ʵ���˴������ļ���
	//
	if (OPEN_EXISTING == CreationDisposition) {

		return FatOpenExistingFile(DeviceObject, FileName, FileObject);

	} else if (CREATE_NEW == CreationDisposition) {

		return STATUS_NOT_SUPPORTED;

	} else if (CREATE_ALWAYS == CreationDisposition) {

		return STATUS_NOT_SUPPORTED;

	} else if (OPEN_ALWAYS == CreationDisposition) {

		return STATUS_NOT_SUPPORTED;

	} else if (TRUNCATE_EXISTING == CreationDisposition) {

		return STATUS_NOT_SUPPORTED;
	}
}

//
// ��������� Close ���ܺ�����
//
VOID
FatClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PFILE_OBJECT FileObject
	)
{
	FatCloseFile((PFCB)FileObject->FsContext);
}

//
// ��������� Read ���ܺ�����
//
STATUS
FatRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result
	)
{
	return FatReadFile( (PVCB)DeviceObject->DeviceExtension,
						(PFCB)FileObject->FsContext,
						FileObject->CurrentByteOffset,
						Request,
						Buffer,
						Result );
}

//
// ��������� Write ���ܺ�����
//
STATUS
FatWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	)
{
	STATUS Status;

	Status = FatWriteFile( (PVCB)DeviceObject->DeviceExtension,
						   (PFCB)FileObject->FsContext,
						   FileObject->CurrentByteOffset,
						   Request,
						   Buffer,
						   Result );

	return Status;
}

//
// ��������� Query ���ܺ�����
//
STATUS
FatQuery(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PFILE_INFO FileInfo
	)
{
	PFCB Fcb;

	Fcb = (PFCB)FileObject->FsContext;

	//
	// Ŀǰ��δʵ��ʱ�������ʱ�����ļ�ʱ�䡣
	//
	FileInfo->FileAttributes = 0;

	if (Fcb->AttrDirectory) {
		FileInfo->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
	} else {
		FileInfo->FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
	}

	if (Fcb->AttrHidden) {
		FileInfo->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
	}

	if (Fcb->AttrSystem) {
		FileInfo->FileAttributes |= FILE_ATTRIBUTE_SYSTEM;
	}

	if (Fcb->AttrReadOnly) {
		FileInfo->FileAttributes |= FILE_ATTRIBUTE_READONLY;
	}

	FileInfo->FileSize = Fcb->FileSize;

	return STATUS_SUCCESS;
}

//
// ��������� Set ���ܺ�����
//
STATUS
FatSet(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PSET_FILE_INFO FileInfo
	)
{
	//
	// Ŀǰ�в�֧���κ��޸Ĳ�����
	//
	return STATUS_NOT_SUPPORTED;
}

USHORT
FatGetFatEntryValue(
	IN PVCB Vcb,
	IN USHORT Index
	)
/*++

����������
	��FAT��ָ�����ֵ������ȡ���ص��ڴ��е�FAT����������

������
	Vcb -- ����ƿ�ָ�롣
	Index -- ָ�����������

����ֵ��
	FAT���ֵ��

--*/
{
	USHORT Value;

	ASSERT(2 <= Index && Index < 0xFF0);

	//
	// ������FAT������������ֽڶ���һ��16λ���α�����
	//
	CopyUchar2(&Value, (PCHAR)Vcb->Fat + Index * 3 / 2)

	//
	// ������������/żȡ���α����ĸ�/��12λ��ֵ��
	//
	return (Index & 0x1) != 0 ? Value >> 4 : Value & 0x0FFF;
}

STATUS
FatSetFatEntryValue(
	IN PVCB Vcb,
	IN USHORT Index,
	IN USHORT Value12
	)
/*++

����������
	дFAT��ָ�����ֵ��дFAT��������ͬʱд��͸�����̡�

������
	Vcb -- ����ƿ�ָ�롣
	Index -- ָ�����������
	Value12 -- ����д���12bitֵ��

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	USHORT OldValue16;
	USHORT NewValue16;
	USHORT SectorNumber;
	USHORT BytesOffset;
	USHORT i;

	ASSERT(2 <= Index && Index < 0xFF0);
	ASSERT(Value12 != 1 && Value12 <= 0xFFF);

	//
	// ������FAT������������ֽڶ���һ��16λ���α���OriginalValue��
	//
	CopyUchar2(&OldValue16, (PCHAR)Vcb->Fat + Index * 3 / 2);

	//
	// ������������ż���ԣ����޸�16λ�еĸ�12λ���12λ��ֵ������4λ���䡣
	//
	NewValue16 = (Index & 0x01) != 0 ? Value12 << 4 | (OldValue16 & 0x000F) : (OldValue16 & 0xF000) | Value12;

	//
	// ���޸ĺ��ֵд��FAT��������
	//
	CopyUchar2((PUCHAR)Vcb->Fat + Index * 3 / 2, &NewValue16);

	//
	// ���޸ĺ��ֵд������е�FAT����Ϊ�����Ͽ����ж��FAT������Ҫѭ��д��
	//
	for (i = 0; i < Vcb->Bpb.Fats; i++) {

		//
		// ����������ڵ��������Լ��������ڵ��ֽ�ƫ�ơ�
		//
		SectorNumber = Vcb->Bpb.ReservedSectors + i * Vcb->Bpb.SectorsPerFat + (Index * 3 / 2) / Vcb->Bpb.BytesPerSector;
		BytesOffset = (Index * 3 / 2) % Vcb->Bpb.BytesPerSector;
		
		//
		// ���޸ĺ��2�ֽ�д����̡�ע�⣺Ҫ���ǵ�2���ֽڿ��ܿ�������
		//
		if (BytesOffset < Vcb->Bpb.BytesPerSector - 1) {

			Status = IopReadWriteSector( Vcb->DiskDevice, 
										 SectorNumber,
										 BytesOffset,
										 &NewValue16,
										 2,
										 FALSE );
			if (!EOS_SUCCESS(Status)) {
				return Status;
			}

		} else {

			//
			// 2���ֽ����ÿ������ˣ�дǰһ���������1�ֽڣ�д��һ�����ĵ�һ��1�ֽڡ�
			//
			Status = IopReadWriteSector( Vcb->DiskDevice,
										 SectorNumber,
										 BytesOffset,
										 &NewValue16,
										 1,
										 FALSE );
			if (!EOS_SUCCESS(Status)) {
				return Status;
			}

			Status = IopReadWriteSector( Vcb->DiskDevice,
										 SectorNumber + 1,
										 0,
										 (char*)(&NewValue16) + 1,
										 1,
										 FALSE );
			if (!EOS_SUCCESS(Status)) {
				return Status;
			}
		}
	}

	return STATUS_SUCCESS;
}

BOOL
FatCheckPath(
	IN PCSTR PathName,
	IN BOOL IsDirectoryName
	)
/*++

����������
	���·���ַ����Ƿ���Ч��·���е������ļ������������8.3���򣬹������£�
	�ļ�����1~8���ַ���ɣ���չ����1~3���ַ���ɣ���չ����ѡ�����������չ������
	�ļ�������չ��֮����'.'�ָ���磺G9401.DBF������G9401���ļ�����DBF����չ����
	�ļ�������չ������ʹ����ĸ(�����ִ�Сд�������ֺ͸��ַ��ţ��ո� < > / \ | : "
	* ?���ַ����⡣

������
	PathName -- ·������
	IsDirectoryName -- �Ƿ�Ŀ¼·����

����ֵ��
	���·����Ч�򷵻�TRUE��

--*/
{
	PCSTR Dot;
	PCSTR Name = PathName;
	PCSTR End = PathName;
	ULONG CountOfFileName = 0;

	for (;;) {

		//
		// �������·���еķָ�������б�ܻ��߷�б�ܣ���ͬʱ����Ƿ�����Ƿ��ַ���
		//
		while (*End != '\0' && *End != '\\' && *End != '/') {

			//
			// 8.3�ļ����в��ܰ��������ַ���
			// �� ֵС�ڵ���0x20���ַ����������⣬�����ļ����ĵ�һ���ַ�������0x05��
			//    �����滻�����ַ�0xE5��
			// �� 0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
			//    0x3F, 0x5B, 0x5C, 0x5D��and 0x7C.
			// Ϊ�˼򵥣������������ļ����׸��ַ���0x05�������
			//
			if (*End <= 0x20 ||
				0x2A <= *End && *End <= 0x2C ||
				0x3A <= *End && *End <= 0x3F || 
				0x5B == *End || 0x5D == *End || 0x7c == *End) {
				return FALSE;
			}

			End++;
		}

		if (End > Name) {

			//
			// ����ļ������ǵ�ǰĿ¼"."���ϼ�Ŀ¼".."�����������8.3����
			//
			if (!(End - Name == 1 && '.' == *Name) &&
				!(End - Name == 2 && '.' == *Name && '.' == *(Name+1))) {

				//
				// 8.3�ļ�����һ���ַ���������'.'���12���ַ��������ļ�����Ч��
				//
				if ('.' == *Name || End - Name > 12) {
					return FALSE;
				}

				//
				// �Ӻ���ǰ�����ļ�������չ��֮��ķָ���'.'��
				//
				for (Dot = End - 1; Dot > Name && *Dot != '.'; Dot--);

				if (Dot > Name) {

					//
					// �ҵ�'.'���ļ������8���ַ�����չ����1~3���ַ���������Ч��
					//
					if (Dot - Name > 8 || End - Dot == 1 || End - Dot > 4) {
						return FALSE;
					}

					//
					// ������ǰ�������ļ����в�������'.'��������Ч��
					//
					for (Dot--; Dot > Name && *Dot != '.'; Dot--);
					
					if (Dot > Name) {
						return FALSE;
					}

				} else {

					//
					// ������'.'��û����չ�����ļ������8���ַ���������Ч��
					//
					if (End - Name > 8) {
						return FALSE;
					}
				}

				CountOfFileName++;	// ͳ��·������Ч���ļ���������
			}
		}

		if ('\0' == *End) {
			
			if (End-- == PathName) {
				return FALSE; // ·���ַ�������Ϊ0����Ч��
			}

			if (!IsDirectoryName) {

				//
				// �ļ�·��������Ҫ��һ���ļ��������һ���ַ�������'\'��'/'��'.'��
				//
				if (0 == CountOfFileName || '\\' == *End || '/' == *End || '.' == *End) {
					return FALSE;
				}
			}

			return TRUE;

		}

		//
		// �������·������һ���ļ�����
		//
		Name = End + 1;
		End = Name;
	}
}

VOID
FatConvertDirNameToFileName(
	IN CHAR DirName[],
	OUT PSTR FileName
	)
/*++

����������
	��Ŀ¼���е�11�ַ��ļ���ת��Ϊ��0��β���ַ������ַ�����ʽ��*.*��

������
	DirName -- Ŀ¼�����ļ����ַ����顣
	FileName -- �洢ת��������ַ���������ָ�롣

����ֵ��
	����ɹ��򷵻�TRUE��

--*/
{
	INT i;
	PCHAR Dot;

	//
	// ���˵��ļ��������������Ŀո�
	//
	for (i = 7; i >= 0 && ' ' == DirName[i]; i--); // �ֺţ�
	i++;

	//
	// �����ļ�����
	//
	strncpy(FileName, &DirName[0], i);

	Dot = FileName + i;

	//
	// ���˵���չ�������������Ŀո�
	//
	for (i = 10; i > 7 && ' ' == DirName[i]; i--);

	//
	// ���������չ���������ļ����������'.'����չ����
	//
	if (i > 7) {
		*Dot = '.';
		strncpy(Dot + 1, &DirName[8], i - 7);
	}
}

VOID
FatConvertFileNameToDirName(
	IN PSTR FileName,
	OUT CHAR DirName[]
	)
{
	PCHAR p = FileName;
	INT i = 0;

	//
	// �ȸ����ļ������������8�ַ����ÿո��롣
	//
	for (i = 0, p = FileName; i < 8 && *p != '.' && *p != 0; i++, p++) {
		DirName[i] = *p;
	}

	for (; i < 8; i++) {
		DirName[i] = ' ';
	}

	//
	// ������չ�����������3�ַ����ÿո��롣
	//
	if ('.' == *p) {
		p++;
	}

	for (; i < 11 && *p != 0; i++, p++) {
		DirName[i] = *p;
	}

	for(; i < 11; i++) {
		DirName[i] = ' ';
	}
}

STATUS
FatAllocateOneCluster(
	IN PVCB Vcb,
	OUT PUSHORT ClusterNumber
	)
/*++

����������
	����һ�����дء��·���Ĵ��� FAT ���ж�Ӧ�ı�����Զ�����Ϊ 0xFF8��
	���ԣ��·���Ĵر�����Ϊ�ļ������е����һ���ء�

������
	Vcb -- ���̾���ƿ�ָ�롣
	ClusterNumber -- ���سɹ�����Ĵغš�

����ֵ��
	����ɹ��򷵻� STATUS_SUCCESS��

--*/
{
	STATUS Status;
	USHORT n;

	// ���� FAT �����ҵ�һ��ֵΪ 0 �� FAT ��������Ӧ�Ĵ���δ�õġ�
	// ע��غŴ� 2 ��ʼ��
	for (n = 2; n < Vcb->NumberOfClusters + 2; n++) {
		if (0 == FatGetFatEntryValue(Vcb, n))
			break;
	}

	// ���δ�ҵ����õĴ��򷵻ش����롣
	if (n == Vcb->NumberOfClusters + 2)
		return STATUS_NO_SPACE;

	// �޸� FAT �����ֵΪ 0xFF8����Ǵ˴ر��ļ�ռ�ã������ļ������һ���ء�
	Status = FatSetFatEntryValue(Vcb, n, 0xFF8);
	
	if (EOS_SUCCESS(Status))
		*ClusterNumber = n;

	return Status;
}

STATUS
FatReadFile(
	IN PVCB Vcb,
	IN PFCB File,
	IN ULONG Offset,
	IN ULONG BytesToRead,
	OUT PVOID Buffer,
	OUT PULONG BytesRead
	)
/*++

����������
	���ļ���ָ��ƫ�ƴ���ȡָ���ֽڵ����ݣ�ʵ�ʶ�ȡ���ֽ��������ܵ��ļ����ȵ�����
	��С������ֵ��

������
	Vcb -- ����ƿ�ָ�롣
	File -- �ļ����ƿ�ָ�롣
	Offset -- ��ȡ���ļ�����ʼƫ�Ƶ�ַ��
	BytesToRead -- ������ȡ���ֽ�����
	Buffer -- ���ڴ�Ŷ�ȡ���ݵĻ�������ָ�롣
	BytesRead -- ʵ�ʶ�ȡ���ֽ�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	ULONG i;
	ULONG ReadCount = 0;
	USHORT Cluster;
	ULONG FirstSectorOfCluster;
	ULONG OffsetInSector;
	ULONG BytesToReadInSector;

	//
	// �����ȡ�ļ�����ʼƫ��λ�ó����ļ���С����ֱ�ӷ��ء�
	//
	if (Offset >= File->FileSize) {
		*BytesRead = 0;
		return STATUS_SUCCESS;
	}

	//
	// ʵ�ʿ��Զ�ȡ���ֽ������ļ����ȵ����ơ�
	//
	if (BytesToRead > File->FileSize - Offset) {
		BytesToRead = File->FileSize - Offset;
	}

	//
	// ˳���ļ������������ң��ҵ�ƫ��λ�����ڵĴء�
	//
	Cluster = File->FirstCluster;
	for (i = Offset / FatBytesPerCluster(&Vcb->Bpb); i > 0; i--) {
		Cluster = FatGetFatEntryValue(Vcb, Cluster);
	}

	//
	// ��ƫ��λ�����ڵĴؿ�ʼ����ȡ�ļ��Ĵأ�ֱ����ȡ��ɡ�
	//
	for (;;) {

		//
		// ���ɶ������������ɣ�����ص���ʼ�����š�
		//
		FirstSectorOfCluster = Vcb->FirstDataSector + (Cluster - 2) * Vcb->Bpb.SectorsPerCluster;

		//
		// ����ƫ��λ���ڴ��ڵĵڼ���������Ȼ������������ʼ��ȡ���ڵ�����������
		//
		for (i = ((Offset + ReadCount) / Vcb->Bpb.BytesPerSector) % Vcb->Bpb.SectorsPerCluster;
			i < Vcb->Bpb.SectorsPerCluster; i++ ) {

			//
			// �����ȡλ���������ڵ��ֽ�ƫ�ơ�
			//
			OffsetInSector = (Offset + ReadCount) % Vcb->Bpb.BytesPerSector;

			//
			// ������Ҫ����������ڶ�ȡ���ֽ�����
			//
			if (BytesToRead - ReadCount > Vcb->Bpb.BytesPerSector - OffsetInSector) {
				BytesToReadInSector = Vcb->Bpb.BytesPerSector - OffsetInSector;
			} else {
				BytesToReadInSector = BytesToRead - ReadCount;
			}

			//
			// ��ȡ�������ݡ�
			//
			Status = IopReadWriteSector( Vcb->DiskDevice,
										 FirstSectorOfCluster + i,
										 OffsetInSector,
										 (PCHAR)Buffer + ReadCount,
										 BytesToReadInSector,
										 TRUE );

			if (!EOS_SUCCESS(Status)) {
				return Status;
			}

			//
			// �����ȡ����򷵻ء�
			//
			ReadCount += BytesToReadInSector;
			if (ReadCount == BytesToRead) {
				*BytesRead = ReadCount;
				return STATUS_SUCCESS;
			}
		}

		//
		// �������ļ�����һ���ء�
		//
		Cluster = FatGetFatEntryValue(Vcb, Cluster);
	}
}

STATUS
FatWriteFile(
	IN PVCB Vcb,
	IN PFCB File,
	IN ULONG Offset,
	IN ULONG BytesToWrite,
	IN PVOID Buffer,
	OUT PULONG BytesWriten
	)
/*++

����������
	���ļ�ָ����ƫ��λ�ÿ�ʼд���ݣ����ƫ��λ��С���ļ���С�򸲸�ԭ�����ݣ����
	д��Χ�����ļ���С���Զ������ļ���С������ļ���С���Ӻ󳬹��ļ�ռ�õĴ��̿�
	���С���Զ�Ϊ�ļ������µĴأ������ļ�ռ�õĴ��̿ռ䡣
	����ע����������ļ���С���ļ�ռ�ô��̿ռ䡣��Ϊ�ļ�ռ�ô��̿ռ����Դ�Ϊ��
	λ�ģ����ļ���С�ĵ�λ���ֽڣ������ļ���С <= �ļ�ռ�ô��̿ռ䣬���Ե��ļ���
	������ʱ������һ��Ҫ���Ӵ���ռ�ÿռ䡣����һ���ļ���ǰֻ��1�ֽڣ���ô��ռ����
	һ���صĴ��̿ռ䡣���ļ���С���ӵ�10�ֽ�ʱ����ռ�õĴ��̿ռ���ȻΪһ���ء���
	���Ĵ�С���ӵ�����һ���صĴ�Сʱ����ô����ҪΪ�����Ӵ��̿ռ��ˡ�

������
	Vcb -- ����ƿ�ָ�롣
	File -- �ļ����ƿ�ָ�롣
	Offset -- ��ʼд��ƫ��λ�á�
	BytesToWrite -- д���ֽ�����
	Buffer -- ָ����Ҫд�����ݡ�
	BytesWriten -- ָ�룬ָ�����ڱ���ʵ�����д���ֽ����ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	return STATUS_NOT_SUPPORTED;
}

STATUS
FatOpenFileInDirectory(
	IN PVCB Vcb,
	IN PFCB Directory,
	IN PSTR FileName,
	OUT PFCB *OpenedFile
	)
/*++

����������
	��ָ����Ŀ¼�д�ָ�����Ƶ������ļ�����Ŀ¼�ļ���Ŀ¼��ÿ��һ���ļ���Ŀ¼
	�ļ����ƿ�Ĵ򿪼�����������1��

������
	Vcb -- ����ƿ�ָ�롣
	Directory -- �����ļ�������Ŀ¼��ָ�룬���ΪNULL���ڸ�Ŀ¼�д򿪡�
	FileName -- �ļ����ַ���ָ�롣
	OpenedFile -- ָ����������ļ�ָ��Ļ�������

����ֵ��
	����򿪳ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	DIRENT DirEntry;
	CHAR DirName[11];
	PLIST_ENTRY FileListHead;
	PLIST_ENTRY FileListEntry;
	ULONG DirectoryFileSize;
	ULONG DirEntryOffset;
	ULONG BytesRead;
	USHORT Cluster;
	USHORT NumberOfClusters;
	PFCB File;

	//
	// �����Ǹ�Ŀ¼����Ч��Ŀ¼�ļ���
	//
	ASSERT(NULL == Directory || Directory->AttrDirectory && Directory->Name[0] != (CHAR)0xE5);

	//
	// �ȼ���ļ����Ƿ��������ļ�����"."��".."��
	// "."��ʾ��ǰĿ¼��".."��ʾ��ǰĿ¼���ϼ�Ŀ¼��
	//
	if (0 == strcmp(FileName, ".")) {

		//
		// �򿪵�ǰĿ¼��
		//
		if (NULL != Directory) {
			Directory->OpenCount++;
		}
		*OpenedFile = Directory;
		
		return STATUS_SUCCESS;

	} else if (0 == strcmp(FileName, "..")) {

		//
		// �����ǰĿ¼�Ǹ�Ŀ¼����Ȼ���ظ�Ŀ¼(NULL)������򿪵�ǰĿ¼���ϼ�Ŀ¼��
		//
		if (NULL == Directory) {
			*OpenedFile = NULL;
		} else {
			if (NULL != Directory->ParentDirectory) {
				Directory->ParentDirectory->OpenCount++;
			}
			*OpenedFile = Directory->ParentDirectory;
		}

		return STATUS_SUCCESS;
	}

	//
	// ʹFileListHeadָ��Ŀ¼���Ѵ��ļ�������Ȼ�����������ѯҪ�򿪵��ļ�
	// �Ƿ��Ѿ��ڱ��򿪣�����Ѿ��������������򿪼��������ɡ�
	//
	if (NULL == Directory) {

		FileListHead = &Vcb->FileListHead;

	} else {

		//
		// �������Ѿ�����ռ�򿪵�Ŀ¼�д��ļ���
		//
		if (!Directory->SharedRead || !Directory->SharedWrite) {
			return STATUS_SHARING_VIOLATION;
		}

		FileListHead = &Directory->FileListHead;
	}

	for (FileListEntry = FileListHead->Next; 
		FileListEntry != FileListHead;
		FileListEntry = FileListEntry->Next) {

		File = CONTAINING_RECORD(FileListEntry, FCB, FileListEntry);
		
		//
		// �ļ����ĵ�һ���ַ�Ϊ0xE5�����ļ��Ѿ���ɾ����
		//
		if (File->Name[0] != (CHAR)0xE5 && 0 == strcmp(File->Name, FileName)) {

			File->OpenCount++;
			*OpenedFile = File;

			return STATUS_SUCCESS;
		}
	}

	//
	// ��������Ŀ¼�е�DIRENT�ṹ�壬��Ҫ�����̡�
	//
	if (NULL == Directory) {
		DirectoryFileSize = Vcb->RootDirSize;
	} else {
		DirectoryFileSize = Directory->FileSize;
	}

	FatConvertFileNameToDirName(FileName, DirName);

	for (DirEntryOffset = 0; DirEntryOffset < DirectoryFileSize; DirEntryOffset += sizeof(DIRENT)) {

		if (NULL == Directory) {

			//
			// �ڸ�Ŀ¼��Offsetƫ�ƴ���ȡһ��Ŀ¼�
			//
			Status = IopReadWriteSector( Vcb->DiskDevice,
										 Vcb->FirstRootDirSector + DirEntryOffset / Vcb->Bpb.BytesPerSector,
										 DirEntryOffset % Vcb->Bpb.BytesPerSector,
										 &DirEntry,
										 sizeof(DIRENT),
										 TRUE );
		} else {

			//
			// ��Ŀ¼�ļ���Offsetƫ�ƴ���ȡһ��Ŀ¼�
			//
			Status = FatReadFile( Vcb,
								  Directory,
								  DirEntryOffset,
								  sizeof(DIRENT),
								  &DirEntry,
								  &BytesRead );
		}

		//
		// �����ȡʧ�������̷��ء�
		//
		if (!EOS_SUCCESS(Status)) {
			return Status;
		}

		//
		// ���������Ŀ¼��(��һ���ַ�Ϊ0��Ŀ¼���־Ŀ¼�ļ�����)�������������
		//
		if (0 == DirEntry.Name[0]) {
			return STATUS_FILE_NOT_FOUND;
		}

		//
		// ��Ŀ¼����Ч(��һ���ַ�Ϊ0xE5)���ļ�����һ�£���������������
		//
		if ((CHAR)0xE5 == DirEntry.Name[0] ||
			0 != strnicmp(DirEntry.Name, DirName, 11)) {
			continue;
		}

		//
		// ����ļ��Ĵ�����ͳ�ƴ�����ĳ��ȡ�
		//
		NumberOfClusters = 0;

		if (DirEntry.FirstCluster != 0) {

			for (Cluster = DirEntry.FirstCluster;
				Cluster < 0xFF8;
				Cluster  = FatGetFatEntryValue(Vcb, Cluster)) {

				//
				// ����������г�����Ч�Ĵغţ�С��2��������غţ����򷵻ش���
				//
				if (Cluster < 2 || Cluster > Vcb->NumberOfClusters + 1) {
					return STATUS_FILE_CORRUPT_ERROR;
				}

				NumberOfClusters++; // ͳ���ļ�ռ�ôص�������
			}
		}

		//
		// �����ļ���Ŀ¼���¼�е��ļ���С�������ռ�ôص�����һ�£�Ŀ¼�ļ���
		// Ŀ¼���м�¼���ļ���С������0������ռ��һ���ء�
		//
		if ((DirEntry.Attributes & DIRENT_ATTR_ARCHIVE) != 0) {
			if ((DirEntry.FileSize + FatBytesPerCluster(&Vcb->Bpb) - 1) / FatBytesPerCluster(&Vcb->Bpb) != NumberOfClusters) {
				return STATUS_FILE_CORRUPT_ERROR;
			}
		} else if (DirEntry.FileSize != 0 || 0 == NumberOfClusters) {
			return STATUS_FILE_CORRUPT_ERROR;
		}
		

		//
		// ��ϵͳ�ڴ���з���һ���ļ����ƿ顣
		//
		File = (PFCB)MmAllocateSystemPool(sizeof(FCB));

		if (NULL == File) {
			return STATUS_NO_MEMORY;
		}

		//
		// ��ʼ��FCB����֮��������Ŀ¼���ļ������С�
		//
		FatConvertDirNameToFileName(DirEntry.Name, File->Name);
		File->AttrReadOnly = (DirEntry.Attributes & DIRENT_ATTR_READ_ONLY) != 0;
		File->AttrHidden = (DirEntry.Attributes & DIRENT_ATTR_HIDDEN) != 0;
		File->AttrSystem = (DirEntry.Attributes & DIRENT_ATTR_SYSTEM) != 0;
		File->AttrDirectory = (DirEntry.Attributes & DIRENT_ATTR_DIRECTORY) != 0;
		File->SharedRead = TRUE;
		File->SharedWrite = TRUE;
		File->LastWriteTime = DirEntry.LastWriteTime;
		File->LastWriteDate = DirEntry.LastWriteDate;
		File->FirstCluster = DirEntry.FirstCluster;
		File->DirEntryOffset = DirEntryOffset;
		File->OpenCount = 1;
		File->ParentDirectory = Directory;
		ListInsertTail(FileListHead, &File->FileListEntry);
		ListInitializeHead(&File->FileListHead);

		//
		// �����ļ�����ֱ�ӻ���ļ��Ĵ�С������Ŀ¼�ļ���Ҫ���ݴص����������㣬
		// ��ΪĿ¼�ļ���Ŀ¼���е�FileSize��ԶΪ0��
		//
		if (!File->AttrDirectory) {
			File->FileSize = DirEntry.FileSize;
		} else {
			ASSERT(0 == DirEntry.FileSize);
			File->FileSize = NumberOfClusters * FatBytesPerCluster(&Vcb->Bpb);
		}

		// 
		// ����Ŀ¼�ļ��Ĵ򿪼�������
		//
		if (NULL != Directory) {
			Directory->OpenCount++;
		}

		*OpenedFile = File;
		return STATUS_SUCCESS;
	}
}

STATUS
FatOpenFile(
	IN PVCB Vcb,
	IN PCSTR FullPathname,
	OUT PFCB *OpenedFile
	)
/*++

����������
	�򿪸���·���������ļ���Ŀ¼�ļ���

������
	Vcb -- ����ƿ�ָ�롣
	FullPathname -- �ļ��ڴ����ڵ�ȫ·������
	OpenedFile -- ָ����������ļ�ָ��Ļ�������

����ֵ��
	����򿪳ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PCSTR PathnamePtr;
	CHAR FileName[13];	// �ļ����12���ַ���8���ļ��� + 1��'.' + 3����չ������
	PSTR FileNamePtr;
	PFCB ParentFcb;
	PFCB ChildFcb;

	ParentFcb = NULL;
	ChildFcb = NULL;
	PathnamePtr = FullPathname;

	for (;;) {

		//
		// �������·���еķָ�������б�ܻ��߷�б�ܣ����������ָ���֮����ļ���������
		// �ļ���������FileName[13]�С�
		//
		FileNamePtr = FileName;
		while (*PathnamePtr != '\0' && *PathnamePtr != '\\' && *PathnamePtr != '/') {
			ASSERT(FileNamePtr - FileName < 12);
			*FileNamePtr++ = *PathnamePtr++;
		}

		if (FileNamePtr != FileName) {

			*FileNamePtr = '\0';

			Status = FatOpenFileInDirectory( Vcb,
											 ParentFcb,
											 FileName,
											 &ChildFcb );

			//
			// Ŀ¼�ļ�ָ�벻��ʹ���ˣ��ر�����
			//
			if (NULL != ParentFcb) {
				FatCloseFile(ParentFcb);
			}

			if (!EOS_SUCCESS(Status)) {

				//
				// δ�ҵ�Ŀ¼�ļ�ʱ������STATUS_PATH_NOT_FOUND��
				//
				if (STATUS_FILE_NOT_FOUND == Status && *PathnamePtr != '\0') {
					return STATUS_PATH_NOT_FOUND;
				}

				return Status;
			}

			//
			// δ�ҵ�Ŀ¼�ļ�ʱ������STATUS_PATH_NOT_FOUND��
			//
			if (*PathnamePtr != '\0' && NULL != ChildFcb && !ChildFcb->AttrDirectory) {
				FatCloseFile(ChildFcb);
				return STATUS_PATH_NOT_FOUND;
			}
		}

		//
		// ���·������������ѭ����
		//
		if ('\0' == *PathnamePtr) {
			break;
		}

		//
		// �����ָ�������������Ŀ¼�ļ���
		//
		PathnamePtr++;
		ParentFcb = ChildFcb;
	}

	//
	// ���δ���κ��ļ���˵��·��Ϊ�մ����߲�������Ч���ļ�����ȫ���Ƿָ�������
	//
	if (NULL == ChildFcb) {	
		return STATUS_PATH_SYNTAX_BAD;
	}

	*OpenedFile = ChildFcb;
	return STATUS_SUCCESS;
}

STATUS
FatOpenExistingFile(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN OUT PFILE_OBJECT FileObject
	)
{
	STATUS Status;
	PFCB Fcb;

	//
	// ���ļ���
	//
	Status = FatOpenFile( (PVCB)DeviceObject->DeviceExtension,
						  FileName,
						  &Fcb );

	if (!EOS_SUCCESS(Status)) {

		if (STATUS_FILE_NOT_FOUND == Status && FileObject->FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			return STATUS_PATH_NOT_FOUND;
		}

		return Status;
	}

	//
	// ���򿪵��ļ������Ƿ����Ҫ��
	//
	if (FileObject->FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) {

		if (!Fcb->AttrDirectory) {
			FatCloseFile(Fcb);
			return STATUS_PATH_NOT_FOUND; // Ŀ¼�ļ������ڡ�
		}

	} else {

		if (Fcb->AttrDirectory) {
			FatCloseFile(Fcb);
			return STATUS_ACCESS_DENIED; // �ܾ������ݷ�ʽ��Ŀ¼�ļ���
		}
	}

	if (Fcb->AttrReadOnly && FileObject->WriteAccess) {
		FatCloseFile(Fcb);
		return STATUS_ACCESS_DENIED; // �ܾ���ֻ���ļ���д����
	}

	//
	// ����ļ��ǵ�һ�α��������ù���Ȩ�ޣ��������Ƿ����ͻ��
	//
	if (1 == Fcb->OpenCount) {

		Fcb->SharedRead = FileObject->SharedRead;
		Fcb->SharedWrite = FileObject->SharedWrite;

	} else {

		if (FileObject->ReadAccess && !Fcb->SharedRead ||
			FileObject->WriteAccess && !Fcb->SharedWrite ||
			FileObject->SharedRead != Fcb->SharedRead ||
			FileObject->SharedWrite != Fcb->SharedWrite) {
			FatCloseFile(Fcb);
			return STATUS_SHARING_VIOLATION;
		}
	}

	FileObject->FsContext = Fcb;
	return STATUS_SUCCESS;
}

STATUS
FatWriteDirEntry(
	IN PVCB Vcb,
	IN PFCB Fcb
	)
/*++

����������
	д�ļ���Ӧ��DIRENT�ṹ�嵽���̣��ļ������ԡ����ȡ�ʱ����ȱ��޸ĺ���Ҫִ�д˺�����

������
	Vcb -- ���̾���ƿ�ָ�롣
	Fcb -- �ļ����ƿ�ָ�롣

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	DIRENT DirEntry = {0};
	ULONG BytesWriten;

	//
	// ��FCB�ṹ���ʼ��һ��DIRENT�ṹ�塣
	//
	FatConvertFileNameToDirName(Fcb->Name, DirEntry.Name);

	if (Fcb->AttrReadOnly) {
		DirEntry.Attributes |= DIRENT_ATTR_READ_ONLY;
	}
	if (Fcb->AttrHidden) {
		DirEntry.Attributes |= DIRENT_ATTR_HIDDEN;
	}
	if (Fcb->AttrSystem) {
		DirEntry.Attributes |= DIRENT_ATTR_SYSTEM;
	}
	if (Fcb->AttrDirectory) {
		DirEntry.Attributes |= DIRENT_ATTR_DIRECTORY;
	} else {
		DirEntry.Attributes |= DIRENT_ATTR_ARCHIVE;
	}

	DirEntry.LastWriteTime = Fcb->LastWriteTime;
	DirEntry.LastWriteDate = Fcb->LastWriteDate;

	DirEntry.FirstCluster = Fcb->FirstCluster;
	DirEntry.FileSize = Fcb->FileSize;

	//
	// ����ļ�λ�ڸ�Ŀ¼��д��Ŀ¼������д����Ŀ¼�ļ���
	//
	if (Fcb->ParentDirectory != NULL) {
		Status = FatWriteFile( Vcb,
							   Fcb->ParentDirectory,
							   Fcb->DirEntryOffset,
							   sizeof(DIRENT),
							   &DirEntry,
							   &BytesWriten );
	} else {
		Status = IopReadWriteSector( Vcb->DiskDevice,
									 Vcb->FirstRootDirSector + Fcb->DirEntryOffset / Vcb->Bpb.BytesPerSector,
									 Fcb->DirEntryOffset % Vcb->Bpb.BytesPerSector,
									 &DirEntry,
									 sizeof(DIRENT),
									 FALSE );
	}

	return Status;
}

VOID
FatCloseFile(
	IN PFCB Fcb
	)
/*++

����������
	�ر��ļ����ƿ顣��С�ļ����ƿ�Ĵ򿪼������������������Ϊ0��ر��ļ����ƿ�
	����С�ļ�����Ŀ¼���ļ����ƿ�Ĵ򿪼��������Դ˵ݹ顣

������
	File -- �ļ����ƿ�ָ�롣

����ֵ��
	�ޡ�

--*/
{
	PFCB Current;
	PFCB Parent;

	ASSERT(NULL != Fcb && Fcb->OpenCount > 0);

	for (Current = Fcb; Current != NULL; Current = Parent) {

		//
		// �����ǰ�ļ��Ĵ򿪼�������1���Դ���0�򷵻ء�
		//
		if (--Current->OpenCount > 0) {
			break;
		}

		//
		// ��¼��ǰ�ļ�������Ŀ¼���رյ�ǰ�ļ���Ҫ�ݹ�رյ�ǰ�ļ�����Ŀ¼��
		//
		Parent = Current->ParentDirectory;

		//
		// ����ǰ�ļ�������Ŀ¼���ļ��������Ƴ���
		//
		ListRemoveEntry(&Current->FileListEntry);

		//
		// �ͷ�FCBռ�õ��ڴ档
		//
		MmFreeSystemPool(Current);
	}
}
