;***
;
; Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����
;
; ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
; ����������ܣ�����ʹ����Щ���롣
;
; �ļ���: __alloca.asm
;
; ����: __alloca
;
; 
;
;*******************************************************************************/

global __alloca

[section .text]

__alloca:
;{
	sub eax, 1
	and eax, ~3
	sub esp, eax
	push dword [esp + eax]
	mov eax, esp
	add eax, 4
	ret
;}
