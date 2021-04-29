/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: keymap.c

����: �����ַ�ӳ���



*******************************************************************************/

#include "rtl.h"

//
// �ַ�ӳ���Ӧ���ݼ�����������ѡ����Ӧ�ַ�ӳ���
// Ŀǰ��֧����ʽ���̣���֧��ӳ����л���
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

����������
	�����ֵת��Ϊ��Ӧ�Ŀɼ�ASCII�롣

������
	VirtualKeyValue -- ���ֵ��
	ControlKeyState -- ���Ƽ���״̬��
	LocationCode -- ����������롣ָ��0ʹ����ʽ���̣��ݲ�֧�ֶ�����

����ֵ��
	��������ת��Ϊ�ɼ�ASCII���򷵻�TRUE�����򷵻�FALSE��

--*/
{
	CHAR AsciiChar;
	BOOL IsShiftDown;
	BOOL IsCapsLockOn;
	BOOL IsNumLockOn;

	ASSERT(0 == LocationCode);

	//
	// ��ѯ��ǰ���ܼ���״̬��
	// ע�⣬����Shift��������һ������������ΪShift�������¡�
	//
	IsShiftDown = (ControlKeyState & SHIFT_PRESSED) != 0;
	IsCapsLockOn = (ControlKeyState & CAPSLOCK_ON) != 0;
	IsNumLockOn = (ControlKeyState & NUMLOCK_ON) != 0;

	AsciiChar = 0;

	if (VirtualKeyValue >= '0' && VirtualKeyValue <= '9') {

		//
		// ���ּ���
		//
		if (!IsShiftDown) {
			AsciiChar = VirtualKeyValue; // �ַ�'0' - '9'
		}else {
			AsciiChar = IopAsciiMap0_9[VirtualKeyValue - '0']; // ���ּ��ϵķ���
		}

	} else if (VirtualKeyValue >= 'A' && VirtualKeyValue <= 'Z') {

		//
		// ��ĸ����
		//
		if (IsShiftDown != IsCapsLockOn) {
			AsciiChar = VirtualKeyValue; // ��д�ַ�'A' - 'Z'
		} else {
			AsciiChar = VirtualKeyValue + ('a' - 'A'); // Сд�ַ�'a' - 'z'
		}

	} if (VirtualKeyValue >= VK_NUMPAD0 && VirtualKeyValue <= VK_NUMPAD9) {

		//
		// С�����ϵ����ּ�0-9��
		//
		if (IsNumLockOn) {
			AsciiChar = VirtualKeyValue - VK_NUMPAD0 + '0';
		}

	} else if (VirtualKeyValue >= VK_MULTIPLY && VirtualKeyValue <= VK_DIVIDE) {

		//
		// С����������� + - * \ . ��
		// ע�⡮.����û��NumLock��ʱ������Del
		//
		if (VK_DECIMAL != VirtualKeyValue || IsNumLockOn) {
			AsciiChar = IopAsciiMapOperator[VirtualKeyValue - VK_MULTIPLY]; 
		}

	} else if (VirtualKeyValue >= VK_OEM_1 && VirtualKeyValue <= VK_OEM_3) {

		//
		// OEM����
		//
		if (!IsShiftDown) {
			AsciiChar = IopAsciiMapOEM1_3[VirtualKeyValue - VK_OEM_1][0];
		} else {
			AsciiChar = IopAsciiMapOEM1_3[VirtualKeyValue - VK_OEM_1][1];
		}

	} else if (VirtualKeyValue >= VK_OEM_4 && VirtualKeyValue <= VK_OEM_7) {

		//
		// OEM����
		//
		if (!IsShiftDown) {
			AsciiChar = IopAsciiMapOEM4_7[VirtualKeyValue - VK_OEM_4][0];
		} else {
			AsciiChar = IopAsciiMapOEM4_7[VirtualKeyValue - VK_OEM_4][1];
		}
	}

	return AsciiChar;
}
