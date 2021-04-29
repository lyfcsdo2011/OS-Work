/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: io.c

描述: IO模块接口函数的实现。



*******************************************************************************/

#include "iop.h"

STATUS 
IoCreateFile(
	IN PSTR FileName, 
	IN ULONG DesiredAccess, 
	IN ULONG ShareMode, 
	IN ULONG CreationDisposition, 
	IN ULONG FlagsAndAttributes, 
	OUT PHANDLE FileHandle
	)
/*++

功能描述：
	创建或打开文件。目标可以是位于磁盘文件系统中的文件，也可以是IO设备。

参数：
	FileName -- 期望创建或打开的文件的名称。

	DesiredAccess -- 期望获得的文件访问权限，包括GENERIC_READ和GENERIC_WRITE。

	ShareMode -- 对象的共享模式，包括FILE_SHARE_READ和FILE_SHARE_WRITE。

	CreationDisposition -- 函数的执行动作，取决于文件是否已经存在。可以取值如下：
		CREATE_ALWAYS：创建一个大小为0的新文件，如果文件已经存在则覆盖已存在文件；
		CREATE_NEW：创建一个大小为0的新文件，如果文件已经存在则返回失败；
		OPEN_ALWAYS：打开一个文件，如果文件不存在则创建一个新文件；
		OPEN_EXISTING：打开一个文件，如果文件不存在则返回失败；
		TRUNCATE_EXISTING：打开一个文件并截断之大小为0，如果文件不存在则返回失败。

	FlagsAndAttributes -- 文件的属性和标志，可以包括如下：
		FILE_ATTRIBUTE_READONLY： 只读文件；
		FILE_ATTRIBUTE_HIDDEN：隐藏文件；
		FILE_ATTRIBUTE_SYSTEM：系统文件；

	FileHandle -- 指针，指向用于输出文件对象指针的缓冲区。

返回值：
	如果成功则返回STATUS_SUCCESS或STATUS_FILE_ALLREADY_EXISTS。

--*/
{
	STATUS Status;
	PFILE_OBJECT FileObject;

	//
	// 只能打开数据文件，不能打开目录文件。
	//
	if ((FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 不能直接创建目录文件。
	//
	if ((FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 自动补充FILE_ATTRIBUTE_ARCHIVE属性。
	//
	FlagsAndAttributes |= FILE_ATTRIBUTE_ARCHIVE;

	Status = IopCreateFileObject( FileName,
								  DesiredAccess,
								  ShareMode,
								  CreationDisposition,
								  FlagsAndAttributes,
								  &FileObject );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	Status = ObCreateHandle(FileObject, FileHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(FileObject);
	}

	return Status;
}

STATUS
IoCreateDirectory(
	IN PSTR PathName
	)
/*++

功能描述：
	创建一个目录。

参数：
	PathName -- 目录路径字符串。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PFILE_OBJECT NewFile;

	Status = IopCreateFileObject( PathName,
								  0,
								  0,
								  CREATE_NEW,
								  FILE_ATTRIBUTE_DIRECTORY,
								  &NewFile );

	if (EOS_SUCCESS(Status)) {
		ObDerefObject(NewFile);
	}

	return Status;
}

STATUS
IoGetFileTime(
	IN HANDLE FileHandle,
	OUT PFILETIME CreationTime,
	OUT PFILETIME LastAccessTime,
	OUT PFILETIME LastWriteTime
	)
{
	STATUS Status;
	PFILE_OBJECT FileObject;
	FILE_INFO FileInfo;

	Status = ObRefObjectByHandle(FileHandle, IopFileObjectType, (PVOID*)&FileObject);

	if (EOS_SUCCESS(Status)) {

		Status = IopQueryFileObjectInfo(FileObject, &FileInfo);

		if (EOS_SUCCESS(Status)) {

			*CreationTime = FileInfo.CreationTime;
			*LastAccessTime = FileInfo.LastAccessTime;
			*LastWriteTime = FileInfo.LastWriteTime;
		}

		ObDerefObject(FileObject);
	}

	return Status;
}

STATUS 
IoGetFileSize(
	IN HANDLE FileHandle,
	OUT PULONG FileSize
	)
{
	STATUS Status;
	PFILE_OBJECT FileObject;
	FILE_INFO FileInfo;

	Status = ObRefObjectByHandle(FileHandle, IopFileObjectType, (PVOID*)&FileObject);

	if (EOS_SUCCESS(Status)) {

		Status = IopQueryFileObjectInfo(FileObject, &FileInfo);

		if (EOS_SUCCESS(Status)) {
			*FileSize = FileInfo.FileSize;
		}

		ObDerefObject(FileObject);
	}

	return Status;
}

//
// 设置文件读写位置。
//
STATUS
IoSetFilePointer(
	IN HANDLE FileHandle,
	IN LONG DistanceToMove,
	IN ULONG MoveMethod,
	OUT PULONG NewFilePointer
	)
{
	STATUS Status;
	PFILE_OBJECT FileObject;
	FILE_INFO FileInfo;

	if (MoveMethod != FILE_BEGIN && MoveMethod != FILE_CURRENT && MoveMethod != FILE_END) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = ObRefObjectByHandle(FileHandle, IopFileObjectType, (PVOID*)&FileObject);

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	//
	// 互斥访问文件对象。
	//
	PsWaitForMutex(&FileObject->Mutex, INFINITE);

	Status = IopQueryFileObjectInfo(FileObject, &FileInfo);

	if (EOS_SUCCESS(Status)) {

		if (FILE_BEGIN == MoveMethod) {

			if (DistanceToMove <= 0) {
				FileObject->CurrentByteOffset = 0;
			} else if ((ULONG)DistanceToMove >= FileInfo.FileSize) {
				FileObject->CurrentByteOffset = FileInfo.FileSize;
			} else {
				FileObject->CurrentByteOffset = DistanceToMove;
			}

		} else if (FILE_CURRENT == MoveMethod) {

			if (DistanceToMove < 0 && (ULONG)(-DistanceToMove) > FileObject->CurrentByteOffset) {
				FileObject->CurrentByteOffset = 0;
			} else if (DistanceToMove > 0 && FileObject->CurrentByteOffset + DistanceToMove >= FileInfo.FileSize) {
				FileObject->CurrentByteOffset = FileInfo.FileSize;
			} else {
				FileObject->CurrentByteOffset += DistanceToMove;
			}

		} else {

			if (DistanceToMove >= 0) {
				FileObject->CurrentByteOffset = FileInfo.FileSize;
			} else if ((ULONG)(-DistanceToMove) >= FileInfo.FileSize) {
				FileObject->CurrentByteOffset = 0;
			} else {
				FileObject->CurrentByteOffset = FileInfo.FileSize + DistanceToMove;
			}
		}

		*NewFilePointer = FileObject->CurrentByteOffset;

		Status = STATUS_SUCCESS;
	}

	PsReleaseMutex(&FileObject->Mutex);

	return Status;
}

//
// 得到文件的属性值。
//
STATUS
IoGetFileAttributes(
	IN PSTR FileName,
	OUT PULONG FileAttributes
	)
{
	STATUS Status;
	PFILE_OBJECT FileObject;
	FILE_INFO FileInfo;

	//
	// 共享打开文件。
	//
	Status = IopCreateFileObject( FileName,
								  0,
								  FILE_SHARE_READ | FILE_SHARE_READ,
								  OPEN_EXISTING,
								  FILE_ATTRIBUTE_ARCHIVE,
								  &FileObject );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	Status = IopQueryFileObjectInfo( FileObject, &FileInfo);

	//
	// 关闭文件。
	//
	ObDerefObject(FileObject);

	if (EOS_SUCCESS(Status)) {
		*FileAttributes = FileInfo.FileAttributes;
	}

	return Status;
}

//
// 修改文件的属性值。
//
STATUS
IoSetFileAttributes(
	IN PSTR FileName,
	IN ULONG FileAttributes
	)
{
	STATUS Status;
	PFILE_OBJECT FileObject;
	SET_FILE_INFO FileInfo;

	if ((FileAttributes & ~(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN |
		FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	if ((FileAttributes & FILE_ATTRIBUTE_ARCHIVE) == 0) {
		FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
	}

	//
	// 独占打开文件。
	//
	Status = IopCreateFileObject( FileName,
								  0,
								  0,
								  OPEN_EXISTING,
								  FILE_ATTRIBUTE_ARCHIVE,
								  &FileObject );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	FileInfo.FileInfoClass = FileAttributesInfo;
	FileInfo.u.AttributesInfo.FileAttributes = FileAttributes;

	Status = IopSetFileObjectInfo(FileObject, &FileInfo);

	//
	// 关闭文件。
	//
	ObDerefObject(FileObject);

	return Status;
}

//
// 删除文件。
//
STATUS
IoDeleteFile(
	IN PSTR FileName
	)
{
	STATUS Status;
	PFILE_OBJECT FileObject;
	SET_FILE_INFO FileInfo;

	//
	// 独占打开文件。
	//
	Status = IopCreateFileObject( FileName,
								  0,
								  0,
								  OPEN_EXISTING,
								  FILE_ATTRIBUTE_ARCHIVE,
								  &FileObject );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	FileInfo.FileInfoClass = FileAttributesInfo;
	FileInfo.u.DispositionInfo.DeleteFile = TRUE;

	Status = IopSetFileObjectInfo(FileObject, &FileInfo);

	//
	// 关闭文件。
	//
	ObDerefObject(FileObject);

	return Status;
}

//
// 删除一个目录。
//
STATUS
IoRemoveDirectory(
	IN PSTR PathName
	)
{
	STATUS Status;
	PFILE_OBJECT FileObject;
	SET_FILE_INFO FileInfo;

	//
	// 独占目录打开文件。
	//
	Status = IopCreateFileObject( PathName,
								  0,
								  0,
								  OPEN_EXISTING,
								  FILE_ATTRIBUTE_DIRECTORY,
								  &FileObject );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}

	FileInfo.FileInfoClass = FileAttributesInfo;
	FileInfo.u.DispositionInfo.DeleteFile = TRUE;

	Status = IopSetFileObjectInfo(FileObject, &FileInfo);

	//
	// 关闭文件。
	//
	ObDerefObject(FileObject);

	return Status;
}
