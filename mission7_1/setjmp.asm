;***
;
; Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����
;
; ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
; ����������ܣ�����ʹ����Щ���롣
;
; �ļ���: setjmp.asm
;
; ����: 
;
; 
;
;*******************************************************************************/


global _setjmp
global _longjmp

OFF_EBX equ 0
OFF_EDI equ 4
OFF_ESI equ 8
OFF_EBP equ 12
OFF_ESP equ 16
OFF_EIP equ 20

[section .text]

_setjmp:
;{	
	push ebp
	mov ebp, esp

	mov ecx, [ebp + 8]			; ecx ָ�� JUMP_BUFFER
	
	mov [ecx + OFF_EBX], ebx
	mov [ecx + OFF_EDI], edi
	mov [ecx + OFF_ESI], esi
	
	mov eax, [ebp]
	mov [ecx + OFF_EBP], eax
	
	mov eax, esp
	add eax, 8
	mov [ecx + OFF_ESP], eax
	
	mov eax, [ebp + 4]
	mov [ecx + OFF_EIP], eax
	
	xor eax, eax
	leave
	ret
;}

_longjmp:
;{
	push ebp
	mov ebp, esp
	
	mov ecx, [ebp + 8]			; ecx ָ�� JUMP_BUFFER
	mov eax, [ebp + 12]			; eax = retval
	
	mov ebx, [ecx + OFF_EBX]
	mov edi, [ecx + OFF_EDI]
	mov esi, [ecx + OFF_ESI]
	mov ebp, [ecx + OFF_EBP]
	mov esp, [ecx + OFF_ESP]
	
	push dword [ecx + OFF_EIP]
	ret
;}
