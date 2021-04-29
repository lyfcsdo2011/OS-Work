/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: io.c

����: IOģ��ӿں�����ʵ�֡�



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

����������
	��������ļ���Ŀ�������λ�ڴ����ļ�ϵͳ�е��ļ���Ҳ������IO�豸��

������
	FileName -- ����������򿪵��ļ������ơ�

	DesiredAccess -- ������õ��ļ�����Ȩ�ޣ�����GENERIC_READ��GENERIC_WRITE��

	ShareMode -- ����Ĺ���ģʽ������FILE_SHARE_READ��FILE_SHARE_WRITE��

	CreationDisposition -- ������ִ�ж�����ȡ�����ļ��Ƿ��Ѿ����ڡ�����ȡֵ���£�
		CREATE_ALWAYS������һ����СΪ0�����ļ�������ļ��Ѿ������򸲸��Ѵ����ļ���
		CREATE_NEW������һ����СΪ0�����ļ�������ļ��Ѿ������򷵻�ʧ�ܣ�
		OPEN_ALWAYS����һ���ļ�������ļ��������򴴽�һ�����ļ���
		OPEN_EXISTING����һ���ļ�������ļ��������򷵻�ʧ�ܣ�
		TRUNCATE_EXISTING����һ���ļ����ض�֮��СΪ0������ļ��������򷵻�ʧ�ܡ�

	FlagsAndAttributes -- �ļ������Ժͱ�־�����԰������£�
		FILE_ATTRIBUTE_READONLY�� ֻ���ļ���
		FILE_ATTRIBUTE_HIDDEN�������ļ���
		FILE_ATTRIBUTE_SYSTEM��ϵͳ�ļ���

	FileHandle -- ָ�룬ָ����������ļ�����ָ��Ļ�������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��STATUS_FILE_ALLREADY_EXISTS��

--*/
{
	STATUS Status;
	PFILE_OBJECT FileObject;

	//
	// ֻ�ܴ������ļ������ܴ�Ŀ¼�ļ���
	//
	if ((FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ����ֱ�Ӵ���Ŀ¼�ļ���
	//
	if ((FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// �Զ�����FILE_ATTRIBUTE_ARCHIVE���ԡ�
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

����������
	����һ��Ŀ¼��

������
	PathName -- Ŀ¼·���ַ�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

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
// �����ļ���дλ�á�
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
	// ��������ļ�����
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
// �õ��ļ�������ֵ��
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
	// ������ļ���
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
	// �ر��ļ���
	//
	ObDerefObject(FileObject);

	if (EOS_SUCCESS(Status)) {
		*FileAttributes = FileInfo.FileAttributes;
	}

	return Status;
}

//
// �޸��ļ�������ֵ��
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
	// ��ռ���ļ���
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
	// �ر��ļ���
	//
	ObDerefObject(FileObject);

	return Status;
}

//
// ɾ���ļ���
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
	// ��ռ���ļ���
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
	// �ر��ļ���
	//
	ObDerefObject(FileObject);

	return Status;
}

//
// ɾ��һ��Ŀ¼��
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
	// ��ռĿ¼���ļ���
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
	// �ر��ļ���
	//
	ObDerefObject(FileObject);

	return Status;
}
