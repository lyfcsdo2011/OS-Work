;***
;
; Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。
;
; 只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
; 如果您不接受，不能使用这些代码。
;
; 文件名: loader.asm
;
; 描述: 加载内核。
;
; 
;
;*******************************************************************************/


; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               loader.asm
;
; 系统虚拟内存基址、内核映像基址、页表项基址等引导配置参数。
;
SYSTEM_VIRTUAL_BASE		equ 0x80000000
IMAGE_VIRTUAL_BASE		equ 0x80010000
MAX_IMAGE_SIZE			equ	0x90000
PTE_BASE				equ 0xC0000000
;
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
; 计算机启动时，BIOS 把 512 字节的引导扇区加载到 0000:0x7C00 处并开始执行，然
; 后引导扇区再把 Loader.bin 加载到 0000:0x1000 处并开始执行。
;
	org 0x1000
	jmp	Start

;+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;								数据区域
;
; FAT12引导扇区头。
;
BOOT_ORG				equ 0x7C00			; 引导扇区在内存中的位置
Oem						equ BOOT_ORG + 3
BytesPerSector			equ BOOT_ORG + 11
SectorsPerCluster		equ BOOT_ORG + 13
ReservedSectors			equ BOOT_ORG + 14
Fats					equ BOOT_ORG + 16
RootEntries				equ BOOT_ORG + 17
Sectors					equ BOOT_ORG + 19
Media					equ BOOT_ORG + 21
SectorsPerFat			equ BOOT_ORG + 22
SectorsPerTrack			equ BOOT_ORG + 24
Heads					equ BOOT_ORG + 26
HiddenSectors			equ BOOT_ORG + 28
LargeSectors			equ BOOT_ORG + 32
DriveNumber				equ BOOT_ORG + 36
Reserved				equ BOOT_ORG + 37
Signature				equ BOOT_ORG + 38
Id						equ BOOT_ORG + 39
VolumeLabel				equ BOOT_ORG + 43
SystemId				equ BOOT_ORG + 54  

;
; 文件系统变量定义。
;
FirstSectorOfRootDir	dw 0				; 根目录的起始扇区号
RootDirectorySectors	dw 0				; 根目录占用的扇区数量
FirstSectorOfFileArea	dw 0				; 数据区的起始扇区号
BufferOfFat				dw 0				; FAT 表缓冲区地址
BufferOfRootDir			dw 0				; 根目录缓冲区地址
DirectoryEntry			dw 0				; 目录项指针

;
; 用于定义描述符的宏。
; 用法: Descriptor Base, Limit, Attr
;
%macro Descriptor 3
	dw	%2 & 0xFFFF							; 段界限 1						(2 字节)
	dw	%1 & 0xFFFF							; 段基址 1						(2 字节)
	db	(%1 >> 16) & 0xFF					; 段基址 2						(1 字节)
	dw	((%2 >> 8) & 0x0F00) | (%3 & 0xF0FF); 属性 1 + 段界限 2 + 属性 2	(2 字节)
	db	(%1 >> 24) & 0xFF					; 段基址 3						(1 字节)
%endmacro ; 共 8 字节

;
; 描述符属性定义。
;
DA_32					equ	0x4000			; 32 位段
DA_LIMIT_4K				equ	0x8000			; 段界限粒度为 4K 字节
DA_DRW					equ	0x92			; 存在的可读写数据段属性值
DA_CR					equ	0x9A			; 存在的可执行可读代码段属性值
PG_ATTR					equ 3				; 存在的可读写、可执行的系统页

;
; 全局描述符表，包含了数据段和代码段的描述符。
;
;			描述：			段基址,		段界限,		段属性
GDT:		Descriptor		0,			0,			0								; 空描述符
CS_DESC:	Descriptor		0,			0x0FFFFF,	DA_CR  | DA_32 | DA_LIMIT_4K	; 0 ~ 4G 的代码段
DS_DESC:	Descriptor		0,			0x0FFFFF,	DA_DRW | DA_32 | DA_LIMIT_4K	; 0 ~ 4G 的数据段

;
; 描述符表虚拟地址、大小以及选择子定义。
; 注意：必须紧随在描述符表的定义之下，否则表长计算将会错误。
;
GDT_VA					equ	SYSTEM_VIRTUAL_BASE + GDT	; 全局描述符表的虚拟基址
GDT_SIZE				equ	$ - GDT						; 全局描述符表的长度
CS_SELECTOR				equ	CS_DESC - GDT				; 代码段选择子
DS_SELECTOR				equ	DS_DESC - GDT				; 数据段选择子

;
; 字符串常量定义。
;
szKernelFileName		db	"KERNEL  DLL",0
szNoKernel				db	"File kernel.dll not found!",0
szInvalidFileSize		db	"The file size of kernel.dll must less than 576KB!",0
szLoading				db	"Loading kernel.dll...",0
szInvalidImageSize		db	"The image size of kernel.dll must less than 0x90000!",0
szInvalidImageBase		dd	"Invalid image base address of kernel.dll!",0

;
; LOADER_PARAMETER_BLOCK 结构体定义。
;
PhysicalMemorySize		dd	0
MappedMemorySize		dd	0
SystemVirtualBase		dd	SYSTEM_VIRTUAL_BASE
PageTableVirtualBase	dd	PTE_BASE
FirstFreePageFrame		dd	0
ImageVirtualBase		dd	IMAGE_VIRTUAL_BASE
ImageSize				dd	0
ImageEntry				dd	0

va_LoaderBlock			equ	SYSTEM_VIRTUAL_BASE + PhysicalMemorySize
va_ImageEntry			equ	SYSTEM_VIRTUAL_BASE + ImageEntry
va_PhysicalMemorySize	equ SYSTEM_VIRTUAL_BASE + PhysicalMemorySize

;
;+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;								实模式代码
;
Start:
	;
	; 得到物理内存的大小
	call GetMemorySize
	mov dword [PhysicalMemorySize], eax
	
	;
	; 初始化和 FAT12 文件系统相关的变量
	call InitFAT12

	;
	; 在根目录中查找文件名为 szKernelFileName 的文件，返回文件的目录项指针
	push word szKernelFileName
	call OpenFile
	mov [DirectoryEntry], ax

	;
	; 如果文件不存在，则提示错误并死循环
	cmp ax, 0
	jne .FILE_EXIST
	push word szNoKernel
	call TextOut
	jmp $

	; 
	; 检查文件的大小，如果大小不在区间 (0，MAX_IMAGE_SIZE]
	; 之内，则提示错误并死循环
.FILE_EXIST:
	mov bx, ax
	mov ecx, dword [bx + 0x1C]
	cmp ecx, 0
	je .INVALID_FILE_SIZE
	cmp ecx, MAX_IMAGE_SIZE
	jbe .VALID_FILE_SIZE
	
.INVALID_FILE_SIZE:
	push word szInvalidFileSize
	call TextOut
	jmp $
	
	;
	; 加载内核文件到其虚拟基址对应的物理内存中
.VALID_FILE_SIZE:
	push dword szLoading
	call TextOut
	push word 0
	push word (IMAGE_VIRTUAL_BASE - SYSTEM_VIRTUAL_BASE) >> 4
	push word [DirectoryEntry]
	call ReadFile
	
	;
	; 检查内核映像的虚拟基址及映像大小是否符合约定，Loader 不支持内核重定位。
	;
	; 先使 es:bx 指向 IMAGE_NT_HEADER 结构体
	;
	mov	ax, (IMAGE_VIRTUAL_BASE - SYSTEM_VIRTUAL_BASE) >> 4
	mov	es, ax								; es <- BaseOfKernelFile
	mov	bx, [es:0 + 0x3C]					; bx = IMAGE_DOS_HEADER::e_lfanew，即 IMAGE_NT_HEADER 的段内偏移地址	
	
	;
	; 如果基址不符合约定要求则提示错误并死循环
	;
	mov eax, [es:bx + 0x34]					; eax = IMAGE_OPTIONAL_HEADER::ImageBase
	cmp eax, IMAGE_VIRTUAL_BASE
	je	.VALID_IMAGE_BASE
	push word szInvalidImageBase
	call TextOut
	jmp $
	
	;
	; 如果内核映像大小超过约定最大值则提示错误并死循环。
	;
.VALID_IMAGE_BASE:
	mov eax, [es:bx + 0x50]					; eax = IMAGE_OPTIONAL_HEADER::SizeOfImage
	mov [ImageSize], eax					; ImageSize = eax
	cmp eax, MAX_IMAGE_SIZE
	jbe .VALID_IMAGE_SIZE
	push word szInvalidImageSize
	call TextOut
	jmp $

	;
	; 获取映像的入口地址
.VALID_IMAGE_SIZE:							
	mov	eax, [es:bx + 0x28]					; eax = IMAGE_OPTIONAL_HEADER::AddressOfEntryPoint
	add eax, IMAGE_VIRTUAL_BASE
	mov [ImageEntry], eax

	;
	; 下面准备跳入保护模式
	cli
	
	;
	; 加载全局描述符表
	push dword GDT
	push word GDT_SIZE
	movzx eax, sp
	lgdt [eax]
	add sp, 6
	
	;
	; 打开地址线 A20
	in	al, 0x92
	or	al, 0x02
	out	0x92, al
	
	;
	; 设置 cr0 的保护标志位
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax
	
	;
	; 跳转执行保护模式代码
	jmp	dword CS_SELECTOR:ProtectionMode

;----------------------------------------------------------------------------
; 函数名: void TextOut(char* Text)
; 作  用: 显示一个字符串
;----------------------------------------------------------------------------
TextOut:
;{
	push bp
	mov bp, sp
	
	; 计算字符串的长度
	xor cx, cx
	mov di, word [bp + 4]
.LOOP:
	cmp byte [di], 0
	je .DO_BIOS_CALL
	inc di
	inc cx
	jmp .LOOP
	
.DO_BIOS_CALL:
	mov bp, [bp + 4]
	mov ax, 0x1301
	mov bx, 0x07
	mov dl, 0
	int 0x10
	
	pop bp
	ret 2
;}


;----------------------------------------------------------------------------
; 函	数：DWORD GetMemorySize()
; 作	用：返回物理内存的大小
;----------------------------------------------------------------------------
GetMemorySize:
;{
	push bp
	mov bp, sp
	
	; 分配堆栈变量
	sub sp, 4					; 记录物理内存最高地址的变量
	sub sp, 20					; 地址范围描述符结构体（Address Range Descriptor Structure）变量
	
	xor eax,eax
	mov dword [bp - 4], eax
	mov	di, sp					; es:di 指向地址范围描述符结构体
	mov	ebx, 0					; ebx = 后续值, 开始时需为 0

.LOOP:
	mov	eax, 0xE820				; eax = 0xE820
	mov	ecx, 20					; ecx = 地址范围描述符结构的大小
	mov	edx, 0x534D4150			; edx = 'SMAP'
	int	0x15
	cmp	dword [es:di + 16], 1	; 检查是否是可使用内存
	jne	.CONTINUE				; 不可使用块，不保存信息
	
	mov	eax, [es:di]			; 可用内存区域的基址
	add	eax, [es:di + 8]		; 可用内存区域的结束地址 = 基址 + 长度
	cmp	eax, [bp - 4]			;
	jbe	.CONTINUE				;
	
	mov	[bp - 4], eax			;
	
.CONTINUE:
	cmp ebx, 0
	jne	.LOOP
	
	mov eax, [bp - 4]			; 设置返回值
	leave						; 恢复调用前的堆栈帧
	ret
;}
	
;----------------------------------------------------------------------------
; 函	数：VOID InitFAT12()
; 作	用：初始化文件系统参数
;----------------------------------------------------------------------------
InitFAT12:
;{
	;
	; 计算根目录的起始扇区号
	; FirstSectorOfRootDir = ReservedSectors + SectorsPerFat * Fats
	;
	mov ax, word [SectorsPerFat]
	movzx bx, byte [Fats]
	mul bx
	add ax, word [ReservedSectors]
	mov word [FirstSectorOfRootDir], ax
	
	;
	; 计算根目录占用的扇区数量
	; RootDirectorySectors = RootEntries * 32 / BytesPerSector
	;
	mov ax, word [RootEntries]
	shl ax, 5
	mov bx, word [BytesPerSector]
	div bx
	mov word [RootDirectorySectors], ax
	
	;
	; 计算数据区域的起始扇区号
	; FirstSectorOfFileArea = FirstSectorOfRootDir + RootDirectorySectors
	;
	add ax, word [FirstSectorOfRootDir]
	mov word [FirstSectorOfFileArea], ax
	
	;
	; 计算 FAT 缓冲区地址（紧接在引导扇区后）
	; BufferOfFat = BOOT_ORG + BytesPerSector * ReservedSectors
	;
	mov ax, word [BytesPerSector]
	mul word [ReservedSectors]
	add ax, BOOT_ORG
	mov word [BufferOfFat], ax
	
	;
	; 计算根目录缓冲区地址（紧接在 FAT 缓冲区后）
	; BufferOfRootDir = BufferOfFat + BytesPerSector * SectorsPerFat
	;
	mov ax, word [BytesPerSector]
	mul word [SectorsPerFat]
	add ax, word [BufferOfFat]
	mov word [BufferOfRootDir], ax
	
	ret
;}

;----------------------------------------------------------------------------
; 函	数：DIR_ENTRY* OpenFile(char* pszFileName)
; 作	用：在根目录中查找文件，并返回文件的目录项指针
;----------------------------------------------------------------------------
OpenFile:
;{
	push bp
	mov bp, sp
	sub sp, 2							; 堆栈变量，根目录缓冲区的地址界限
	
	mov ax, 0x20						; ax = sizeof(DIR_ENTRY)
	mul word [RootEntries]
	mov	bx, word [BufferOfRootDir]		; bx 指向第一个根目录项
	add ax, bx
	mov [bp - 2], ax
	
.while:
	mov	si, word [bp + 4]				; si -> pszFileName
	mov di, bx							; di -> 目录项中文件名字符串
	mov	cx, 11							; 文件名字符串的长度
	repe cmpsb							; 字符串比较
	cmp	cx, 0
	je	.return_bx						; 如果比较了 11 个字符都相等, 表示找到文件
	
	add bx, 0x20
	cmp bx, word [bp - 2]
	jb .while
	
.return_0:
	xor ax, ax	; 查找失败，返回 0
	jmp .ret
.return_bx:
	mov ax, bx	; 查找成功，返回目录项
.ret:
	leave
	ret 2
;}

;----------------------------------------------------------------------------
; 函    数: ReadSector(WORD wSector, WORD wCount, WORD wBase, WORD wOffset)
; 作    用: 从第 wSector 个扇区开始, 将 wCount(1~255) 个扇区读入 wBase:wOffset 中
;----------------------------------------------------------------------------
ReadSector:
;{
	push bp
	mov	bp, sp
	push es
	
	;
	; 计算 柱面号、起始扇区 和 磁头号
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数     │
	;                   └ 余 z => 起始扇区号 = z + 1
	;
	mov ax, [bp + 4]			; ax = wSector
	mov	bl, [SectorsPerTrack]	; bl: 除数
	div	bl						; y 在 al 中, z 在 ah 中
	inc	ah						; z ++
	mov	cl, ah					; cl <- 起始扇区号
	mov	dh, al					; dh <- y
	shr	al, 1					; y >> 1 (其实是 y / Heads, 这里 Heads = 2)
	mov	ch, al					; ch <- 柱面号
	and	dh, 1					; dh & 1 = 磁头号
	mov	dl, [DriveNumber]		; 驱动器号 (0 表示 A 盘)
	
	
	mov ax, [bp + 8]
	mov es, ax
	mov bx, [bp + 10]
.GoOnReading:
	mov	ah, 2					; 读
	mov	al, [bp + 6]			; 读 al 个扇区
	int	0x13
	jc	.GoOnReading			; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	; 恢复堆栈并返回
	pop es
	leave
	ret 8
;}

;----------------------------------------------------------------------------
; 函    数: WORD GetNextCluster(WORD wCurrentCluster)
; 作    用: 找到同文件中 wCurrentCluster 的后继簇
;----------------------------------------------------------------------------
GetNextCluster:
;{
	push bp
	mov bp, sp
	
	mov ax, word[bp + 4]
	mov bx, 3
	mul bx
	mov bx, 2
	div bx
	mov bx, word [BufferOfFat]
	add bx, ax
	mov ax, word [bx]
	cmp dx, 0
	jz .else
	shr	ax, 4
	jmp .return
.else:
	and ax, 0x0FFF

.return:
	leave
	ret 2
;}

;----------------------------------------------------------------------------
; 函	数：VOID ReadFile(DIR_ENTRY* pFile, WORD wBufferBase, WORD wBufferOffset)
; 作	用：读取指定文件到指定的位置
;----------------------------------------------------------------------------
ReadFile:
;{
	push bp
	mov bp, sp
	; 分配堆栈变量，初始化为文件的第一个簇号
	mov bx, word [bp + 4]
	push word [bx + 0x1A]
	; 分配堆栈变量，初始化为每个簇的字节数
	mov ax, word [BytesPerSector]
	movzx bx, byte [SectorsPerCluster]
	mul bx
	push ax
	 
	; 软驱复位
	xor	ah, ah
	xor	dl, dl
	int	0x13
			
.while:
	; 如果文件已经结束则停止循环
	mov ax, word [bp - 2]
	cmp ax, 0x0FF7
	ja	.return
	
	; 计算簇号对应的扇区号：数据区起始扇区号 + 每簇扇区数 * （簇号 - 2）
	sub ax, 2
	movzx cx, byte [SectorsPerCluster]
	mul	cx
	add ax, word [FirstSectorOfFileArea]
	
	; 读取一个簇到缓冲区中
	push word [bp + 8]
	push word [bp + 6]
	push cx
	push ax
	call ReadSector
	
	; 根据当前簇号得到下个簇号
	push word [bp - 2]
	call GetNextCluster
	mov word [bp - 2], ax
	
	; 文件位置向后移动一个簇的大小。如果超过了段边界，需要向后移动段基址
	mov ax, word [bp - 4]
	add word [bp + 8], ax
	jnz .while
	add word[bp + 6], 0x1000
	jmp .while
	
.return:
	; 关闭软驱马达
	mov	dx, 0x03F2
	xor al, al
	out	dx, al
	; 恢复堆栈
	leave
	ret 6
;}

;
;+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;								保护模式代码
[SECTION .s32]
ALIGN	32
[BITS	32]

ProtectionMode:
	mov	ax, DS_SELECTOR
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax
	mov esp, 0x10000
	mov ebp, esp

	;
	; 计算要映射的物理内存的大小（物理内存的1/8，向上对齐到4M边界且不超过256MB）。
	;
	mov eax, [PhysicalMemorySize]
	cmp eax, 0x80000000
	jb .DIVIDE

	mov dword [MappedMemorySize], 0x20000000
	jmp .INIT_PDE

.DIVIDE:
	shr eax, 3					; eax = eax/8
	add eax, (0x400000 - 1)
	and eax, ~(0x400000 - 1)
	mov [MappedMemorySize], eax

	;
	; 对全部页目录项进行零初始化。
	;
.INIT_PDE:
	xor eax, eax
	mov edi, [MappedMemorySize]
	mov ecx, 1024
.LOOP_1:
	stosd
	dec ecx
	jnz .LOOP_1

	;
	; 设置用于映射内存的页表项，页表跟在页目录后面。
	;
	mov	eax, 0 | PG_ATTR		; 页表项内容
	mov ecx, [MappedMemorySize]
	shr ecx, 12
.LOOP_2:
	stosd
	add	eax, 4096				; 下一页被映射的物理内存的地址
	dec ecx
	jnz .LOOP_2
	
	;
	; 紧接着前面的页表再初始化一张空页表。
	;
	xor eax, eax
	mov ecx, 1024
.LOOP_3:
	stosd
	dec ecx
	jnz .LOOP_3

	;
	; 页表后面的内存是自由内存。
	;
	shr edi, 12
	mov [FirstFreePageFrame], edi

	;
	; 设置页目录项，映射物理内存。
	;

	mov eax, [MappedMemorySize]
	mov ecx, eax
	shr ecx, 22					; ecx等于映射内存的页目录项数
	mov edi, eax				; edi指向第一个页目录项
	add eax, 4096				; eax指向第一张页表
	or eax, PG_ATTR

	;
	; 将0-4MB物理内存映射到虚拟地址0-4MB上。
	;
	mov [edi], eax

	;
	; 将0-MappedMemorySize物理内存映射到虚拟地址SYSTEM_VIRTUAL_BASE处。
	;
	add edi, SYSTEM_VIRTUAL_BASE >> 20	; edi指向系统起始地址对应的页目录项
.LOOP_4:
	stosd
	add	eax, 4096						; 下一个页表的地址
	dec ecx
	jnz .LOOP_4
	
	;
	; 此时eax还指向多初始化的一张页表，将之设置为页表空间后的4M空间的页表。
	;
	mov edi, [MappedMemorySize]
	add edi, PTE_BASE >> 20
	mov [edi+4], eax
	
	;
	; 将所有页表映射到页表空间，用页目录充当映射页表的页表。
	;
	mov eax, [MappedMemorySize]
	or eax, PG_ATTR
	mov [edi], eax

	;
	; 启动分页机制
	;
	mov	eax, [MappedMemorySize]
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 0x80000000
	mov	cr0, eax
	jmp	dword CS_SELECTOR:(SYSTEM_VIRTUAL_BASE+.NOP)	; 跳转到虚拟地址上执行
.NOP:
	nop
	
	;
	; 调整栈指针，从已映射内存的最高处向下增长。
	;
	mov esp, [MappedMemorySize]
	add esp, SYSTEM_VIRTUAL_BASE
	mov ebp, esp

	;
	; 重新加载 GDT，使 GDTR 指向 GDT 的虚拟地址
	;
	push dword GDT_VA
	push word GDT_SIZE
	lgdt [esp]
	add esp, 6

	;
	; 关闭虚拟地址 0~4M 对物理内存 0~4M 的映射。
	;
	mov dword [PTE_BASE + (PTE_BASE>>10)], 0
	mov eax, cr3
	mov cr3, eax	; 刷新快表

	;
	; 初始化内核镜像，将映像内的节对齐到各自所需位置。
	;
	push dword IMAGE_VIRTUAL_BASE
	call InitKernelImage
	
	;
	; 进入内核
	;
	push dword va_LoaderBlock
	call dword [va_ImageEntry]
	
;----------------------------------------------------------------------------
; 函	数：VOID MemCopy(DWORD *pDst, DWORD *pSrc, DWORD dwCountOfDWORD)
; 作	用：按双字单位进行内存复制。
;----------------------------------------------------------------------------
MemCopy:
;{
	push ebp
	mov ebp, esp
	
	mov edi, [ebp + 8]
	mov esi, [ebp + 12]
	mov ecx, [ebp + 16]
	mov eax, ecx
	dec eax
	shl eax, 2
	add edi, eax
	add esi, eax

.LOOP:
	cmp ecx, 0
	je .BREAK
	
	mov eax, [esi]
	mov [edi], eax
	sub esi, 4
	sub edi, 4
	dec ecx
	jmp .LOOP
.BREAK:

	leave
	ret 12
;}

;----------------------------------------------------------------------------
; 函	数：VOID MemClear(DWORD *pDst, DWORD dwCountOfDWORD)
; 作	用：按双字单位进行内存清零。
;----------------------------------------------------------------------------
MemClear:
;{
	push ebp
	mov ebp, esp
	
	xor eax, eax
	mov edi, [ebp + 8]
	mov ecx, [ebp + 12]
.LOOP:
	cmp ecx, 0
	je .BREAK
	stosd
	dec ecx
	jmp .LOOP
.BREAK:

	leave
	ret 8
;}
	
;----------------------------------------------------------------------------
; 函	数：VOID InitKernelImage(DWORD dwImageBase)
; 作	用：将文件对齐的映像展开为节对齐。
;----------------------------------------------------------------------------
InitKernelImage:
;{
	push ebp
	mov ebp, esp
	sub esp, 8

	dwSections		equ -4
	pSectionHeader	equ -8	

	mov ecx, [ebp + 8]					;ecx = dwImageBase = &IMAGE_DOS_HEADER
	mov eax, [ecx + 0x3C]				;eax = IMAGE_DOS_HEADER::e_lfanew
	add eax, ecx						;eax = &IMAGE_NT_HEADERS = dwImageBase + IMAGE_DOS_HEADER::e_lfanew
	mov ecx, eax						;ecx =  &IMAGE_NT_HEADERS
	xor ebx, 0
	mov WORD bx, [ecx + 0x06]			;ebx = IMAGE_FILE_HEADER::NumberOfSections
	mov [ebp + dwSections], ebx			;dwSections = IMAGE_FILE_HEADER::NumberOfSections - 1
	mov eax, 0x28						;eax = sizeof(IMAGE_SECTION_HEADER)
	mul ebx								;eax *= dwSections
	sub eax, 0x28
	add eax, 0xF8						;eax += sizeof(IMAGE_NT_HEADERS)
	add eax, ecx						;eax += &IMAGE_NT_HEADERS
	mov [ebp + pSectionHeader], eax		;pSectionHeader = eax, Address of last section header
	
.LOOP:
	cmp dword [ebp + dwSections], 0		; while(dwSections != 0)
	je .BREAK

	mov ecx, [ebp + pSectionHeader]
	mov eax, [ecx + 0x08]				;eax = pSectionHeader->VirtualSize
	cmp eax, 0							;if(eax == 0)
	je	.CONTINUE						;	continue

	mov edi, [ecx + 0x0C]				;edi = pSectionHeader->VirtualAddress
	mov esi, [ecx + 0x14]				;esi = pSectionHeader->PointerToRawData
	cmp esi, edi						;if(edi == esi)
	je	.CONTINUE						;	continue
	
	add eax, 3							;eax += 3;
	shr eax, 2							;eax /= 4;
	cmp esi, 0							;if(esi == 0) MemClear() else MemCopy()
	jne	.MEM_COPY

	push eax
	add edi, dword [ebp + 8]
	push edi
	call MemClear
	jmp .CONTINUE

.MEM_COPY:
	push eax
	mov eax, [ebp + 8]					;┓
	add esi, eax
	push esi							;
	add edi, eax
	push edi							;┣	MemCopy(edi, esi, eax)
	call MemCopy						;┛
	
.CONTINUE:
	sub dword [ebp + pSectionHeader], 0x28	; pSectionHeader --
	dec dword [ebp + dwSections]			; wNumberOfSections --
	jmp .LOOP
	
.BREAK:
	leave
	ret 4
;}
