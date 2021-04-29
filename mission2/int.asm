;***
;
; Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。
;
; 只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
; 如果您不接受，不能使用这些代码。
;
; 文件名: int.asm
;
; 描述: 
;
; 
;
;*******************************************************************************/


;
; 导出符号
;
global _KeIsrStack
global _KiIntNesting

global _KiInitializeInterrupt
global _KeGetIntNesting
global _KeEnableInterrupts

;
; 导入符号
;
extern _KiDispatchException
extern _KiDispatchInterrupt
extern _PspSelectNextThread

;
; CONTEXT 结构体的大小以及各个域的偏移量
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
; 用于设置中断描述符的宏，三个参数分别是 中断描述符表地址、中断号、中断处理程序入口地址
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
; 数据节
;
[section .data]

_Idt times 2048				db	0				; 中断描述符表
_KiIntNesting				dd	1				; 系统启动时，KiSystemStartup在ISR栈中执行。
_KeIsrStack					dd	0				; ISR栈指针
_IntNumber					dd	0				; 中断号
_ErrorCode					dd	0				; 异常错误码
_ContextPtr					dd	0				; 当前线程 CONTEXT 的指针
_RetAddress					dd	0				; 返回地址
_EaxValue					dd	0				; 用于暂存 EAX
_EbxValue					dd	0				; 用于暂存 EBX

;
; 代码节
;
[section .text]

_KiInitializeInterrupt:
;{
	push ebp
	mov ebp, esp

	;
	; 初始化中断描述符表的所有表项为默认值
	;
	xor edi, edi
.LOOP:
	SET_INT_DESC _Idt, edi, Exp_3
	inc edi
	cmp edi, 256
	jb .LOOP
	
	;
	; 设置中断描述符表中要用到的表项
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
	; 加载中断描述符表
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
; 异常处理
;
Exception:
;{
	;
	; 调用中断进入函数，保存被中断的 CPU 现场环境并构造中断堆栈帧
	;
	call IntEnter
	
	;
	; 以异常号、错误码、CONTEXT指针为参数，调用异常派遣函数KiDispatchException。
	; 注意：KiDispatchException有返回值，返回值的意义如下：
	;		0：返回线程时不执行线程调度，直接返回到产生异常的那个线程。
	;		非0：返回线程时执行线程调度，返回到调度程序确定应该执行的线程。
	;
	push eax ; eax是IntEnter的返回值，指向产生异常的CONTEXT
	push dword [_ErrorCode]
	push dword [_IntNumber]
	call _KiDispatchException
	add esp, 12
	
	;
	; 以KiDispatchException的返回值为参数调用中断返回函数。
	;
	push eax
	call IntExit
;}

;
; 中断处理
;
Interrupt:
;{
	;
	; 调用中断进入函数，保存被中断的 CPU 现场环境并构造中断堆栈帧。
	; 然后打开中断（设备中断可嵌套）。
	;
	call IntEnter
	sti

	;
	; 以中断号作为参数调用中断派遣函数（无返回值）
	;
	push dword [_IntNumber]
	call _KiDispatchInterrupt
	add esp, 4

	;
	; 关中断，然后调用中断返回函数（执行线程调度）
	;
	cli
	push dword 1
	call IntExit
;}

;
; 中断进入函数。
;
IntEnter:
;{
	;
	; 取消 call 对 esp 值的影响，并暂存 eax 和 ebx
	;
	pop dword [_RetAddress]
	mov [_EaxValue], eax
	mov [_EbxValue], ebx

	;
	; 增加中断嵌套深度。
	;
	inc dword [_KiIntNesting]
	cmp dword [_KiIntNesting], 1
	jne	.NESTED_INT
	
	;
	; 线程被中断，此时 [_ContextPtr] 指向被中断线程的 CONTEXT。
	; 使 ebx 指向中断栈的基址，eax 指向线程的 CONTEXT。
	;
	mov ebx, [_KeIsrStack]
	mov eax, [_ContextPtr]
	jmp .SAVE_CONTEXT
	
	
.NESTED_INT:
	;
	; 嵌套中断，在当前中断栈顶开辟 CONTEXT 空间。
	; 使 ebx 指向深度增加后的栈顶，eax 指向栈中的 CONTEXT。
	;
	mov ebx, esp
	sub ebx, CONTEXT_SIZE
	mov eax, ebx

.SAVE_CONTEXT:
	;
	; 将 CPU 现场保存到 eax 指向的 CONTEXT 结构体中
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
	; 在 ebx 指向的位置构造一个最外层调用帧
	;
	mov esp, ebx
	push dword 0	; ret address of call instruction
	push dword 0	; old value of ebp (push ebp)
	mov ebp, esp

	;
	; 返回
	;
	jmp dword [_RetAddress]	
;}

;
; 中断退出函数。
;
IntExit:
;{
	cli
	push ebp
	mov ebp, esp

	;
	; 如果中断嵌套计数器为1则返回到线程，否则返回到被嵌套中断。
	;
	cmp dword [_KiIntNesting], 1
	je .RETURN_TO_THREAD

	;
	; 返回到被嵌套中断，使EAX指向保存在中断栈中的CONTEXT结构体。
	;
	mov eax, [ebp]
	add eax, 8
	jmp .RESTORE_CONTEXT

.RETURN_TO_THREAD:
	;
	; 根据参数确定是否执行线程调度，如果需要调度则返回到调度程序确定的
	; 下一个应该该运行的线程，否则返回到被中断线程。
	;
	cmp dword [ebp + 8], 0
	jne .SELECT_NEXT_THREAD

	;
	; 不调度，使EAX指向被中断的线程的CONTEXT结构体。
	;
	mov eax, [_ContextPtr]
	jmp .RESTORE_CONTEXT

.SELECT_NEXT_THREAD:
	;
	; 调用PspSelectNextThread，其返回下一个应该运行线程的CONTEXT指针。
	; 注意：要保存指针值到[_ContextPtr]，当线程被中断时还将使用。
	;
	call _PspSelectNextThread
	mov [_ContextPtr], eax

.RESTORE_CONTEXT:
	;
	; 恢复 CPU 环境为 EAX 指向的 CONTEXT
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

	; 减少中断嵌套深度。
	dec dword [_KiIntNesting]
	
	; 中断返回，开始执行选中的线程。
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
