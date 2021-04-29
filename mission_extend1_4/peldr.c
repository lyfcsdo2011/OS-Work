/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: peldr.c

����: PE �ļ��ļ��ء�



*******************************************************************************/

#include "psp.h"
#include "io.h"

PRIVATE
PIMAGE_NT_HEADERS
PspGetImageNtHeaders(
	IN PVOID ImageHeader
	)
/*++

����������
	���PE�ļ�ͷ���ĸ�����־��ȷ���Ƿ���Ч��PE�ļ���������򷵻�PE�ļ�ͷ��
	IMAGE_NT_HEADERS�ṹ���ָ�롣

������
	ImageHeader -- PE�ļ�ͷ����

����ֵ��
	PE�ļ�ͷ���е�IMAGE_NT_HEADERS�ṹ��ָ�롣

--*/
{
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeaders;

	DosHeader = (PIMAGE_DOS_HEADER)ImageHeader;

	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return NULL;
	}

	NtHeaders = (PIMAGE_NT_HEADERS)(ImageHeader + DosHeader->e_lfanew);

	if (NtHeaders->Signature != IMAGE_NT_SIGNATURE) {
		return NULL;
	}

	if (NtHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HEADER_MAGIC) {
		return NULL;
	}

	return NtHeaders;
}

PRIVATE
PIMAGE_EXPORT_DIRECTORY
PspGetExportDirecotry(
	IN PVOID ImageBase,
	IN PCHAR ImageName
	)
/*++

����������
	�õ�ӳ��ĵ���Ŀ¼��

������
	ImageBase -- ӳ�񱻼��صĻ�ַ��
	ImageName -- ӳ������ơ�

����ֵ��
	���ӳ������Ʒ���Ҫ���򷵻�ӳ��ĵ���Ŀ¼�����򷵻�NULL��

--*/
{
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;
	ULONG Rva;
	
	NtHeaders = PspGetImageNtHeaders(ImageBase);

	if (NULL == NtHeaders) {
		return NULL;
	}

	Rva = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

	if (0 == Rva) {
		return NULL;
	}

	ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(ImageBase + Rva);

	if (0 != stricmp((PCHAR)(ImageBase + ExportDirectory->Name), ImageName)) {
		return NULL;
	}

	return ExportDirectory;
}

PRIVATE
ULONG
PspFindSymbolByOrdinal(
	IN PVOID ImageBase,
	IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
	IN USHORT Ordinal
	)
/*++

����������
	���ݵ��������������õ����������ĵ�ַ��

������
	ImageBase -- ����������ӳ��ļ��ػ�ַ��
	ExportDirectory -- ����������ӳ��ĵ���Ŀ¼��
	Ordinal -- ����������������

����ֵ��
	���������Ч�򷵻ض�Ӧ�ĺ�����ַ�����򷵻�0��

--*/
{
	PULONG FunctionArray;

	if (Ordinal < ExportDirectory->Base ||
		Ordinal >= ExportDirectory->Base + ExportDirectory->NumberOfFunctions) {
			return 0;
	}

	FunctionArray = (PULONG)(ImageBase + ExportDirectory->AddressOfFunctions);

	return FunctionArray[Ordinal - ExportDirectory->Base];
}

PRIVATE
ULONG
PspFindSymbolByName(
	IN PVOID ImageBase,
	IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
	IN PIMAGE_IMPORT_BY_NAME Name
	)
/*++

����������
	���ݵ��������ķ������Ƶõ������ĵ�ַ��

������
	ImageBase -- ����������ӳ��ļ��ػ�ַ��
	ExportDirectory -- ����������ӳ��ĵ���Ŀ¼��
	Name -- ���������ķ���������Ϣ��

����ֵ��
	�������ָ�����Ƶĵ��������򷵻غ�����ַ�����򷵻�0��

--*/
{
	PULONG FunctionArray;
	PULONG NameArray;
	PUSHORT NameOrdinalArray;
	ULONG Index;

	FunctionArray = (PULONG)(ImageBase + ExportDirectory->AddressOfFunctions);
	NameArray = (PULONG)(ImageBase + ExportDirectory->AddressOfNames);
	NameOrdinalArray = (PUSHORT)(ImageBase + ExportDirectory->AddressOfNameOrdinals);

	//
	// �Ȱ�����ʾ������ʾ���������ƣ����������ò�����
	//
	if (Name->Hint < ExportDirectory->NumberOfNames) {

		if (0 == strcmp((PCHAR)(ImageBase + NameArray[Name->Hint]), Name->Name)) {

			Index = NameOrdinalArray[Name->Hint];

			return (ULONG)ImageBase + FunctionArray[Index];
		}
	}

	//
	// ��ʾû�����У�������Ҫ���������������ָ���ķ��š�
	//
	for (Index = 0; Index < ExportDirectory->NumberOfNames; Index++) {

		if (0 == strcmp((PCHAR)(ImageBase + NameArray[Index]), Name->Name)) {

			Index = NameOrdinalArray[Index];

			return (ULONG)ImageBase + FunctionArray[Index];
		}
	}

	return 0;
}

PRIVATE
BOOL
PspLinkAppWithKernel(
	PVOID AppImageBase,
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor
	)
/*++

����������
	�����ص��ڴ��е�EOSӦ�ó�����ں˽������ӡ�

������
	AppImageBase -- Ӧ�ó���ļ��ػ�ַ��
	ImportDescriptor -- ӳ��ĵ�һ��������������ָ�롣

����ֵ��
	������з��Ŷ����ӳɹ��򷵻�TRUE�����򷵻�FALSE��

--*/
{
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;
	PULONG OriginalFirstThunk;
	PULONG FirstThunk;
	ULONG Index;
	ULONG Function;

	if (NULL == ImportDescriptor) {
		return FALSE;
	}

	//
	// ���û�е������������������ӡ�
	//
	if (0 == ImportDescriptor[0].u.Characteristics) {
		return TRUE;	
	}
	
	//
	// Ŀǰ������kernel.dll���ӣ����Ե���������Ӧ��ֻ��һ����
	//
	if(0 != ImportDescriptor[1].u.Characteristics) {
		return FALSE;
	}

	//
	// �õ�kernel.dll�ĵ���Ŀ¼��
	//
	ExportDirectory = PspGetExportDirecotry( MM_KERNEL_IMAGE_BASE, 
											 (PCHAR)(AppImageBase + ImportDescriptor[0].Name) );

	if (NULL == ExportDirectory) {
		return FALSE;
	}

	//
	// �õ�ԭʼ��һ�������ݺ͵�һ�������ݵĵ�ַ��
	//
	OriginalFirstThunk = (PULONG)(AppImageBase + ImportDescriptor[0].u.OriginalFirstThunk);
	FirstThunk = (PULONG)(AppImageBase + ImportDescriptor[0].FirstThunk);

	//
	// �����������ݣ�����ԭʼ��������ָ��ĵ������������Ϣ��kernel.dll���ҷ��ŵĵ�ַ��
	// Ȼ���÷��ŵ�ַ�滻��Ӧ�ĵ�һ�������ݡ�
	//
	for (Index = 0; OriginalFirstThunk[Index] != 0; Index++) {

		if (IMAGE_SNAP_BY_ORDINAL(OriginalFirstThunk[Index])) {

			//
			// �����������Һ�����ַ��
			//
			Function = PspFindSymbolByOrdinal( MM_KERNEL_IMAGE_BASE,
											   ExportDirectory,
											   IMAGE_ORDINAL(OriginalFirstThunk[Index]));

		} else {

			//
			// �������Ʋ��Һ�����ַ��
			//
			Function = PspFindSymbolByName( MM_KERNEL_IMAGE_BASE,
											ExportDirectory,
											(PIMAGE_IMPORT_BY_NAME)(AppImageBase + OriginalFirstThunk[Index]));
		}

		if (0 == Function) {
			return FALSE;
		}

		FirstThunk[Index] = Function;
	}

	return TRUE;
}

STATUS
PspLoadProcessImage(
	IN PPROCESS Process,
	IN PSTR ImageName,
	OUT PVOID *ImageBase,
	OUT PVOID *ImageEntry
	)
/*++

����������
	����EOSӦ�ó���Ŀ�ִ���ļ����ڴ档

������
	ImageName -- ��ִ���ļ���ȫ·�����ơ�
	ImageBase -- ������غ�Ļ�ַ��
	ImageEntry -- ������غ�ӳ�����ڵ�ַ��

����ֵ��
	�ɹ����� STATUS_SUCCESS

--*/
{
	STATUS Status;
	HANDLE FileHandle;
	ULONG FilePointer;
	ULONG NumberOfBytesRead;
	PVOID AppImageBase;
	PVOID FileHeaders;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_SECTION_HEADER SectionHeader;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	PVOID SectionAddress;
	SIZE_T VirtualSize;
	ULONG n;

	//
	// �򿪿�ִ���ļ���
	//
	Status = IoCreateFile( ImageName,
						   GENERIC_READ,
						   FILE_SHARE_READ,
						   OPEN_EXISTING,
						   0,
						   &FileHandle );

	if (!EOS_SUCCESS(Status)) {
		return Status;
	}
	
	//
	// ��ǰ�̸߳��ŵ��½����̵ĵ�ַ�ռ���ִ�С�
	//
	PspThreadAttachProcess(Process);

	do {

		FileHeaders = MmAllocateSystemPool(0x400);

		if (NULL == FileHeaders) {
			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// ��ȡPE�ļ���ȫ��ͷ��
		//
		Status = ObRead( FileHandle,
						 FileHeaders,
						 0x400,
						 &NumberOfBytesRead );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// ����ļ�����������ͷ�����ȶ�������˵���ļ���Ч��
		//
		if (NumberOfBytesRead != 0x400) {
			Status = STATUS_INVALID_APP_IMAGE;
			break;
		}

		NtHeaders = PspGetImageNtHeaders(FileHeaders);

		if (NULL == NtHeaders) {
			Status = STATUS_INVALID_APP_IMAGE;
			break;
		}

		//
		// ��֤�Ƿ�EOSӦ�ó���
		//
		if (NtHeaders->OptionalHeader.MajorSubsystemVersion != IMAGE_SUBSYSTEM_EOS_CUI) {
			Status = STATUS_INVALID_APP_IMAGE;
			break;
		}

		//
		// �õ���ִ���ļ��Ļ�ַ�ʹ�С��
		//
		AppImageBase = (PVOID)NtHeaders->OptionalHeader.ImageBase;
		VirtualSize = NtHeaders->OptionalHeader.SizeOfImage;

		//
		// �ڵ�ǰ���̵�ַ�ռ��з��������ڴ档
		//
		Status = MmAllocateVirtualMemory( &AppImageBase,
										  &VirtualSize,
										  MEM_RESERVE,
										  FALSE );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// ӳ���ַ����ҳ���롣
		//
		if (AppImageBase != (PVOID)NtHeaders->OptionalHeader.ImageBase) {
			Status = STATUS_INVALID_APP_IMAGE;
			break;
		}

		//
		// IMAGE_NT_HEADERS֮�������IMAGE_SECTION_HEADER��ɵ����顣
		//
		SectionHeader = (PIMAGE_SECTION_HEADER)(NtHeaders + 1);
		
		for (n = NtHeaders->FileHeader.NumberOfSections; n > 0; n--, SectionHeader++) {

			//
			// ����ڵĴ�СΪ0������������Ϊ���ύ�����ڴ档
			//
			VirtualSize = SectionHeader->Misc.VirtualSize;

			if (0 == VirtualSize) {
				continue;
			}

			SectionAddress = (AppImageBase + SectionHeader->VirtualAddress);

			Status = MmAllocateVirtualMemory( &SectionAddress,
											  &VirtualSize,
											  MEM_COMMIT,
											  FALSE );

			if (!EOS_SUCCESS(Status)) {
				break;
			}

			//
			// �ڿ��Բ�ҳ���룬��һ����ҳ����ġ�
			//
			ASSERT(SectionAddress == (AppImageBase + SectionHeader->VirtualAddress));
			SectionAddress = (AppImageBase + SectionHeader->VirtualAddress);
			VirtualSize = SectionHeader->Misc.VirtualSize;

			//
			// ����ڲ����ļ������������ļ���δ��ʼ�����ݶΣ�ȫ������Ϊ0����
			//
			if (0 == SectionHeader->PointerToRawData) {
				continue;
			}

			ASSERT(SectionHeader->SizeOfRawData != 0);

			//
			// ���ļ�ָ���Ƶ������ļ��е�λ�ã���������ļ�������˵���ļ���Ч��
			//
			Status = IoSetFilePointer( FileHandle,
									   SectionHeader->PointerToRawData,
									   FILE_BEGIN,
									   &FilePointer);

			if (!EOS_SUCCESS(Status)) {
				break;
			}
			
			if (FilePointer != SectionHeader->PointerToRawData) {
				Status = STATUS_INVALID_APP_IMAGE;
				break;
			}

			//
			// ��ȡ�ڵ����ݵ��ڴ��С�
			//
			Status = ObRead( FileHandle,
							 SectionAddress,
							 VirtualSize,
							 &NumberOfBytesRead );

			if (!EOS_SUCCESS(Status)) {
				break;
			}

			if (NumberOfBytesRead != VirtualSize) {
				Status = STATUS_INVALID_APP_IMAGE;
				break;
			}
		}

		//
		// ���潫Ӧ�ó�����ں˽������ӡ�
		//
		n = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

		if (0 != n) {
			if (!PspLinkAppWithKernel(AppImageBase, (PIMAGE_IMPORT_DESCRIPTOR)(AppImageBase + n))) {
				Status = STATUS_SYMBOL_NOT_FOUND;
				break;
			}
		}
		
		//
		// ���÷���ֵ��
		//
		*ImageBase = AppImageBase;
		*ImageEntry = AppImageBase + NtHeaders->OptionalHeader.AddressOfEntryPoint;

		Status = STATUS_SUCCESS;

	} while (0);

	ObCloseHandle(FileHandle);

	//
	// �ͷ�PE�ļ�ͷ������ʹ�á�
	//
	if (NULL != FileHeaders) {
		MmFreeSystemPool(FileHeaders);
	}

	//
	// �������ʧ�����ͷ������ڴ档
	//
	if (!EOS_SUCCESS(Status) && NULL != AppImageBase) {
		VirtualSize = 0;
		MmFreeVirtualMemory(&AppImageBase, &VirtualSize, MEM_RELEASE, FALSE);
	}

	//
	// ��ǰ�̷߳����������̵�ַ�ռ����ִ�С�
	//
	PspThreadAttachProcess(PspCurrentProcess);

	return Status;
}
