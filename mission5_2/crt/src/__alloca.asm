;***
;
; Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。
;
; 只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
; 如果您不接受，不能使用这些代码。
;
; 文件名: __alloca.asm
;
; 描述: __alloca
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
