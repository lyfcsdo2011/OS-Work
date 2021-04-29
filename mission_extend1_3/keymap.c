/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: keymap.c

描述: 按键字符映射表。



*******************************************************************************/

#include "rtl.h"

//
// 字符映射表，应根据键盘区域设置选择相应字符映射表。
// 目前仅支持美式键盘，不支持映射表切换。
//
PRIVATE CHAR IopAsciiMap0_9[10] = {')','!','@','#','$','%','^','&','*','('};

PRIVATE CHAR IopAsciiMapOperator[6] = {'*','+', 0 ,'-','.','/'};

PRIVATE CHAR IopAsciiMapOEM1_3[7][2] = {
	{';', ':'},
	{'=', '+'},
	{',', '<'},
	{'-', '_'},
	{'.', '>'},
	{'/', '?'},
	{'`', '~'}
};

PRIVATE CHAR IopAsciiMapOEM4_7[4][2] = {
	{'[', '{'},
	{'\\', '|'},
	{']', '}'},
	{'\'', '\"'}
};

CHAR
TranslateKeyToChar(
	IN UCHAR VirtualKeyValue,
	IN ULONG ControlKeyState,
	IN ULONG LocationCode
	)
/*++

功能描述：
	将虚键值转换为对应的可见ASCII码。

参数：
	VirtualKeyValue -- 虚键值。
	ControlKeyState -- 控制键的状态。
	LocationCode -- 键盘区域编码。指定0使用美式键盘，暂不支持多区域。

返回值：
	如果虚键可转换为可见ASCII码则返回TRUE，否则返回FALSE。

--*/
{
	CHAR AsciiChar;
	BOOL IsShiftDown;
	BOOL IsCapsLockOn;
	BOOL IsNumLockOn;

	ASSERT(0 == LocationCode);

	//
	// 查询当前功能键的状态。
	// 注意，左右Shift键有任意一个被按下则认为Shift键被按下。
	//
	IsShiftDown = (ControlKeyState & SHIFT_PRESSED) != 0;
	IsCapsLockOn = (ControlKeyState & CAPSLOCK_ON) != 0;
	IsNumLockOn = (ControlKeyState & NUMLOCK_ON) != 0;

	AsciiChar = 0;

	if (VirtualKeyValue >= '0' && VirtualKeyValue <= '9') {

		//
		// 数字键。
		//
		if (!IsShiftDown) {
			AsciiChar = VirtualKeyValue; // 字符'0' - '9'
		}else {
			AsciiChar = IopAsciiMap0_9[VirtualKeyValue - '0']; // 数字键上的符号
		}

	} else if (VirtualKeyValue >= 'A' && VirtualKeyValue <= 'Z') {

		//
		// 字母键。
		//
		if (IsShiftDown != IsCapsLockOn) {
			AsciiChar = VirtualKeyValue; // 大写字符'A' - 'Z'
		} else {
			AsciiChar = VirtualKeyValue + ('a' - 'A'); // 小写字符'a' - 'z'
		}

	} if (VirtualKeyValue >= VK_NUMPAD0 && VirtualKeyValue <= VK_NUMPAD9) {

		//
		// 小键盘上的数字键0-9。
		//
		if (IsNumLockOn) {
			AsciiChar = VirtualKeyValue - VK_NUMPAD0 + '0';
		}

	} else if (VirtualKeyValue >= VK_MULTIPLY && VirtualKeyValue <= VK_DIVIDE) {

		//
		// 小键盘上运算符 + - * \ . 。
		// 注意‘.’在没有NumLock的时候用作Del
		//
		if (VK_DECIMAL != VirtualKeyValue || IsNumLockOn) {
			AsciiChar = IopAsciiMapOperator[VirtualKeyValue - VK_MULTIPLY]; 
		}

	} else if (VirtualKeyValue >= VK_OEM_1 && VirtualKeyValue <= VK_OEM_3) {

		//
		// OEM键。
		//
		if (!IsShiftDown) {
			AsciiChar = IopAsciiMapOEM1_3[VirtualKeyValue - VK_OEM_1][0];
		} else {
			AsciiChar = IopAsciiMapOEM1_3[VirtualKeyValue - VK_OEM_1][1];
		}

	} else if (VirtualKeyValue >= VK_OEM_4 && VirtualKeyValue <= VK_OEM_7) {

		//
		// OEM键。
		//
		if (!IsShiftDown) {
			AsciiChar = IopAsciiMapOEM4_7[VirtualKeyValue - VK_OEM_4][0];
		} else {
			AsciiChar = IopAsciiMapOEM4_7[VirtualKeyValue - VK_OEM_4][1];
		}
	}

	return AsciiChar;
}
