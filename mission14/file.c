/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: file.c

����: �ļ������ʵ�֣������ļ�����Ĵ���������д����ѯ�����á�



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
		FILE_ATTRIBUTE_DIRECTORY��Ŀ¼�ļ�����FILE_ATTRIBUTE_ARCHIVE��⣻
		FILE_ATTRIBUTE_ARCHIVE�����ڴ洢���ݵ��ļ�����FILE_ATTRIBUTE_DIRECTORY��⣻

	FileObject -- ָ�룬ָ����������ļ�����ָ��Ļ�������

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��STATUS_FILE_ALLREADY_EXISTS��

--*/
{
	STATUS Status;
	PCHAR SlashPtr;
	CHAR Slash;
	PSTR RelativName;
	PDEVICE_OBJECT DeviceObject = NULL;
	PFILE_OBJECT File = NULL;

	//
	// �������Ƿ���ȷ��
	//

	//
	// ���ΪDesiredAccess��ShareModeָ���˲���ʶ�Ĳ����򷵻�ʧ�ܡ�
	//
	if ((DesiredAccess & ~(GENERIC_READ | GENERIC_WRITE)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	if ((ShareMode & ~(FILE_SHARE_READ | FILE_SHARE_WRITE)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ����ΪCreationDispositionָ��һ����Ч��ֵ��
	//
	if (CreationDisposition != CREATE_NEW &&
		CreationDisposition != CREATE_ALWAYS &&
		CreationDisposition != OPEN_EXISTING &&
		CreationDisposition != OPEN_ALWAYS &&
		CreationDisposition != TRUNCATE_EXISTING) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ���ָ����TRUNCATE_EXISTING�������дȨ�ޡ�
	//
	if (TRUNCATE_EXISTING == CreationDisposition && 0 == (DesiredAccess & GENERIC_WRITE)) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ����в���ʶ�ı�־�������򷵻�ʧ�ܡ�
	//
	if ((FlagsAndAttributes & ~(FILE_ATTRIBUTE_READONLY |
		FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM |
		FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) != 0) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ����ָ��FILE_ATTRIBUTE_DIRECTORY��FILE_ATTRIBUTE_ARCHIVE����֮һ��
	//
	if ((FlagsAndAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) == 0||
		(FlagsAndAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE)) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// ���Ŀ����Ŀ¼��������Ǵ����еĻ򴴽��µ�Ŀ¼��Ŀ¼���ܱ���д��
	//
	if ((FlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
		(CreationDisposition != CREATE_NEW && CreationDisposition != OPEN_EXISTING || DesiredAccess != 0)) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// �����ļ����еĵ�һ��б��'\'��'/'��
	//
	for (SlashPtr = FileName; *SlashPtr != '\0' && *SlashPtr != '\\' && *SlashPtr != '/'; SlashPtr++);

	//
	// б��֮ǰ����Ϊ���豸������б�ܴ��ض��ַ����������豸�����豸����Ȼ��ָ�б�ܵ�ֵ��
	//
	Slash = *SlashPtr;	// ����б�ܵ�ֵ��
	*SlashPtr = '\0';	// �ض��ַ�����
	Status = ObRefObjectByName(FileName, IopDeviceObjectType, (PVOID*)&DeviceObject);
	*SlashPtr = Slash;	// �ָ�б�ܵ�ֵ

	//
	// ����豸�����ڻ��豸��֧�ֱ�ֱ�Ӵ򿪣��򷵻�STATUS_PATH_NOT_FOUND��
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
	// �����ļ����󲢳�ʼ��֮��
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
	// ִ�����������Create���ܺ�����ʹ��б��֮��������豸����·������
	//
	for (RelativName = SlashPtr; '\\' == *RelativName || '/' == *RelativName; RelativName++);
	
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	
	Status = DeviceObject->DriverObject->Create(DeviceObject, RelativName, CreationDisposition, File);

	if (EOS_SUCCESS(Status)) {

		//
		// ��������ɹ�ִ��Create���������FileObject��FsContextָ��ֵ��
		//
		ASSERT(File->FsContext != NULL);
		if (NULL == File->FsContext) {
			KeBugCheck("%s:%d:Checked a driver error!", __FILE__, __LINE__);
		}

		DeviceObject->OpenCount++;
		
		if (File->FsContext == DeviceObject) {

			//
			// �ļ�����ֱ�Ӻ��豸��������˹���������ǳ��δ��豸�������մ���
			// Ҫ�������豸�Ĺ������ԣ�����Ӧ����Ƿ���ڹ����ͻ��
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

����������
	�ļ����������������

������
	FileObject -- �ļ�����ָ�롣

����ֵ��
	�ޡ�

--*/
{
	PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;

	if (DeviceObject != NULL) {

		//
		// ���FsContext��ΪNULL��˵���Ѿ������豸����Ҫ�ر��豸��
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
		// �ر�ָ��FileObject->DeviceObject��
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

����������
	��ȡ�ļ�����

������
	File -- �ļ�����ָ�롣
	Buffer -- ָ�룬ָ�����������ȡ����Ļ�������
	NumberOfBytesToRead -- ������ȡ���ֽ�����
	NumberOfBytesRead -- ����ָ�룬ָ���������ʵ�ʶ�ȡ�ֽ����ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PDEVICE_OBJECT DeviceObject = File->DeviceObject;

	//
	// ����ļ��Ƿ�ɶ���
	//
	if (!File->ReadAccess) {
		return STATUS_ACCESS_DENIED;
	}

	//
	// ��������0�ֽ������̷��سɹ���
	//
	if (0 == NumberOfBytesToRead) {
		*NumberOfBytesRead = 0;
		return STATUS_SUCCESS;
	}

	//
	// ����ض�дͬһ���ļ�����
	//
	PsWaitForMutex(&File->Mutex, INFINITE);

	//
	// ִ�����������Read���ܺ�����
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

����������
	д�ļ�����

������
	File -- �ļ�����ָ�롣
	Buffer -- ָ�룬ָ����д�����ݵĻ�������
	NumberOfBytesToWrite -- ����д����ֽ�����
	NumberOfBytesWritten -- ����ָ�룬ָ���������ʵ��д���ֽ����ı�����

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PDEVICE_OBJECT DeviceObject = File->DeviceObject;

	//
	// ����Ƿ��д�ļ���
	//
	if (!File->WriteAccess) {
		return STATUS_ACCESS_DENIED;
	}

	//
	// �������д0�ֽ������̷��سɹ���
	//
	if (0 == NumberOfBytesToWrite) {
		*NumberOfBytesWritten = 0;
		return STATUS_SUCCESS;
	}

	//
	// �����дͬһ���ļ�����
	//
	PsWaitForMutex(&File->Mutex, INFINITE);

	//
	// ִ�����������Write���ܺ�����
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
	// ��������ļ�����
	//
	PsWaitForMutex(&FileObject->Mutex, INFINITE);

	//
	// ִ�����������Query���ܺ�����
	//
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	Status = DeviceObject->DriverObject->Query(DeviceObject, FileObject, FileInfo);
	PsReleaseMutex(&DeviceObject->Mutex);

	//
	// �ͷ��ļ�����Ļ����ź�����
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
	// ��������ļ�����
	//
	PsWaitForMutex(&FileObject->Mutex, INFINITE);

	//
	// ִ�����������Set���ܺ�����
	//
	PsWaitForMutex(&DeviceObject->Mutex, INFINITE);
	Status = DeviceObject->DriverObject->Set(DeviceObject, FileObject, FileInfo);
	PsReleaseMutex(&DeviceObject->Mutex);

	//
	// �ͷ��ļ�����Ļ����ź�����
	//
	PsReleaseMutex(&FileObject->Mutex);

	return Status;
}
