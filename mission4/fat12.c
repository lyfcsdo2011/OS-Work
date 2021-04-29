/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: fat12.c

描述: FAT12 文件系统驱动的实现。



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
// 驱动程序的 AddDevice 功能函数。
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
	// FAT12 文件系统下层必须是一个块设备。
	//
	ASSERT(NULL != NextLayerDevice && NextLayerDevice->IsBlockDevice);

	//
	// 读取引导扇区（0扇区）开始的 62 个字节，包括 BPB。
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
	// 如果不是 FAT12 文件系统则返回失败。
	//
	if (0 != strnicmp((PCHAR)BootSector.SystemId, "FAT12   ", 8)) {
		return STATUS_WRONG_VOLUME;
	}

	//
	// 目前只支持扇区大小为 512 字节的软盘, FAT12文件系统的簇的数量不可能达到 4085,
	// FAT表也不可能超过 12 个扇区。
	//
	if (BootSector.Bpb.BytesPerSector != 512
		|| BootSector.Bpb.Media != 0xF0
		|| FatNumberOfClusters(&BootSector.Bpb) >= 4085
		|| BootSector.Bpb.SectorsPerFat > 12 ) {
			return STATUS_WRONG_VOLUME;
	}

	//
	// 分配 FAT 缓存并读入整个 FAT，前 3 个字节应为固定值 0xF0,0xFF,0xFF。
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
	// 创建 FAT12 文件系统设备对象，设备扩展块为 VCB 结构体。
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
	// 初始化 VCB。
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
// 驱动程序的 Create 功能函数。
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
	// 检查路径是否有效。
	//
	if (!FatCheckPath(FileName, FALSE)) {
		return STATUS_PATH_SYNTAX_BAD;
	}

	//
	// 目前仅实现了打开已有文件。
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
// 驱动程序的 Close 功能函数。
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
// 驱动程序的 Read 功能函数。
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
// 驱动程序的 Write 功能函数。
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
// 驱动程序的 Query 功能函数。
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
	// 目前尚未实现时间管理，暂时忽略文件时间。
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
// 驱动程序的 Set 功能函数。
//
STATUS
FatSet(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	IN PSET_FILE_INFO FileInfo
	)
{
	//
	// 目前尚不支持任何修改操作。
	//
	return STATUS_NOT_SUPPORTED;
}

USHORT
FatGetFatEntryValue(
	IN PVCB Vcb,
	IN USHORT Index
	)
/*++

功能描述：
	读FAT中指定项的值（仅读取加载到内存中的FAT缓冲区）。

参数：
	Vcb -- 卷控制块指针。
	Index -- 指定项的索引。

返回值：
	FAT项的值。

--*/
{
	USHORT Value;

	ASSERT(2 <= Index && Index < 0xFF0);

	//
	// 将包含FAT项的连续两个字节读入一个16位整形变量。
	//
	CopyUchar2(&Value, (PCHAR)Vcb->Fat + Index * 3 / 2)

	//
	// 根据索引的奇/偶取整形变量的高/低12位的值。
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

功能描述：
	写FAT中指定项的值，写FAT缓冲区的同时写穿透到磁盘。

参数：
	Vcb -- 卷控制块指针。
	Index -- 指定项的索引。
	Value12 -- 期望写入的12bit值。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
	// 将包含FAT项的连续两个字节读入一个16位整形变量OriginalValue。
	//
	CopyUchar2(&OldValue16, (PCHAR)Vcb->Fat + Index * 3 / 2);

	//
	// 根据索引的奇偶属性，仅修改16位中的高12位或低12位的值，其余4位不变。
	//
	NewValue16 = (Index & 0x01) != 0 ? Value12 << 4 | (OldValue16 & 0x000F) : (OldValue16 & 0xF000) | Value12;

	//
	// 将修改后的值写入FAT表缓冲区。
	//
	CopyUchar2((PUCHAR)Vcb->Fat + Index * 3 / 2, &NewValue16);

	//
	// 将修改后的值写入磁盘中的FAT表。因为磁盘上可能有多份FAT表，所以要循环写。
	//
	for (i = 0; i < Vcb->Bpb.Fats; i++) {

		//
		// 计算表项所在的扇区号以及在扇区内的字节偏移。
		//
		SectorNumber = Vcb->Bpb.ReservedSectors + i * Vcb->Bpb.SectorsPerFat + (Index * 3 / 2) / Vcb->Bpb.BytesPerSector;
		BytesOffset = (Index * 3 / 2) % Vcb->Bpb.BytesPerSector;
		
		//
		// 将修改后的2字节写入磁盘。注意：要考虑到2个字节可能跨扇区。
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
			// 2个字节正好跨扇区了，写前一扇区的最后1字节，写后一扇区的第一个1字节。
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

功能描述：
	检查路径字符串是否有效，路径中的所有文件名都必须符合8.3规则，规则如下：
	文件名由1~8个字符组成，扩展名由1~3个字符组成，扩展名可选。如果存在扩展名，则
	文件名和扩展名之间由'.'分割。例如：G9401.DBF，这里G9401是文件名，DBF是扩展名。
	文件名和扩展名可以使用字母(不区分大小写）、数字和各种符号，空格 < > / \ | : "
	* ?等字符除外。

参数：
	PathName -- 路径名。
	IsDirectoryName -- 是否目录路径。

返回值：
	如果路径有效则返回TRUE。

--*/
{
	PCSTR Dot;
	PCSTR Name = PathName;
	PCSTR End = PathName;
	ULONG CountOfFileName = 0;

	for (;;) {

		//
		// 向后搜索路径中的分隔符（正斜杠或者反斜杠），同时检查是否包含非法字符。
		//
		while (*End != '\0' && *End != '\\' && *End != '/') {

			//
			// 8.3文件名中不能包含以下字符：
			// · 值小于等于0x20的字符，但有例外，日文文件名的第一个字符可以是0x05，
			//    用以替换日文字符0xE5。
			// · 0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
			//    0x3F, 0x5B, 0x5C, 0x5D，and 0x7C.
			// 为了简单，不考虑日文文件名首个字符是0x05的情况。
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
			// 如果文件名不是当前目录"."或上级目录".."，则必须满足8.3规则。
			//
			if (!(End - Name == 1 && '.' == *Name) &&
				!(End - Name == 2 && '.' == *Name && '.' == *(Name+1))) {

				//
				// 8.3文件名第一个字符不可以是'.'且最长12个字符，否则文件名无效。
				//
				if ('.' == *Name || End - Name > 12) {
					return FALSE;
				}

				//
				// 从后向前搜索文件名和扩展名之间的分隔符'.'。
				//
				for (Dot = End - 1; Dot > Name && *Dot != '.'; Dot--);

				if (Dot > Name) {

					//
					// 找到'.'，文件名最多8个字符，扩展名须1~3个字符，否则无效。
					//
					if (Dot - Name > 8 || End - Dot == 1 || End - Dot > 4) {
						return FALSE;
					}

					//
					// 继续向前搜索，文件名中不可以有'.'，否则无效。
					//
					for (Dot--; Dot > Name && *Dot != '.'; Dot--);
					
					if (Dot > Name) {
						return FALSE;
					}

				} else {

					//
					// 不存在'.'，没有扩展名，文件名最多8个字符，否则无效。
					//
					if (End - Name > 8) {
						return FALSE;
					}
				}

				CountOfFileName++;	// 统计路径中有效的文件名数量。
			}
		}

		if ('\0' == *End) {
			
			if (End-- == PathName) {
				return FALSE; // 路径字符串长度为0，无效！
			}

			if (!IsDirectoryName) {

				//
				// 文件路径中至少要有一个文件名且最后一个字符不能是'\'或'/'或'.'。
				//
				if (0 == CountOfFileName || '\\' == *End || '/' == *End || '.' == *End) {
					return FALSE;
				}
			}

			return TRUE;

		}

		//
		// 继续检查路径中下一个文件名。
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

功能描述：
	将目录项中的11字符文件名转换为以0结尾的字符串，字符串格式：*.*。

参数：
	DirName -- 目录项中文件名字符数组。
	FileName -- 存储转换结果的字符串缓冲区指针。

返回值：
	如果成功则返回TRUE。

--*/
{
	INT i;
	PCHAR Dot;

	//
	// 过滤掉文件名后面用于填充的空格。
	//
	for (i = 7; i >= 0 && ' ' == DirName[i]; i--); // 分号！
	i++;

	//
	// 拷贝文件名。
	//
	strncpy(FileName, &DirName[0], i);

	Dot = FileName + i;

	//
	// 过滤掉扩展名后面用于填充的空格。
	//
	for (i = 10; i > 7 && ' ' == DirName[i]; i--);

	//
	// 如果存在扩展名，则在文件名后面添加'.'和扩展名。
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
	// 先复制文件名，如果不够8字符则用空格补齐。
	//
	for (i = 0, p = FileName; i < 8 && *p != '.' && *p != 0; i++, p++) {
		DirName[i] = *p;
	}

	for (; i < 8; i++) {
		DirName[i] = ' ';
	}

	//
	// 复制扩展名，如果不够3字符则用空格补齐。
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

功能描述：
	分配一个空闲簇。新分配的簇在 FAT 表中对应的表项将被自动设置为 0xFF8，
	所以，新分配的簇必须做为文件簇链中的最后一个簇。

参数：
	Vcb -- 磁盘卷控制块指针。
	ClusterNumber -- 返回成功分配的簇号。

返回值：
	如果成功则返回 STATUS_SUCCESS。

--*/
{
	STATUS Status;
	USHORT n;

	// 搜索 FAT 表，查找第一个值为 0 的 FAT 表项，此项对应的簇是未用的。
	// 注意簇号从 2 开始。
	for (n = 2; n < Vcb->NumberOfClusters + 2; n++) {
		if (0 == FatGetFatEntryValue(Vcb, n))
			break;
	}

	// 如果未找到可用的簇则返回错误码。
	if (n == Vcb->NumberOfClusters + 2)
		return STATUS_NO_SPACE;

	// 修改 FAT 表项的值为 0xFF8，标记此簇被文件占用，且是文件的最后一个簇。
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

功能描述：
	在文件的指定偏移处读取指定字节的数据，实际读取的字节数可能受到文件长度的限制
	而小于期望值。

参数：
	Vcb -- 卷控制块指针。
	File -- 文件控制块指针。
	Offset -- 读取的文件内起始偏移地址。
	BytesToRead -- 期望读取的字节数。
	Buffer -- 用于存放读取数据的缓冲区的指针。
	BytesRead -- 实际读取的字节数。

返回值：
	如果成功则返回STATUS_SUCCESS。

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
	// 如果读取文件的起始偏移位置超出文件大小，就直接返回。
	//
	if (Offset >= File->FileSize) {
		*BytesRead = 0;
		return STATUS_SUCCESS;
	}

	//
	// 实际可以读取的字节数受文件长度的限制。
	//
	if (BytesToRead > File->FileSize - Offset) {
		BytesToRead = File->FileSize - Offset;
	}

	//
	// 顺着文件簇链表向后查找，找到偏移位置所在的簇。
	//
	Cluster = File->FirstCluster;
	for (i = Offset / FatBytesPerCluster(&Vcb->Bpb); i > 0; i--) {
		Cluster = FatGetFatEntryValue(Vcb, Cluster);
	}

	//
	// 从偏移位置所在的簇开始向后读取文件的簇，直到读取完成。
	//
	for (;;) {

		//
		// 簇由多个连续扇区组成，计算簇的起始扇区号。
		//
		FirstSectorOfCluster = Vcb->FirstDataSector + (Cluster - 2) * Vcb->Bpb.SectorsPerCluster;

		//
		// 计算偏移位置在簇内的第几个扇区，然后从这个扇区开始读取簇内的连续扇区。
		//
		for (i = ((Offset + ReadCount) / Vcb->Bpb.BytesPerSector) % Vcb->Bpb.SectorsPerCluster;
			i < Vcb->Bpb.SectorsPerCluster; i++ ) {

			//
			// 计算读取位置在扇区内的字节偏移。
			//
			OffsetInSector = (Offset + ReadCount) % Vcb->Bpb.BytesPerSector;

			//
			// 计算需要在这个扇区内读取的字节数。
			//
			if (BytesToRead - ReadCount > Vcb->Bpb.BytesPerSector - OffsetInSector) {
				BytesToReadInSector = Vcb->Bpb.BytesPerSector - OffsetInSector;
			} else {
				BytesToReadInSector = BytesToRead - ReadCount;
			}

			//
			// 读取扇区数据。
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
			// 如果读取完成则返回。
			//
			ReadCount += BytesToReadInSector;
			if (ReadCount == BytesToRead) {
				*BytesRead = ReadCount;
				return STATUS_SUCCESS;
			}
		}

		//
		// 继续读文件的下一个簇。
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

功能描述：
	在文件指定的偏移位置开始写数据，如果偏移位置小于文件大小则覆盖原有内容，如果
	写范围超出文件大小则自动增加文件大小，如果文件大小增加后超过文件占用的磁盘空
	间大小则自动为文件分配新的簇，增加文件占用的磁盘空间。
	这里注意两个概念，文件大小和文件占用磁盘空间。因为文件占用磁盘空间是以簇为单
	位的，而文件大小的单位是字节，所以文件大小 <= 文件占用磁盘空间，所以当文件长
	度增加时，并不一定要增加磁盘占用空间。例如一个文件当前只有1字节，那么它占用了
	一个簇的磁盘空间。当文件大小增加到10字节时，它占用的磁盘空间仍然为一个簇。当
	它的大小增加到超过一个簇的大小时，那么就需要为它增加磁盘空间了。

参数：
	Vcb -- 卷控制块指针。
	File -- 文件控制块指针。
	Offset -- 开始写的偏移位置。
	BytesToWrite -- 写的字节数。
	Buffer -- 指向存放要写的数据。
	BytesWriten -- 指针，指向用于保存实际完成写的字节数的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

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

功能描述：
	在指定的目录中打开指定名称的数据文件或者目录文件。目录中每打开一个文件，目录
	文件控制块的打开计数器将被加1。

参数：
	Vcb -- 卷控制块指针。
	Directory -- 欲打开文件的所在目录的指针，如果为NULL则在根目录中打开。
	FileName -- 文件名字符串指针。
	OpenedFile -- 指向输出被打开文件指针的缓冲区。

返回值：
	如果打开成功则返回STATUS_SUCCESS。

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
	// 必须是根目录或有效的目录文件。
	//
	ASSERT(NULL == Directory || Directory->AttrDirectory && Directory->Name[0] != (CHAR)0xE5);

	//
	// 先检查文件名是否是特殊文件名："."和".."。
	// "."表示当前目录，".."表示当前目录的上级目录。
	//
	if (0 == strcmp(FileName, ".")) {

		//
		// 打开当前目录。
		//
		if (NULL != Directory) {
			Directory->OpenCount++;
		}
		*OpenedFile = Directory;
		
		return STATUS_SUCCESS;

	} else if (0 == strcmp(FileName, "..")) {

		//
		// 如果当前目录是根目录则仍然返回根目录(NULL)，否则打开当前目录的上级目录。
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
	// 使FileListHead指向目录中已打开文件的链表，然后遍历链表，查询要打开的文件
	// 是否已经在被打开，如果已经被打开则仅增加其打开计数器即可。
	//
	if (NULL == Directory) {

		FileListHead = &Vcb->FileListHead;

	} else {

		//
		// 不能在已经被独占打开的目录中打开文件。
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
		// 文件名的第一个字符为0xE5表明文件已经被删除。
		//
		if (File->Name[0] != (CHAR)0xE5 && 0 == strcmp(File->Name, FileName)) {

			File->OpenCount++;
			*OpenedFile = File;

			return STATUS_SUCCESS;
		}
	}

	//
	// 下面搜索目录中的DIRENT结构体，需要读磁盘。
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
			// 在根目录的Offset偏移处读取一个目录项。
			//
			Status = IopReadWriteSector( Vcb->DiskDevice,
										 Vcb->FirstRootDirSector + DirEntryOffset / Vcb->Bpb.BytesPerSector,
										 DirEntryOffset % Vcb->Bpb.BytesPerSector,
										 &DirEntry,
										 sizeof(DIRENT),
										 TRUE );
		} else {

			//
			// 在目录文件的Offset偏移处读取一个目录项。
			//
			Status = FatReadFile( Vcb,
								  Directory,
								  DirEntryOffset,
								  sizeof(DIRENT),
								  &DirEntry,
								  &BytesRead );
		}

		//
		// 如果读取失败则立刻返回。
		//
		if (!EOS_SUCCESS(Status)) {
			return Status;
		}

		//
		// 如果搜索完目录项(第一个字符为0的目录项标志目录文件结束)，则结束搜索。
		//
		if (0 == DirEntry.Name[0]) {
			return STATUS_FILE_NOT_FOUND;
		}

		//
		// 若目录项无效(第一个字符为0xE5)或文件名不一致，则继续向后搜索。
		//
		if ((CHAR)0xE5 == DirEntry.Name[0] ||
			0 != strnicmp(DirEntry.Name, DirName, 11)) {
			continue;
		}

		//
		// 检查文件的簇链表并统计簇链表的长度。
		//
		NumberOfClusters = 0;

		if (DirEntry.FirstCluster != 0) {

			for (Cluster = DirEntry.FirstCluster;
				Cluster < 0xFF8;
				Cluster  = FatGetFatEntryValue(Vcb, Cluster)) {

				//
				// 如果簇链表中出现无效的簇号（小于2或大于最大簇号），则返回错误。
				//
				if (Cluster < 2 || Cluster > Vcb->NumberOfClusters + 1) {
					return STATUS_FILE_CORRUPT_ERROR;
				}

				NumberOfClusters++; // 统计文件占用簇的数量。
			}
		}

		//
		// 数据文件的目录项记录中的文件大小必须和其占用簇的数量一致，目录文件的
		// 目录项中记录的文件大小必须是0且至少占用一个簇。
		//
		if ((DirEntry.Attributes & DIRENT_ATTR_ARCHIVE) != 0) {
			if ((DirEntry.FileSize + FatBytesPerCluster(&Vcb->Bpb) - 1) / FatBytesPerCluster(&Vcb->Bpb) != NumberOfClusters) {
				return STATUS_FILE_CORRUPT_ERROR;
			}
		} else if (DirEntry.FileSize != 0 || 0 == NumberOfClusters) {
			return STATUS_FILE_CORRUPT_ERROR;
		}
		

		//
		// 从系统内存池中分配一个文件控制块。
		//
		File = (PFCB)MmAllocateSystemPool(sizeof(FCB));

		if (NULL == File) {
			return STATUS_NO_MEMORY;
		}

		//
		// 初始化FCB并将之插入所在目录的文件链表中。
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
		// 数据文件可以直接获得文件的大小，但是目录文件需要根据簇的数量来计算，
		// 因为目录文件的目录项中的FileSize永远为0。
		//
		if (!File->AttrDirectory) {
			File->FileSize = DirEntry.FileSize;
		} else {
			ASSERT(0 == DirEntry.FileSize);
			File->FileSize = NumberOfClusters * FatBytesPerCluster(&Vcb->Bpb);
		}

		// 
		// 增加目录文件的打开计数器。
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

功能描述：
	打开给定路径的数据文件或目录文件。

参数：
	Vcb -- 卷控制块指针。
	FullPathname -- 文件在磁盘内的全路径名。
	OpenedFile -- 指向输出被打开文件指针的缓冲区。

返回值：
	如果打开成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PCSTR PathnamePtr;
	CHAR FileName[13];	// 文件名最长12个字符（8个文件名 + 1个'.' + 3个扩展名）。
	PSTR FileNamePtr;
	PFCB ParentFcb;
	PFCB ChildFcb;

	ParentFcb = NULL;
	ChildFcb = NULL;
	PathnamePtr = FullPathname;

	for (;;) {

		//
		// 向后搜索路径中的分隔符（正斜杠或者反斜杠），将两个分隔符之间的文件名拷贝到
		// 文件名缓冲区FileName[13]中。
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
			// 目录文件指针不再使用了，关闭它。
			//
			if (NULL != ParentFcb) {
				FatCloseFile(ParentFcb);
			}

			if (!EOS_SUCCESS(Status)) {

				//
				// 未找到目录文件时，返回STATUS_PATH_NOT_FOUND。
				//
				if (STATUS_FILE_NOT_FOUND == Status && *PathnamePtr != '\0') {
					return STATUS_PATH_NOT_FOUND;
				}

				return Status;
			}

			//
			// 未找到目录文件时，返回STATUS_PATH_NOT_FOUND。
			//
			if (*PathnamePtr != '\0' && NULL != ChildFcb && !ChildFcb->AttrDirectory) {
				FatCloseFile(ChildFcb);
				return STATUS_PATH_NOT_FOUND;
			}
		}

		//
		// 如果路径结束则跳出循环。
		//
		if ('\0' == *PathnamePtr) {
			break;
		}

		//
		// 跳过分隔符，继续打开子目录文件。
		//
		PathnamePtr++;
		ParentFcb = ChildFcb;
	}

	//
	// 如果未打开任何文件，说明路径为空串或者不存在有效的文件名（全部是分隔符）。
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
	// 打开文件。
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
	// 检查打开的文件类型是否符合要求。
	//
	if (FileObject->FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) {

		if (!Fcb->AttrDirectory) {
			FatCloseFile(Fcb);
			return STATUS_PATH_NOT_FOUND; // 目录文件不存在。
		}

	} else {

		if (Fcb->AttrDirectory) {
			FatCloseFile(Fcb);
			return STATUS_ACCESS_DENIED; // 拒绝以数据方式打开目录文件。
		}
	}

	if (Fcb->AttrReadOnly && FileObject->WriteAccess) {
		FatCloseFile(Fcb);
		return STATUS_ACCESS_DENIED; // 拒绝对只读文件的写请求。
	}

	//
	// 如果文件是第一次被打开则设置共享权限，否则检查是否共享冲突。
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

功能描述：
	写文件对应的DIRENT结构体到磁盘，文件的属性、长度、时间戳等被修改后，需要执行此函数。

参数：
	Vcb -- 磁盘卷控制块指针。
	Fcb -- 文件控制块指针。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	DIRENT DirEntry = {0};
	ULONG BytesWriten;

	//
	// 由FCB结构体初始化一个DIRENT结构体。
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
	// 如果文件位于根目录则写根目录，否则写所在目录文件。
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

功能描述：
	关闭文件控制块。减小文件控制块的打开计数器，如果计数器变为0则关闭文件控制块
	并减小文件所在目录的文件控制块的打开计数器，以此递归。

参数：
	File -- 文件控制块指针。

返回值：
	无。

--*/
{
	PFCB Current;
	PFCB Parent;

	ASSERT(NULL != Fcb && Fcb->OpenCount > 0);

	for (Current = Fcb; Current != NULL; Current = Parent) {

		//
		// 如果当前文件的打开计数器减1后仍大于0则返回。
		//
		if (--Current->OpenCount > 0) {
			break;
		}

		//
		// 记录当前文件的所在目录，关闭当前文件后还要递归关闭当前文件所在目录。
		//
		Parent = Current->ParentDirectory;

		//
		// 将当前文件从所在目录的文件链表中移除。
		//
		ListRemoveEntry(&Current->FileListEntry);

		//
		// 释放FCB占用的内存。
		//
		MmFreeSystemPool(Current);
	}
}
