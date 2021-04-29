;***
;
; Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����
;
; ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
; ����������ܣ�����ʹ����Щ���롣
;
; �ļ���: boot.asm
;
; ����: ����������
;
; 
;
;*******************************************************************************/

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               boot.asm
;
;     PC ���ӵ��CPU ����ʵģʽ���ֶι����ڴ棬������ 1M ��ַ�ռ䣨û
; �д� A20 ������£���CPU ����ִ�� BIOS ������ BIOS ����豸���ȹ�
; ������� BIOS ������Ϊ�������������� BIOS �Ὣ���̵�����������512 �ֽڣ�
; ���ص������ַ 0x7C00 - 0x7DFF ����Ȼ�� CPU �� CS �Ĵ�������Ϊ 0x0000,
; �� IP �Ĵ�������Ϊ 0x7C00�������� CPU �Ϳ�ʼִ�����������еĳ���
;     ���ڶν���Ϊ 64K�������ڲ��޸ĶμĴ����������ֻ�ܷ��� 0x0000 �� 0xFFFF
; �ĵ�ַ�ռ䣬�������������ͱ����ص��˴˷�Χ�ڣ�������������������������һ��
; ����Ҫ�޸ĶμĴ�����
;     ��ʱ�������ڴ�Ӧ������������ӣ�
;
;                 +-------------------------------------+----------------------
;          0x0000 |                                     |
;                 |   BIOS �ж������� (1K)              |
;                 |   BIOS Interrupt Vector Table       |
;                 |                                     |
;                 +-------------------------------------+
;          0x0400 |   BIOS ������ (512 Bytes)           |
;                 |   BIOS Data Area                    |
;                 +-------------------------------------+
;          0x0600 |                                     |
;                 |                                     |
;                 |             �û�����(1)             |   �����ڴ� (640K)
;                 |                                     |  Conventional Memory
;                 |                                     |
;                 +-------------------------------------+
;          0x7C00 |   ������������ (512 Bytes)          |
;                 |   Floppy Boot Sector                |
;                 +-------------------------------------+
;          0x7E00 |                                     |
;                 |                                     |
;                 |             �û�����(2)             |
;                 |                                     |
;                 |                                     |
;                 +-------------------------------------+----------------------
;         0xA0000 |                                     |
;                 |                                     |
;                 |   ϵͳռ�� (384K)                   |   ��λ�ڴ� (384K)
;                 |                                     |   Upper Memory
;                 |                                     |
;                 +-------------------------------------+----------------------
;        0x100000 |                                     |
;                 |                                     |   ��չ�ڴ棨ֻ�н��뱣��ģʽ���ܷ��ʣ�
;                 |               ������                |  Extended Memory
;                 Z                                     Z
;                 |                                     |
;    �����ڴ���� |                                     |
;                 +-------------------------------------+----------------------
;
;     EOS ������������������ѡ�� Loader.bin �ӵ�һ���û���������� 0x1000 ����ʼ
; ���أ����� 0x1000 �� 0x7BFF������ Loader ���ֻ��Ϊ 0x7C00 - 0x1000 = 0x6C00
; ���ֽڡ�����ڱ���ģʽ�а��� 4K ��С���з�ҳ���� Loader ����һ��ҳ��Ŀ�ʼ����
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	org 0x7C00
	jmp short Start
	nop					; ��� nop ������

; ----------------------------------------------------------------------
; FAT12 ��������ͷ
Oem						db 'Engintim'	; OEM String������ 8 ���ֽ�
BytesPerSector			dw 512			; ÿ�����ֽ���					----+
SectorsPerCluster		db 1			; ÿ�ض�������						|
ReservedSectors			dw 1			; Boot ��¼ռ�ö�������				|
Fats					db 2			; FAT ����							|
RootEntries				dw 224			; ��Ŀ¼�ļ������ֵ				|
Sectors					dw 2880			; ��������							\\ BPB
Media					db 0xF0			; ��������							// BIOS Parameter Block
SectorsPerFat			dw 9			; ÿ FAT ������						|
SectorsPerTrack			dw 18			; ÿ�ŵ�������						|
Heads					dw 2			; ��ͷ��							|
HiddenSectors			dd 0			; ����������						|
LargeSectors			dd 0			; ����������Sectors Ϊ 0 ʱʹ��	----+
DriveNumber				db 0			; ��������
Reserved				db 0			; ����δ��
Signature				db 0x29			; ������� (0x29)
Id						dd 0			; �����к�
VolumeLabel				db 'EOS        '; ��꣬���� 11 ���ֽ�
SystemId				db 'FAT12   '	; �ļ�ϵͳ���ͣ����� 8 ���ֽ�
;------------------------------------------------------------------------

; FAT12 �ļ�ϵͳ��ص�һЩ����
FirstSectorOfRootDir	dw 0			; ��Ŀ¼����ʼ������
RootDirectorySectors	dw 0			; ��Ŀ¼ռ�õ���������
FirstSectorOfFileArea	dw 0			; ����������ʼ������
BufferOfFat				dw 0			; FAT ��������ַ
BufferOfRootDir			dw 0			; ��Ŀ¼��������ַ

LOADER_ORG				equ	0x1000				; Loader.bin ����ʼ��ַ
MAX_FILE_SIZE			equ 0x6C00				; Loader.bin ֻռ�� 0x1000 �� 0x7C00 �Ŀռ�
wFilePos				dw	LOADER_ORG			; ���ڼ��� Loader.bin ���α�
LoaderFileName			db	"LOADER  BIN"		; Loader.bin ���ļ���
strError:				db	"File Loader.bin not found!"

Start:
	; ��ʼ�� CPU �ĶμĴ���Ϊ CS ��ֵ(0)����ջ�� 64K ��������
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	xor sp, sp
	mov bp, sp

	; ��ʼ����Ļ
	mov	ax, 0x0600		; AH = 0x06,  AL = 0x00
	mov	bx, 0x0700		; �ڵװ���(BH = 0x07)
	xor cx, cx			; ���Ͻ�: (�к�  0, �к�  0)
	mov	dx, 0x184F		; ���½�: (�к� 79, �к� 24)
	int	0x10
	
	; ������λ
	xor	ah, ah
	xor	dl, dl
	int	0x13
	
	;
	; �����Ŀ¼����ʼ������
	; FirstSectorOfRootDir = ReservedSectors + SectorsPerFat * Fats
	;
	mov ax, word [SectorsPerFat]
	movzx bx, byte [Fats]
	mul bx
	add ax, word [ReservedSectors]
	mov word [FirstSectorOfRootDir], ax
	
	;
	; �����Ŀ¼ռ�õ���������
	; RootDirectorySectors = RootEntries * 32 / BytesPerSector
	;
	mov ax, word [RootEntries]
	shl ax, 5
	mov bx, word [BytesPerSector]
	div bx
	mov word [RootDirectorySectors], ax
	
	;
	; ���������������ʼ������
	; FirstSectorOfFileArea = FirstSectorOfRootDir + RootDirectorySectors
	;
	add ax, word [FirstSectorOfRootDir]
	mov word [FirstSectorOfFileArea], ax
	
	;
	; ���� FAT ��������ַ������������������
	; BufferOfFat = 0x7C00 + BytesPerSector * ReservedSectors
	;
	mov ax, word [BytesPerSector]
	mul word [ReservedSectors]
	add ax, 0x7C00
	mov word [BufferOfFat], ax
	
	;
	; �����Ŀ¼��������ַ�������� FAT ��������
	; BufferOfRootDir = BufferOfFat + BytesPerSector * SectorsPerFat
	;
	mov ax, word [BytesPerSector]
	mul word [SectorsPerFat]
	add ax, word [BufferOfFat]
	mov word [BufferOfRootDir], ax
	
	; �� FAT1 ���� FAT ������
	mov ax, word [ReservedSectors]		; 
	mov cx, word [SectorsPerFat]		; һ�� FAT �����������
	mov bx, word [BufferOfFat]			; es:bx ָ�� FAT ������
	call ReadSector
	
	; ����Ŀ¼���뻺����
	mov ax, word[FirstSectorOfRootDir]
	mov cx, word[RootDirectorySectors]
	mov bx, word[BufferOfRootDir]
	call ReadSector
	
	; �ڸ�Ŀ¼�в��� Loader.bin �ļ�
FindFile:
	mov	bx, word [BufferOfRootDir]		; bx ָ���һ����Ŀ¼��
	mov dx, word [RootEntries]			; ��Ŀ¼������
	cld

CompareNextDirEntry:
	mov	si, LoaderFileName				; si -> "LOADER  BIN"
	mov di, bx							; di -> Ŀ¼�����ļ����ַ���
	mov	cx, 11							; �ļ����ַ����ĳ���
	repe cmpsb							; �ַ����Ƚ�
	cmp	cx, 0
	je	CheckFileSize					; ����Ƚ��� 11 ���ַ������, ��ʾ�ҵ��ļ�
	
	; �ļ�����һ�£������Ƚ���һ��Ŀ¼��
	add bx, 0x20						; bx ָ����һ��Ŀ¼��
	dec dx								; ��Сʣ��Ŀ¼��
	jnz CompareNextDirEntry
	
	; ����������Ŀ¼����û���ҵ��ļ�����ʾ����
	jmp	Error

	; �ҵ��ļ��󣬼���ļ��Ĵ�С
CheckFileSize:
	mov eax, dword [bx + 0x1C]			; �õ��ļ��Ĵ�С
	test eax, eax
	jz Error
	cmp eax, MAX_FILE_SIZE
	ja Error
	
	; ��ʼ�����ļ�
	mov	ax, word [bx + 0x1A]			; ��ʼ�� ax Ϊ�ļ��ĵ�һ���غ�
ReadNextCluster:
	push ax								; ����Ҫ��ȡ�Ĵغ�
	
	;
	; ���� ax �Ŵض�Ӧ�������ţ������� = ��������ʼ������ + ���غ� - 2�� * ÿ��������
	;
	sub ax, 2
	movzx cx, byte [SectorsPerCluster]
	mul	cx
	add ax, word [FirstSectorOfFileArea]
	
	mov bx, word [wFilePos];			; �ļ���������ַ
	
	call ReadSector						; ��һ����
	
	;
	; �ļ�λ������ƶ�һ���صĴ�С
	; wFilePos = wFilePos + BytesPerSector * SectorsPerCluster
	;
	mov ax, word [BytesPerSector]
	movzx bx, byte [SectorsPerCluster]
	mul bx
	add ax, word [wFilePos];
	mov word [wFilePos], ax		
	
	; ���� FAT �������һ��Ҫ��ȡ�Ĵ�
	pop ax								; �ն�ȡ�Ĵغ�
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
	
	; ���ݴغ��ж��ļ��Ƿ��������û�����������ȡ
CheckEOC:
	cmp ax, 0x0FF7
	jb	ReadNextCluster
	
	; �ļ���ȡ��ϣ��ر��������
	mov	dx, 0x03F2
	xor	al, al
	out	dx, al
	
	; Loader.bin ������ϣ���ת�� Loader.bin ִ��
	jmp	0:LOADER_ORG
	
	; ����������Ļ���Ͻ���ʾ������Ϣ�ַ�����������ѭ��
Error:	
	mov bp, strError
	mov	ax, 0x1301				; AH = 0x13,  AL = 0x01
	mov	bx, 0x0007				; ҳ��Ϊ 0 (BH = 0x00)���ڵװ��� (BL = 0x07)
	mov cx, 26					; �ַ�������
	xor dx, dx
	int	0x10
	jmp $

;----------------------------------------------------------------------------
; ������: ReadSector
; ��  ��: �ӵ� ax �� Sector ��ʼ, �� cl �� Sector ���� es:bx ��
;----------------------------------------------------------------------------
ReadSector:
	push bp
	mov	bp, sp
	push cx						; ���� cl
	push bx						; ���� bx
	
	;
	; ���� ����š���ʼ���� �� ��ͷ��
	; ��������Ϊ x
	;                           �� ����� = y >> 1
	;       x           �� �� y ��
	; -------------- => ��      �� ��ͷ�� = y & 1
	;  ÿ�ŵ�������     ��
	;                   �� �� z => ��ʼ������ = z + 1
	;
	mov	bl, [SectorsPerTrack]	; bl: ����
	div	bl						; y �� al ��, z �� ah ��
	inc	ah						; z ++
	mov	cl, ah					; cl <- ��ʼ������
	mov	dh, al					; dh <- y
	shr	al, 1					; y >> 1 (��ʵ�� y / Heads, ���� Heads = 2)
	mov	ch, al					; ch <- �����
	and	dh, 1					; dh & 1 = ��ͷ��
	mov	dl, [DriveNumber]		; �������� (0 ��ʾ A ��)
	pop bx						; �ָ� bx
	
.GoOnReading:
	mov	ah, 2					; ��
	mov	al, byte [bp-2]			; �� al ������
	int	0x13
	jc	.GoOnReading			; �����ȡ���� CF �ᱻ��Ϊ 1, ��ʱ�Ͳ�ͣ�ض�, ֱ����ȷΪֹ

	; �ָ���ջ������
	pop cx
	pop	bp
	ret

	;
	; ��������������������ʣ�µĿռ䣬ʹ���ɵĶ����ƴ���ǡ��Ϊ 512 �ֽ�
	;
	times 	510-($-$$)	db	0
	dw 	0xaa55					; �������������־
