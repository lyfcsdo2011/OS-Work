/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: file.c

描述: 文件对象的实现，包括文件对象的创建、读、写、查询、设置。



*******************************************************************************/

#include "iop.h"

STATUS
IopCreateFileObject(
	IN PSTR FileName, 
	IN ULONG DesiredAccess, 
	IN ULONG ShareMode, 
	IN ULONG CreationDisposition, 
	IN ULONG FlagsAndAttributes, 
	OUT PFILE_OBJECT *FileObject
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
		FILE_ATTRIBUTE_DIRECTORY：目录文件，和FILE_ATTRIBUTE_ARCHIVE相斥；
		FILE_ATTRIBUTE_ARCHIVE：用于存储数据的文件，和FILE_ATTRIBUTE_DIRECTORY相斥；

	FileObject -- 指针，指向用于输出文件对象指针的缓冲区。

返回值：
	如果成功则返回STATUS_SUCCESS或STATUS_FILE_ALLREADY_EXISTS。

--*/
{
	STATUS Status;
	PCHAR SlashPtr;
	CHAR Slash;
	PSTR RelativName;
	PDEVICE_OBJECT DeviceObject = NULL;
	PFILE_OBJECT File = NULL;

	//
	// 检查参数是否正确。
	//

	//
	// 如果为DesiredAccess、ShareMode指定了不认识的参数则返回失败。
	//
	if ((DesiredAccess & ~(GENERIC_READ | GENERIC_WRITE)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	if ((ShareMode & ~(FILE_SHARE_READ | FILE_SHARE_WRITE)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 必须为CreationDisposition指定一个有效的值。
	//
	if (CreationDisposition != CREATE_NEW &&
		CreationDisposition != CREATE_ALWAYS &&
		CreationDisposition != OPEN_EXISTING &&
		CreationDisposition != OPEN_ALWAYS &&
		CreationDisposition != TRUNCATE_EXISTING) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 如果指定了TRUNCATE_EXISTING则必须有写权限。
	//
	if (TRUNCATE_EXISTING == CreationDisposition && 0 == (DesiredAccess & GENERIC_WRITE)) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 如果有不认识的标志或属性则返回失败。
	//
	if ((FlagsAndAttributes & ~(FILE_ATTRIBUTE_READONLY |
		FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM |
		FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 必须指定FILE_ATTRIBUTE_DIRECTORY和FILE_ATTRIBUTE_ARCHIVE其中之一。
	//
	if ((FlagsAndAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) == 0||
		(FlagsAndAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 如果目标是目录，则必须是打开已有的或创建新的目录，目录不能被读写。
	//
	if ((FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
		(CreationDisposition != CREATE_NEW && CreationDisposition != OPEN_EXISTING || DesiredAccess != 0)) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 搜索文件名中的第一个斜杠'\'或'/'。
	//
	for (SlashPtr = FileName; *SlashPtr != '\0' && *SlashPtr != '\\' && *SlashPtr != '/'; SlashPtr++);

	//
	// 斜杠之前被认为是设备名，在斜杠处截断字符串，根据设备名打开设备对象，然后恢复斜杠的值。
	//
	Slash = *SlashPtr;	// 保存斜杠的值。
	*SlashPtr = '\0';	// 截断字符串。
	Status = ObRefObjectByName(FileName, IopDeviceObjectType, (PVOID*)&DeviceObject);
	*SlashPtr = Slash;	// 恢复斜杠的值

	//
	// 如果设备不存在或设备不支持被直接打开，则返回STATUS_PATH_NOT_FOUND。
	//
	if (EOS_SUCCESS(Status)) {
		if (NULL == DeviceObject->DriverObject->Create) {
			ObDerefObject(DeviceObject);
			return STATUS_PATH_NOT_FOUND;
		}
	} else {
		ASSERT(STATUS_OBJECT_NAME_NOT_FOUND == Status);
		return STATUS_PATH_NOT_FOUND;
	} 

	//
	// 创建文件对象并初始化之。
	//
	Status = ObCreateObject( IopFileObjectType,
							 NULL,
							 sizeof(FILE_OBJECT),
							 0,
							 (PVOID*)&File );

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(DeviceObject);
		return Status;
	}

	File->DeviceObject = DeviceObject;
	File->FsContext = NULL;
	File->ReadAccess = (DesiredAccess & GENERIC_READ) != 0;
	File->WriteAccess = (DesiredAccess & GENERIC_WRITE) != 0;
	File->SharedRead = (ShareMode & FILE_SHARE_READ) != 0;
	File->SharedWrite = (ShareMode & FILE_SHARE_WRITE) != 0;
	File->FlagsAndAttributes = FlagsAndAttributes;
	File->CurrentByteOffset = 0;
	PsInitializeMutex(&File->Mutex, FALSE);

	//
	// 执行驱动程序的Create功能函数，使用斜杠之后相对于设备名的路径名。
	//
	for (RelativName = SlashPtr; '\\' == *RelativName || '/' == *RelativName; RelativName++);
	
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	
	Status = DeviceObject->DriverObject->Create(DeviceObject, RelativName, CreationDisposition, File);

	if (EOS_SUCCESS(Status)) {

		//
		// 驱动程序成功执行Create后必须设置FileObject的FsContext指针值。
		//
		ASSERT(File->FsContext != NULL);
		if (NULL == File->FsContext) {
			KeBugCheck("%s:%d:Checked a driver error!", __FILE__, __LINE__);
		}

		DeviceObject->OpenCount++;
		
		if (File->FsContext == DeviceObject) {

			//
			// 文件对象直接和设备对象进行了关联，如果是初次打开设备对象则按照打开者
			// 要求设置设备的共享属性，否则应检查是否存在共享冲突。
			//
			if (1 == DeviceObject->OpenCount) {

				DeviceObject->ShareRead = File->SharedRead;
				DeviceObject->ShareWrite = File->SharedWrite;

			} else if ( File->SharedRead != DeviceObject->ShareRead ||
						File->SharedWrite != DeviceObject->ShareWrite ||
						File->ReadAccess && !DeviceObject->ShareRead ||
						File->WriteAccess && !DeviceObject->ShareWrite ) {

				Status = STATUS_SHARING_VIOLATION;
			}
		}
	}

	PsReleaseMutex(&DeviceObject->Mutex);

	if (EOS_SUCCESS(Status)) {
		*FileObject =File;
	} else {
		ObDerefObject(File);
	}

	return Status;
}

VOID
IopCloseFileObject(
	IN PFILE_OBJECT FileObject
	)
/*++

功能描述：
	文件对象的析构函数。

参数：
	FileObject -- 文件对象指针。

返回值：
	无。

--*/
{
	PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;

	if (DeviceObject != NULL) {

		//
		// 如果FsContext不为NULL则说明已经打开了设备，需要关闭设备。
		//
		if (FileObject->FsContext != NULL) {

			PsWaitForMutex(&DeviceObject->Mutex, INFINITE);

			DeviceObject->OpenCount--;
			if (DeviceObject->DriverObject->Close != NULL) {
				DeviceObject->DriverObject->Close(DeviceObject, FileObject);
			}

			PsReleaseMutex(&DeviceObject->Mutex);

			FileObject->FsContext = NULL;
		}

		//
		// 关闭指针FileObject->DeviceObject。
		//
		FileObject->DeviceObject = NULL;
		ObDerefObject(DeviceObject);
	}
}

STATUS
IopReadFileObject(
	IN PFILE_OBJECT File,
	OUT PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	)
/*++

功能描述：
	读取文件对象。

参数：
	File -- 文件对象指针。
	Buffer -- 指针，指向用于输出读取结果的缓冲区。
	NumberOfBytesToRead -- 期望读取的字节数。
	NumberOfBytesRead -- 整形指针，指向用于输出实际读取字节数的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PDEVICE_OBJECT DeviceObject = File->DeviceObject;

	//
	// 检查文件是否可读。
	//
	if (!File->ReadAccess) {
		return STATUS_ACCESS_DENIED;
	}

	//
	// 如果请求读0字节则立刻返回成功。
	//
	if (0 == NumberOfBytesToRead) {
		*NumberOfBytesRead = 0;
		return STATUS_SUCCESS;
	}

	//
	// 互斥地读写同一个文件对象。
	//
	PsWaitForMutex(&File->Mutex, INFINITE);

	//
	// 执行驱动程序的Read功能函数。
	//
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	Status = DeviceObject->DriverObject->Read(DeviceObject, File, Buffer, NumberOfBytesToRead, NumberOfBytesRead);
	PsReleaseMutex(&DeviceObject->Mutex);

	if (EOS_SUCCESS(Status)) {
		File->CurrentByteOffset += *NumberOfBytesRead;
	}

	PsReleaseMutex(&File->Mutex);

	return Status;
}

STATUS
IopWriteFileObject(
	IN PFILE_OBJECT File,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	)
/*++

功能描述：
	写文件对象。

参数：
	File -- 文件对象指针。
	Buffer -- 指针，指向存放写入数据的缓冲区。
	NumberOfBytesToWrite -- 期望写入的字节数。
	NumberOfBytesWritten -- 整形指针，指向用于输出实际写入字节数的变量。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PDEVICE_OBJECT DeviceObject = File->DeviceObject;

	//
	// 检查是否可写文件。
	//
	if (!File->WriteAccess) {
		return STATUS_ACCESS_DENIED;
	}

	//
	// 如果请求写0字节则立刻返回成功。
	//
	if (0 == NumberOfBytesToWrite) {
		*NumberOfBytesWritten = 0;
		return STATUS_SUCCESS;
	}

	//
	// 互斥地写同一个文件对象。
	//
	PsWaitForMutex(&File->Mutex, INFINITE);

	//
	// 执行驱动程序的Write功能函数。
	//
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	Status = DeviceObject->DriverObject->Write(DeviceObject, File, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
	PsReleaseMutex(&DeviceObject->Mutex);

	if (EOS_SUCCESS(Status)) {
		File->CurrentByteOffset += *NumberOfBytesWritten;
	}

	PsReleaseMutex(&File->Mutex);

	return Status;
}

STATUS
IopQueryFileObjectInfo(
	IN PFILE_OBJECT FileObject,
	OUT PFILE_INFO FileInfo
	)
{
	STATUS Status;
	PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;

	if (NULL == DeviceObject->DriverObject->Query) {
		return STATUS_NOT_SUPPORTED;
	}

	//
	// 互斥访问文件对象。
	//
	PsWaitForMutex(&FileObject->Mutex, INFINITE);

	//
	// 执行驱动程序的Query功能函数。
	//
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	Status = DeviceObject->DriverObject->Query(DeviceObject, FileObject, FileInfo);
	PsReleaseMutex(&DeviceObject->Mutex);

	//
	// 释放文件对象的互斥信号量。
	//
	PsReleaseMutex(&FileObject->Mutex);

	return Status;
}

STATUS
IopSetFileObjectInfo(
	IN PFILE_OBJECT FileObject,
	IN PSET_FILE_INFO FileInfo
	)
{
	STATUS Status;
	PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;

	if (NULL == DeviceObject->DriverObject->Set) {
		return STATUS_NOT_SUPPORTED;
	}

	//
	// 互斥访问文件对象。
	//
	PsWaitForMutex(&FileObject->Mutex, INFINITE);

	//
	// 执行驱动程序的Set功能函数。
	//
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	Status = DeviceObject->DriverObject->Set(DeviceObject, FileObject, FileInfo);
	PsReleaseMutex(&DeviceObject->Mutex);

	//
	// 释放文件对象的互斥信号量。
	//
	PsReleaseMutex(&FileObject->Mutex);

	return Status;
}
