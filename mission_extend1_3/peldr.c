/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: peldr.c

描述: PE 文件的加载。



*******************************************************************************/

#include "psp.h"
#include "io.h"

PRIVATE
PIMAGE_NT_HEADERS
PspGetImageNtHeaders(
	IN PVOID ImageHeader
	)
/*++

功能描述：
	检查PE文件头部的各个标志，确定是否有效的PE文件，如果是则返回PE文件头部
	IMAGE_NT_HEADERS结构体的指针。

参数：
	ImageHeader -- PE文件头部。

返回值：
	PE文件头部中的IMAGE_NT_HEADERS结构体指针。

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

功能描述：
	得到映像的导出目录。

参数：
	ImageBase -- 映像被加载的基址。
	ImageName -- 映像的名称。

返回值：
	如果映像的名称符合要求则返回映像的导出目录，否则返回NULL。

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

功能描述：
	根据导出函数的序数得到导出函数的地址。

参数：
	ImageBase -- 导出函数的映像的加载基址。
	ExportDirectory -- 导出函数的映像的导出目录。
	Ordinal -- 导出函数的序数。

返回值：
	如果序数有效则返回对应的函数地址，否则返回0。

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

功能描述：
	根据导出函数的符号名称得到函数的地址。

参数：
	ImageBase -- 导出函数的映像的加载基址。
	ExportDirectory -- 导出函数的映像的导出目录。
	Name -- 导出函数的符号名称信息。

返回值：
	如果存在指定名称的导出函数则返回函数地址，否则返回0。

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
	// 先按照提示名字提示检查符号名称，如果命中最好不过。
	//
	if (Name->Hint < ExportDirectory->NumberOfNames) {

		if (0 == strcmp((PCHAR)(ImageBase + NameArray[Name->Hint]), Name->Name)) {

			Index = NameOrdinalArray[Name->Hint];

			return (ULONG)ImageBase + FunctionArray[Index];
		}
	}

	//
	// 提示没有命中，下面需要遍历名称数组查找指定的符号。
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

功能描述：
	将加载到内存中的EOS应用程序和内核进行连接。

参数：
	AppImageBase -- 应用程序的加载基址。
	ImportDescriptor -- 映像的第一个导入描述符的指针。

返回值：
	如果所有符号都连接成功则返回TRUE，否则返回FALSE。

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
	// 如果没有导入描述符则无需链接。
	//
	if (0 == ImportDescriptor[0].u.Characteristics) {
		return TRUE;	
	}
	
	//
	// 目前仅仅和kernel.dll链接，所以导入描述符应该只有一个。
	//
	if(0 != ImportDescriptor[1].u.Characteristics) {
		return FALSE;
	}

	//
	// 得到kernel.dll的导出目录。
	//
	ExportDirectory = PspGetExportDirecotry( MM_KERNEL_IMAGE_BASE, 
											 (PCHAR)(AppImageBase + ImportDescriptor[0].Name) );

	if (NULL == ExportDirectory) {
		return FALSE;
	}

	//
	// 得到原始第一换长数据和第一换长数据的地址。
	//
	OriginalFirstThunk = (PULONG)(AppImageBase + ImportDescriptor[0].u.OriginalFirstThunk);
	FirstThunk = (PULONG)(AppImageBase + ImportDescriptor[0].FirstThunk);

	//
	// 遍历换长数据，根据原始换长数据指向的导入符号名称信息在kernel.dll查找符号的地址，
	// 然后用符号地址替换对应的第一换长数据。
	//
	for (Index = 0; OriginalFirstThunk[Index] != 0; Index++) {

		if (IMAGE_SNAP_BY_ORDINAL(OriginalFirstThunk[Index])) {

			//
			// 根据序数查找函数地址。
			//
			Function = PspFindSymbolByOrdinal( MM_KERNEL_IMAGE_BASE,
											   ExportDirectory,
											   IMAGE_ORDINAL(OriginalFirstThunk[Index]));

		} else {

			//
			// 根据名称查找函数地址。
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

功能描述：
	加载EOS应用程序的可执行文件到内存。

参数：
	ImageName -- 可执行文件的全路径名称。
	ImageBase -- 输出加载后的基址。
	ImageEntry -- 输出加载后映像的入口地址。

返回值：
	成功返回 STATUS_SUCCESS

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
	// 打开可执行文件。
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
	// 当前线程附着到新建进程的地址空间中执行。
	//
	PspThreadAttachProcess(Process);

	do {

		FileHeaders = MmAllocateSystemPool(0x400);

		if (NULL == FileHeaders) {
			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// 读取PE文件的全部头。
		//
		Status = ObRead( FileHandle,
						 FileHeaders,
						 0x400,
						 &NumberOfBytesRead );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// 如果文件长度连基本头部长度都不够，说明文件无效。
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
		// 验证是否EOS应用程序。
		//
		if (NtHeaders->OptionalHeader.MajorSubsystemVersion != IMAGE_SUBSYSTEM_EOS_CUI) {
			Status = STATUS_INVALID_APP_IMAGE;
			break;
		}

		//
		// 得到可执行文件的基址和大小。
		//
		AppImageBase = (PVOID)NtHeaders->OptionalHeader.ImageBase;
		VirtualSize = NtHeaders->OptionalHeader.SizeOfImage;

		//
		// 在当前进程地址空间中分配虚拟内存。
		//
		Status = MmAllocateVirtualMemory( &AppImageBase,
										  &VirtualSize,
										  MEM_RESERVE,
										  FALSE );

		if (!EOS_SUCCESS(Status)) {
			break;
		}

		//
		// 映像基址必须页对齐。
		//
		if (AppImageBase != (PVOID)NtHeaders->OptionalHeader.ImageBase) {
			Status = STATUS_INVALID_APP_IMAGE;
			break;
		}

		//
		// IMAGE_NT_HEADERS之后就是由IMAGE_SECTION_HEADER组成的数组。
		//
		SectionHeader = (PIMAGE_SECTION_HEADER)(NtHeaders + 1);
		
		for (n = NtHeaders->FileHeader.NumberOfSections; n > 0; n--, SectionHeader++) {

			//
			// 如果节的大小为0则跳过，否则为节提交物理内存。
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
			// 节可以不页对齐，但一般是页对齐的。
			//
			ASSERT(SectionAddress == (AppImageBase + SectionHeader->VirtualAddress));
			SectionAddress = (AppImageBase + SectionHeader->VirtualAddress);
			VirtualSize = SectionHeader->Misc.VirtualSize;

			//
			// 如果节不在文件中则跳过读文件（未初始化数据段，全部数据为0）。
			//
			if (0 == SectionHeader->PointerToRawData) {
				continue;
			}

			ASSERT(SectionHeader->SizeOfRawData != 0);

			//
			// 将文件指针移到节在文件中的位置，如果超出文件长度则说明文件无效。
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
			// 读取节的内容到内存中。
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
		// 下面将应用程序和内核进行连接。
		//
		n = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

		if (0 != n) {
			if (!PspLinkAppWithKernel(AppImageBase, (PIMAGE_IMPORT_DESCRIPTOR)(AppImageBase + n))) {
				Status = STATUS_SYMBOL_NOT_FOUND;
				break;
			}
		}
		
		//
		// 设置返回值。
		//
		*ImageBase = AppImageBase;
		*ImageEntry = AppImageBase + NtHeaders->OptionalHeader.AddressOfEntryPoint;

		Status = STATUS_SUCCESS;

	} while (0);

	ObCloseHandle(FileHandle);

	//
	// 释放PE文件头，不再使用。
	//
	if (NULL != FileHeaders) {
		MmFreeSystemPool(FileHeaders);
	}

	//
	// 如果加载失败则释放虚拟内存。
	//
	if (!EOS_SUCCESS(Status) && NULL != AppImageBase) {
		VirtualSize = 0;
		MmFreeVirtualMemory(&AppImageBase, &VirtualSize, MEM_RELEASE, FALSE);
	}

	//
	// 当前线程返回所属进程地址空间继续执行。
	//
	PspThreadAttachProcess(PspCurrentProcess);

	return Status;
}
