;***
;
; Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����
;
; ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
; ����������ܣ�����ʹ����Щ���롣
;
; �ļ���: hal386.asm
;
; ����: 
;
; 
;
;*******************************************************************************/


global _WRITE_PORT_UCHAR
global _READ_PORT_UCHAR
global _WRITE_PORT_USHORT
global _READ_PORT_USHORT
global _WRITE_PORT_ULONG
global _READ_PORT_ULONG
global _BitScanForward
global _BitScanReverse
global _KeEnableInterrupts
global _KiHaltProcessor
global _MiFlushSingleTlb
global _MiFlushEntireTlb
global _MiSetPageDirectory

[section .text]

_READ_PORT_UCHAR:
;{
	push ebp
	mov ebp, esp
	
	mov	edx, [ebp + 8]
	xor	eax, eax
	in	al, dx
	nop
	nop
	
	leave
	ret
;}

_READ_PORT_USHORT:
;{
	push ebp
	mov ebp, esp
	
	mov	edx, [ebp + 8]
	xor	eax, eax
	in	ax, dx
	nop
	nop
	
	leave
	ret
;}

_READ_PORT_ULONG:
;{
	push ebp
	mov ebp, esp
	
	mov	edx, [ebp + 8]
	in	eax, dx
	nop
	nop
	
	leave
	ret
;}

_WRITE_PORT_UCHAR:
;{
	push ebp
	mov ebp, esp
	
	mov	edx, [ebp + 8]
	mov	eax, [ebp + 12]
	out	dx, al
	nop
	nop
	
	leave
	ret
;}

_WRITE_PORT_USHORT:
;{
	push ebp
	mov ebp, esp
	
	mov	edx, [ebp + 8]
	mov	eax, [ebp + 12]
	out	dx, ax
	nop
	nop
	
	leave
	ret
;}

_WRITE_PORT_ULONG:
;{
	push ebp
	mov ebp, esp
	
	mov	edx, [ebp + 8]
	mov	eax, [ebp + 12]
	out	dx, eax
	nop
	nop
	
	leave
	ret
;}

_BitScanForward:
;{
	push ebp
	mov ebp, esp
	
	bsf eax, dword [ebp + 12]
	jz	.ERROR
	
	mov ecx, [ebp + 8]
	mov [ecx], eax
	mov eax, 1
	jmp .RETURN
	
.ERROR:
	xor eax, eax
	
.RETURN:
	leave
	ret
;}

_BitScanReverse:
;{
	push ebp
	mov ebp, esp
	
	bsr eax, dword [ebp + 12]
	jz	.ERROR
	
	mov ecx, [ebp + 8]
	mov [ecx], eax
	mov eax, 1
	jmp .RETURN
	
.ERROR:
	xor eax, eax
	
.RETURN:
	leave
	ret
;}

_KeEnableInterrupts:
;{
	push ebp
	mov ebp, esp
	pushf					; store current eflags

	cmp dword [ebp + 8], 0	; if (EnableInt)
	je .ELSE				; {
	sti						;	Enable interrupts.
	jmp .END_IF				; }
.ELSE:						; else {
	cli						;	Disable interrupts.
.END_IF:					; }

	pop eax					; eax = old eflags
	and eax, 1 << 9			; clear all flags except interrupt flag

	leave
	ret						; return eax
;}

_KiHaltProcessor:
;{
	hlt
	ret
;}

_MiSetPageDirectory:
;{
	mov eax, [esp + 4]
	shl eax, 12
	mov cr3, eax
	ret
;}

_MiFlushSingleTlb:
;{
	mov eax, [esp + 4]
	invlpg [eax]
	ret
;}

_MiFlushEntireTlb:
;{
	mov eax, cr3
	mov cr3, eax
	ret
;}
