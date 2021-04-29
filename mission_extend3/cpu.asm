;***
;
; Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����
;
; ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
; ����������ܣ�����ʹ����Щ���롣
;
; �ļ���: cpu.asm
;
; ����: 
;
; 
;
;*******************************************************************************/


;
; ��������
;
global _KiInitializeProcessor
global _KeCodeSegmentSelector
global _KeDataSegmentSelector
global _Gdt
global _CsDesc
global _DsDesc

;
; ���ڶ��� i386 �������ĺ꣬���������ֱ��� ��ַ�����ޡ����ԣ����Լ����涨�壩
;
%macro Descriptor 3
	dw	%2 & 0xFFFF							; �ν��� 1						(2 �ֽ�)
	dw	%1 & 0xFFFF							; �λ�ַ 1						(2 �ֽ�)
	db	(%1 >> 16) & 0xFF					; �λ�ַ 2						(1 �ֽ�)
	dw	((%2 >> 8) & 0x0F00) | (%3 & 0xF0FF); ���� 1 + �ν��� 2 + ���� 2	(2 �ֽ�)
	db	(%1 >> 24) & 0xFF					; �λ�ַ 3						(1 �ֽ�)
%endmacro ; �� 8 �ֽ�

;
; ����������
;
DA_32			equ		0x4000	; 32 λ��
DA_LIMIT_4K		equ		0x8000	; �ν�������Ϊ 4K �ֽ�
DA_DRW			equ		0x92	; ���ڵĿɶ�д���ݶ�����ֵ
DA_CR			equ		0x9A	; ���ڵĿ�ִ�пɶ����������ֵ

[section .data]

;
; ����ȫ����������
;
;							�λ�ַ,		�ν���,		����
_Gdt:		Descriptor		0,			0,			0								; ��������
_CsDesc:	Descriptor		0,			0x0FFFFF,	DA_CR  | DA_32 | DA_LIMIT_4K	; 0 ~ 4G �Ĵ����
_DsDesc:	Descriptor		0,			0x0FFFFF,	DA_DRW | DA_32 | DA_LIMIT_4K	; 0 ~ 4G �����ݶ�

GDT_SIZE					equ	$ - _Gdt				; ȫ����������ĳ���
_KeCodeSegmentSelector		dw	_CsDesc - _Gdt			; �����ѡ����
_KeDataSegmentSelector		dw	_DsDesc - _Gdt			; ���ݶ�ѡ����

[section .text]

_KiInitializeProcessor:
;{
	push ebp
	mov ebp, esp
	
	;
	; ����ȫ����������
	;
	push dword _Gdt
	push word GDT_SIZE
	lgdt [esp]
	add esp, 6
	
	;
	; ���ô���κ����ݶε�ѡ���ӡ�
	; ע�⣺���ô����ѡ����ֻ��ͨ��һ������תʵ�֡�
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
