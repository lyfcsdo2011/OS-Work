;***
;
; Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。
;
; 只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
; 如果您不接受，不能使用这些代码。
;
; 文件名: cpu.asm
;
; 描述: 
;
; 
;
;*******************************************************************************/


;
; 导出符号
;
global _KiInitializeProcessor
global _KeCodeSegmentSelector
global _KeDataSegmentSelector
global _Gdt
global _CsDesc
global _DsDesc

;
; 用于定义 i386 描述符的宏，三个参数分别是 基址、界限、属性（属性见下面定义）
;
%macro Descriptor 3
	dw	%2 & 0xFFFF							; 段界限 1						(2 字节)
	dw	%1 & 0xFFFF							; 段基址 1						(2 字节)
	db	(%1 >> 16) & 0xFF					; 段基址 2						(1 字节)
	dw	((%2 >> 8) & 0x0F00) | (%3 & 0xF0FF); 属性 1 + 段界限 2 + 属性 2	(2 字节)
	db	(%1 >> 24) & 0xFF					; 段基址 3						(1 字节)
%endmacro ; 共 8 字节

;
; 描述符属性
;
DA_32			equ		0x4000	; 32 位段
DA_LIMIT_4K		equ		0x8000	; 段界限粒度为 4K 字节
DA_DRW			equ		0x92	; 存在的可读写数据段属性值
DA_CR			equ		0x9A	; 存在的可执行可读代码段属性值

[section .data]

;
; 定义全局描述符表
;
;							段基址,		段界限,		属性
_Gdt:		Descriptor		0,			0,			0								; 空描述符
_CsDesc:	Descriptor		0,			0x0FFFFF,	DA_CR  | DA_32 | DA_LIMIT_4K	; 0 ~ 4G 的代码段
_DsDesc:	Descriptor		0,			0x0FFFFF,	DA_DRW | DA_32 | DA_LIMIT_4K	; 0 ~ 4G 的数据段

GDT_SIZE					equ	$ - _Gdt				; 全局描述符表的长度
_KeCodeSegmentSelector		dw	_CsDesc - _Gdt			; 代码段选择子
_KeDataSegmentSelector		dw	_DsDesc - _Gdt			; 数据段选择子

[section .text]

_KiInitializeProcessor:
;{
	push ebp
	mov ebp, esp
	
	;
	; 加载全局描述符表。
	;
	push dword _Gdt
	push word GDT_SIZE
	lgdt [esp]
	add esp, 6
	
	;
	; 设置代码段和数据段的选择子。
	; 注意：设置代码段选择子只能通过一个长跳转实现。
	;
	jmp dword (_CsDesc - _Gdt):.SET_DS

.SET_DS:
	mov ax, (_DsDesc - _Gdt)
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	leave
	ret
;}
