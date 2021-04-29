;***
;
; Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。
;
; 只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
; 如果您不接受，不能使用这些代码。
;
; 文件名: boot.asm
;
; 描述: 引导扇区。
;
; 
;
;*******************************************************************************/

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               boot.asm
;
;     PC 机加电后，CPU 进入实模式，分段管理内存，最多访问 1M 地址空间（没
; 有打开 A20 的情况下）。CPU 首先执行 BIOS 程序，在 BIOS 完成设备检测等工
; 作后，如果 BIOS 被设置为从软盘启动，则 BIOS 会将软盘的引导扇区（512 字节）
; 加载到物理地址 0x7C00 - 0x7DFF 处，然后将 CPU 的 CS 寄存器设置为 0x0000,
; 将 IP 寄存器设置为 0x7C00，接下来 CPU 就开始执行引导扇区中的程序。
;     由于段界限为 64K，所以在不修改段寄存器的情况下只能访问 0x0000 到 0xFFFF
; 的地址空间，软盘引导扇区就被加载到了此范围内，所以在软盘引导扇区程序中一般
; 不需要修改段寄存器。
;     此时的物理内存应该是下面的样子：
;
;                 +-------------------------------------+----------------------
;          0x0000 |                                     |
;                 |   BIOS 中断向量表 (1K)              |
;                 |   BIOS Interrupt Vector Table       |
;                 |                                     |
;                 +-------------------------------------+
;          0x0400 |   BIOS 数据区 (512 Bytes)           |
;                 |   BIOS Data Area                    |
;                 +-------------------------------------+
;          0x0600 |                                     |
;                 |                                     |
;                 |             用户可用(1)             |   常规内存 (640K)
;                 |                                     |  Conventional Memory
;                 |                                     |
;                 +-------------------------------------+
;          0x7C00 |   软盘引导扇区 (512 Bytes)          |
;                 |   Floppy Boot Sector                |
;                 +-------------------------------------+
;          0x7E00 |                                     |
;                 |                                     |
;                 |             用户可用(2)             |
;                 |                                     |
;                 |                                     |
;                 +-------------------------------------+----------------------
;         0xA0000 |                                     |
;                 |                                     |
;                 |   系统占用 (384K)                   |   上位内存 (384K)
;                 |                                     |   Upper Memory
;                 |                                     |
;                 +-------------------------------------+----------------------
;        0x100000 |                                     |
;                 |                                     |   扩展内存（只有进入保护模式才能访问）
;                 |               不可用                |  Extended Memory
;                 Z                                     Z
;                 |                                     |
;    物理内存结束 |                                     |
;                 +-------------------------------------+----------------------
;
;     EOS 的软盘引导扇区程序选择将 Loader.bin 从第一个用户可用区域的 0x1000 处开始
; 加载，即从 0x1000 到 0x7BFF，所以 Loader 最大只能为 0x7C00 - 0x1000 = 0x6C00
; 个字节。如果在保护模式中按照 4K 大小进行分页，则 Loader 就在一个页面的开始处。
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	org 0x7C00
	jmp short Start
	nop					; 这个 nop 不可少

; ----------------------------------------------------------------------
; FAT12 引导扇区头
Oem						db 'Engintim'	; OEM String，必须 8 个字节
BytesPerSector			dw 512			; 每扇区字节数					----+
SectorsPerCluster		db 1			; 每簇多少扇区						|
ReservedSectors			dw 1			; Boot 记录占用多少扇区				|
Fats					db 2			; FAT 表数							|
RootEntries				dw 224			; 根目录文件数最大值				|
Sectors					dw 2880			; 扇区总数							\\ BPB
Media					db 0xF0			; 介质描述							// BIOS Parameter Block
SectorsPerFat			dw 9			; 每 FAT 扇区数						|
SectorsPerTrack			dw 18			; 每磁道扇区数						|
Heads					dw 2			; 磁头数							|
HiddenSectors			dd 0			; 隐藏扇区数						|
LargeSectors			dd 0			; 扇区总数，Sectors 为 0 时使用	----+
DriveNumber				db 0			; 驱动器号
Reserved				db 0			; 保留未用
Signature				db 0x29			; 引导标记 (0x29)
Id						dd 0			; 卷序列号
VolumeLabel				db 'EOS        '; 卷标，必须 11 个字节
SystemId				db 'FAT12   '	; 文件系统类型，必须 8 个字节
;------------------------------------------------------------------------

; FAT12 文件系统相关的一些变量
FirstSectorOfRootDir	dw 0			; 根目录的起始扇区号
RootDirectorySectors	dw 0			; 根目录占用的扇区数量
FirstSectorOfFileArea	dw 0			; 数据区的起始扇区号
BufferOfFat				dw 0			; FAT 表缓冲区地址
BufferOfRootDir			dw 0			; 根目录缓冲区地址

LOADER_ORG				equ	0x1000				; Loader.bin 的起始地址
MAX_FILE_SIZE			equ 0x6C00				; Loader.bin 只占用 0x1000 到 0x7C00 的空间
wFilePos				dw	LOADER_ORG			; 用于加载 Loader.bin 的游标
LoaderFileName			db	"LOADER  BIN"		; Loader.bin 的文件名
strError:				db	"File Loader.bin not found!"

Start:
	; 初始化 CPU 的段寄存器为 CS 的值(0)，堆栈从 64K 向下增长
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	xor sp, sp
	mov bp, sp

	; 初始化屏幕
	mov	ax, 0x0600		; AH = 0x06,  AL = 0x00
	mov	bx, 0x0700		; 黑底白字(BH = 0x07)
	xor cx, cx			; 左上角: (列号  0, 行号  0)
	mov	dx, 0x184F		; 右下角: (列号 79, 行号 24)
	int	0x10
	
	; 软驱复位
	xor	ah, ah
	xor	dl, dl
	int	0x13
	
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
	; BufferOfFat = 0x7C00 + BytesPerSector * ReservedSectors
	;
	mov ax, word [BytesPerSector]
	mul word [ReservedSectors]
	add ax, 0x7C00
	mov word [BufferOfFat], ax
	
	;
	; 计算根目录缓冲区地址（紧接在 FAT 缓冲区后）
	; BufferOfRootDir = BufferOfFat + BytesPerSector * SectorsPerFat
	;
	mov ax, word [BytesPerSector]
	mul word [SectorsPerFat]
	add ax, word [BufferOfFat]
	mov word [BufferOfRootDir], ax
	
	; 将 FAT1 读入 FAT 缓冲区
	mov ax, word [ReservedSectors]		; 
	mov cx, word [SectorsPerFat]		; 一个 FAT 表的扇区数量
	mov bx, word [BufferOfFat]			; es:bx 指向 FAT 表缓冲区
	call ReadSector
	
	; 将根目录读入缓冲区
	mov ax, word[FirstSectorOfRootDir]
	mov cx, word[RootDirectorySectors]
	mov bx, word[BufferOfRootDir]
	call ReadSector
	
	; 在根目录中查找 Loader.bin 文件
FindFile:
	mov	bx, word [BufferOfRootDir]		; bx 指向第一个根目录项
	mov dx, word [RootEntries]			; 根目录项总数
	cld

CompareNextDirEntry:
	mov	si, LoaderFileName				; si -> "LOADER  BIN"
	mov di, bx							; di -> 目录项中文件名字符串
	mov	cx, 11							; 文件名字符串的长度
	repe cmpsb							; 字符串比较
	cmp	cx, 0
	je	CheckFileSize					; 如果比较了 11 个字符都相等, 表示找到文件
	
	; 文件名不一致，继续比较下一个目录项
	add bx, 0x20						; bx 指向下一个目录项
	dec dx								; 减小剩余目录项
	jnz CompareNextDirEntry
	
	; 查找完所有目录项仍没有找到文件，提示出错
	jmp	Error

	; 找到文件后，检查文件的大小
CheckFileSize:
	mov eax, dword [bx + 0x1C]			; 得到文件的大小
	test eax, eax
	jz Error
	cmp eax, MAX_FILE_SIZE
	ja Error
	
	; 开始加载文件
	mov	ax, word [bx + 0x1A]			; 初始化 ax 为文件的第一个簇号
ReadNextCluster:
	push ax								; 保存要读取的簇号
	
	;
	; 计算 ax 号簇对应的扇区号，扇区号 = 数据区起始扇区号 + （簇号 - 2） * 每簇扇区数
	;
	sub ax, 2
	movzx cx, byte [SectorsPerCluster]
	mul	cx
	add ax, word [FirstSectorOfFileArea]
	
	mov bx, word [wFilePos];			; 文件缓冲区地址
	
	call ReadSector						; 读一个簇
	
	;
	; 文件位置向后移动一个簇的大小
	; wFilePos = wFilePos + BytesPerSector * SectorsPerCluster
	;
	mov ax, word [BytesPerSector]
	movzx bx, byte [SectorsPerCluster]
	mul bx
	add ax, word [wFilePos];
	mov word [wFilePos], ax		
	
	; 查找 FAT 表，获得下一个要读取的簇
	pop ax								; 刚读取的簇号
	mov bx, 3
	mul bx
	mov bx, 2
	div bx
	mov bx, word [BufferOfFat]
	add bx, ax
	mov ax, word [bx]
	test dx, dx
	jz EvenClusterNo
	shr	ax, 4
	jmp CheckEOC
EvenClusterNo:
	and ax, 0x0FFF
	
	; 根据簇号判断文件是否结束，如没结束则继续读取
CheckEOC:
	cmp ax, 0x0FF7
	jb	ReadNextCluster
	
	; 文件读取完毕，关闭软驱马达
	mov	dx, 0x03F2
	xor	al, al
	out	dx, al
	
	; Loader.bin 加载完毕，跳转到 Loader.bin 执行
	jmp	0:LOADER_ORG
	
	; 出错处理：在屏幕左上角显示错误信息字符串，并且死循环
Error:	
	mov bp, strError
	mov	ax, 0x1301				; AH = 0x13,  AL = 0x01
	mov	bx, 0x0007				; 页号为 0 (BH = 0x00)，黑底白字 (BL = 0x07)
	mov cx, 26					; 字符串长度
	xor dx, dx
	int	0x10
	jmp $

;----------------------------------------------------------------------------
; 函数名: ReadSector
; 作  用: 从第 ax 个 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
;----------------------------------------------------------------------------
ReadSector:
	push bp
	mov	bp, sp
	push cx						; 保存 cl
	push bx						; 保存 bx
	
	;
	; 计算 柱面号、起始扇区 和 磁头号
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数     │
	;                   └ 余 z => 起始扇区号 = z + 1
	;
	mov	bl, [SectorsPerTrack]	; bl: 除数
	div	bl						; y 在 al 中, z 在 ah 中
	inc	ah						; z ++
	mov	cl, ah					; cl <- 起始扇区号
	mov	dh, al					; dh <- y
	shr	al, 1					; y >> 1 (其实是 y / Heads, 这里 Heads = 2)
	mov	ch, al					; ch <- 柱面号
	and	dh, 1					; dh & 1 = 磁头号
	mov	dl, [DriveNumber]		; 驱动器号 (0 表示 A 盘)
	pop bx						; 恢复 bx
	
.GoOnReading:
	mov	ah, 2					; 读
	mov	al, byte [bp-2]			; 读 al 个扇区
	int	0x13
	jc	.GoOnReading			; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	; 恢复堆栈并返回
	pop cx
	pop	bp
	ret

	;
	; 引导扇区代码结束，填充剩下的空间，使生成的二进制代码恰好为 512 字节
	;
	times 	510-($-$$)	db	0
	dw 	0xaa55					; 引导扇区激活标志
