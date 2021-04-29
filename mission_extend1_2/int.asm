;***
;
; Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����
;
; ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
; ����������ܣ�����ʹ����Щ���롣
;
; �ļ���: int.asm
;
; ����: 
;
; 
;
;*******************************************************************************/


;
; ��������
;
global _KeIsrStack
global _KiIntNesting

global _KiInitializeInterrupt
global _KeGetIntNesting
global _KeEnableInterrupts

;
; �������
;
extern _KiDispatchException
extern _KiDispatchInterrupt
extern _PspSelectNextThread

;
; CONTEXT �ṹ��Ĵ�С�Լ��������ƫ����
;
CONTEXT_SIZE	equ		64

OFF_EAX			equ		0 * 4
OFF_ECX			equ		1 * 4
OFF_EDX			equ		2 * 4
OFF_EBX			equ		3 * 4
OFF_ESP			equ		4 * 4
OFF_EBP			equ		5 * 4
OFF_ESI			equ		6 * 4
OFF_EDI			equ		7 * 4
OFF_EIP			equ		8 * 4
OFF_EFLAGS		equ		9 * 4
OFF_CS			equ		10 * 4
OFF_SS			equ		11 * 4
OFF_DS			equ		12 * 4
OFF_ES			equ		13 * 4
OFF_FS			equ		14 * 4
OFF_GS			equ		15 * 4

;
; ���������ж��������ĺ꣬���������ֱ��� �ж����������ַ���жϺš��жϴ��������ڵ�ַ
;
%macro SET_INT_DESC 3
	push eax
	mov eax, %3
	mov word [%1 + %2 * 8 + 0], ax
	mov word [%1 + %2 * 8 + 2], 0x0008
	mov byte [%1 + %2 * 8 + 5], 0x8E
	shr eax, 16
	mov word [%1 + %2 * 8 + 6], ax
	pop eax
%endmacro

;
; ���ݽ�
;
[section .data]

_Idt times 2048				db	0				; �ж���������
_KiIntNesting				dd	1				; ϵͳ����ʱ��KiSystemStartup��ISRջ��ִ�С�
_KeIsrStack					dd	0				; ISRջָ��
_IntNumber					dd	0				; �жϺ�
_ErrorCode					dd	0				; �쳣������
_ContextPtr					dd	0				; ��ǰ�߳� CONTEXT ��ָ��
_RetAddress					dd	0				; ���ص�ַ
_EaxValue					dd	0				; �����ݴ� EAX
_EbxValue					dd	0				; �����ݴ� EBX

;
; �����
;
[section .text]

_KiInitializeInterrupt:
;{
	push ebp
	mov ebp, esp

	;
	; ��ʼ���ж�������������б���ΪĬ��ֵ
	;
	xor edi, edi
.LOOP:
	SET_INT_DESC _Idt, edi, Exp_3
	inc edi
	cmp edi, 256
	jb .LOOP
	
	;
	; �����ж�����������Ҫ�õ��ı���
	;
	SET_INT_DESC _Idt, 0, Exp_0
	SET_INT_DESC _Idt, 1, Exp_1
	SET_INT_DESC _Idt, 3, Exp_3
	SET_INT_DESC _Idt, 4, Exp_4
	SET_INT_DESC _Idt, 5, Exp_5
	SET_INT_DESC _Idt, 6, Exp_6
	SET_INT_DESC _Idt, 7, Exp_7
	SET_INT_DESC _Idt, 8, Exp_8
	SET_INT_DESC _Idt, 9, Exp_9
	SET_INT_DESC _Idt, 10, Exp_10
	SET_INT_DESC _Idt, 11, Exp_11
	SET_INT_DESC _Idt, 12, Exp_12
	SET_INT_DESC _Idt, 13, Exp_13
	SET_INT_DESC _Idt, 14, Exp_14
	SET_INT_DESC _Idt, 16, Exp_16
	
	SET_INT_DESC _Idt, 32, Int_32
	SET_INT_DESC _Idt, 33, Int_33
	SET_INT_DESC _Idt, 35, Int_35
	SET_INT_DESC _Idt, 36, Int_36
	SET_INT_DESC _Idt, 37, Int_37
	SET_INT_DESC _Idt, 38, Int_38
	SET_INT_DESC _Idt, 39, Int_39
	SET_INT_DESC _Idt, 40, Int_40
	SET_INT_DESC _Idt, 44, Int_44
	SET_INT_DESC _Idt, 45, Int_45
	SET_INT_DESC _Idt, 46, Int_46
	
	SET_INT_DESC _Idt, 48, Int_48

	;
	; �����ж���������
	;
	push dword _Idt
	push word 2048
	lidt [esp]
	add esp, 6
	
	leave
	ret
;}

Exp_0:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 0
	jmp Exception
Exp_1:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 1
	jmp Exception
Exp_3:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 3
	jmp Exception
Exp_4:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 4
	jmp Exception
Exp_5:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 5
	jmp Exception
Exp_6:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 6
	jmp Exception
Exp_7:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 7
	jmp Exception
Exp_8:
	pop dword [_ErrorCode]
	mov dword [_IntNumber], 8
	jmp Exception
Exp_9:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 9
	jmp Exception
Exp_10:
	pop dword [_ErrorCode]
	mov dword [_IntNumber], 10
	jmp Exception
Exp_11:
	pop dword [_ErrorCode]
	mov dword [_IntNumber], 11
	jmp Exception
Exp_12:
	pop dword [_ErrorCode]
	mov dword [_IntNumber], 12
	jmp Exception
Exp_13:
	pop dword [_ErrorCode]
	mov dword [_IntNumber], 13
	jmp Exception
Exp_14:
	pop dword [_ErrorCode]
	mov dword [_IntNumber], 14
	jmp Exception
Exp_16:
	mov dword [_ErrorCode], 0
	mov dword [_IntNumber], 16
	jmp Exception
	
Int_32:
	mov dword [_IntNumber], 32
	jmp Interrupt
Int_33:
	mov dword [_IntNumber], 33
	jmp Interrupt
Int_35:
	mov dword [_IntNumber], 35
	jmp Interrupt
Int_36:
	mov dword [_IntNumber], 36
	jmp Interrupt
Int_37:
	mov dword [_IntNumber], 37
	jmp Interrupt
Int_38:
	mov dword [_IntNumber], 38
	jmp Interrupt
Int_39:
	mov dword [_IntNumber], 39
	jmp Interrupt
Int_40:
	mov dword [_IntNumber], 40
	jmp Interrupt
Int_44:
	mov dword [_IntNumber], 44
	jmp Interrupt
Int_45:
	mov dword [_IntNumber], 45
	jmp Interrupt
Int_46:
	mov dword [_IntNumber], 46
	jmp Interrupt
	
Int_48:
	call IntEnter
	mov dword [_KiIntNesting], 1
	push dword 1
	call IntExit

;
; �쳣����
;
Exception:
;{
	;
	; �����жϽ��뺯�������汻�жϵ� CPU �ֳ������������ж϶�ջ֡
	;
	call IntEnter
	
	;
	; ���쳣�š������롢CONTEXTָ��Ϊ�����������쳣��ǲ����KiDispatchException��
	; ע�⣺KiDispatchException�з���ֵ������ֵ���������£�
	;		0�������߳�ʱ��ִ���̵߳��ȣ�ֱ�ӷ��ص������쳣���Ǹ��̡߳�
	;		��0�������߳�ʱִ���̵߳��ȣ����ص����ȳ���ȷ��Ӧ��ִ�е��̡߳�
	;
	push eax ; eax��IntEnter�ķ���ֵ��ָ������쳣��CONTEXT
	push dword [_ErrorCode]
	push dword [_IntNumber]
	call _KiDispatchException
	add esp, 12
	
	;
	; ��KiDispatchException�ķ���ֵΪ���������жϷ��غ�����
	;
	push eax
	call IntExit
;}

;
; �жϴ���
;
Interrupt:
;{
	;
	; �����жϽ��뺯�������汻�жϵ� CPU �ֳ������������ж϶�ջ֡��
	; Ȼ����жϣ��豸�жϿ�Ƕ�ף���
	;
	call IntEnter
	sti

	;
	; ���жϺ���Ϊ���������ж���ǲ�������޷���ֵ��
	;
	push dword [_IntNumber]
	call _KiDispatchInterrupt
	add esp, 4

	;
	; ���жϣ�Ȼ������жϷ��غ�����ִ���̵߳��ȣ�
	;
	cli
	push dword 1
	call IntExit
;}

;
; �жϽ��뺯����
;
IntEnter:
;{
	;
	; ȡ�� call �� esp ֵ��Ӱ�죬���ݴ� eax �� ebx
	;
	pop dword [_RetAddress]
	mov [_EaxValue], eax
	mov [_EbxValue], ebx

	;
	; �����ж�Ƕ����ȡ�
	;
	inc dword [_KiIntNesting]
	cmp dword [_KiIntNesting], 1
	jne	.NESTED_INT
	
	;
	; �̱߳��жϣ���ʱ [_ContextPtr] ָ���ж��̵߳� CONTEXT��
	; ʹ ebx ָ���ж�ջ�Ļ�ַ��eax ָ���̵߳� CONTEXT��
	;
	mov ebx, [_KeIsrStack]
	mov eax, [_ContextPtr]
	jmp .SAVE_CONTEXT
	
	
.NESTED_INT:
	;
	; Ƕ���жϣ��ڵ�ǰ�ж�ջ������ CONTEXT �ռ䡣
	; ʹ ebx ָ��������Ӻ��ջ����eax ָ��ջ�е� CONTEXT��
	;
	mov ebx, esp
	sub ebx, CONTEXT_SIZE
	mov eax, ebx

.SAVE_CONTEXT:
	;
	; �� CPU �ֳ����浽 eax ָ��� CONTEXT �ṹ����
	;
	mov [eax + OFF_ECX], ecx
	mov ecx, [_EaxValue]
	mov [eax + OFF_EAX], ecx
	mov ecx, [_EbxValue]
	mov [eax + OFF_EBX], ecx	
	mov [eax + OFF_EDX], edx
	mov [eax + OFF_ESP], esp
	mov [eax + OFF_EBP], ebp
	mov [eax + OFF_ESI], esi
	mov [eax + OFF_EDI], edi
	
	xor ecx, ecx
	mov cx, ds
	mov [eax + OFF_DS], ecx
	mov cx, es
	mov [eax + OFF_ES], ecx
	mov cx, fs
	mov [eax + OFF_FS], ecx
	mov cx, gs
	mov [eax + OFF_GS], ecx
	mov cx, ss
	mov [eax + OFF_SS], ecx
	
	pop dword [eax + OFF_EIP]
	pop dword [eax + OFF_CS]
	pop dword [eax + OFF_EFLAGS]
	
	mov [eax + OFF_ESP], esp
	
	;
	; �� ebx ָ���λ�ù���һ����������֡
	;
	mov esp, ebx
	push dword 0	; ret address of call instruction
	push dword 0	; old value of ebp (push ebp)
	mov ebp, esp

	;
	; ����
	;
	jmp dword [_RetAddress]	
;}

;
; �ж��˳�������
;
IntExit:
;{
	cli
	push ebp
	mov ebp, esp

	;
	; ����ж�Ƕ�׼�����Ϊ1�򷵻ص��̣߳����򷵻ص���Ƕ���жϡ�
	;
	cmp dword [_KiIntNesting], 1
	je .RETURN_TO_THREAD

	;
	; ���ص���Ƕ���жϣ�ʹEAXָ�򱣴����ж�ջ�е�CONTEXT�ṹ�塣
	;
	mov eax, [ebp]
	add eax, 8
	jmp .RESTORE_CONTEXT

.RETURN_TO_THREAD:
	;
	; ���ݲ���ȷ���Ƿ�ִ���̵߳��ȣ������Ҫ�����򷵻ص����ȳ���ȷ����
	; ��һ��Ӧ�ø����е��̣߳����򷵻ص����ж��̡߳�
	;
	cmp dword [ebp + 8], 0
	jne .SELECT_NEXT_THREAD

	;
	; �����ȣ�ʹEAXָ���жϵ��̵߳�CONTEXT�ṹ�塣
	;
	mov eax, [_ContextPtr]
	jmp .RESTORE_CONTEXT

.SELECT_NEXT_THREAD:
	;
	; ����PspSelectNextThread���䷵����һ��Ӧ�������̵߳�CONTEXTָ�롣
	; ע�⣺Ҫ����ָ��ֵ��[_ContextPtr]�����̱߳��ж�ʱ����ʹ�á�
	;
	call _PspSelectNextThread
	mov [_ContextPtr], eax

.RESTORE_CONTEXT:
	;
	; �ָ� CPU ����Ϊ EAX ָ��� CONTEXT
	;
	mov ebx, [eax + OFF_EBX]
	mov ecx, [eax + OFF_ECX]
	mov edx, [eax + OFF_EDX]
	mov edi, [eax + OFF_EDI]
	mov esi, [eax + OFF_ESI]
	mov ebp, [eax + OFF_EBP]
	mov esp, [eax + OFF_ESP]
	
	mov ds, [eax + OFF_DS]
	mov es, [eax + OFF_ES]
	mov fs, [eax + OFF_FS]
	mov gs, [eax + OFF_GS]
	mov ss, [eax + OFF_SS]
	
	push dword [eax + OFF_EFLAGS]
	push dword [eax + OFF_CS]
	push dword [eax + OFF_EIP]
	
	mov eax, [eax + OFF_EAX]

	; �����ж�Ƕ����ȡ�
	dec dword [_KiIntNesting]
	
	; �жϷ��أ���ʼִ��ѡ�е��̡߳�
	iret
;}

_KeGetIntNesting:
;{
	push ebp
	mov ebp, esp

	mov eax, [_KiIntNesting]

	leave
	ret
;}
