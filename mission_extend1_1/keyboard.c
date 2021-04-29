/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: keyboard.c

����: ����������ʵ�֡�



*******************************************************************************/

#include "iop.h"

//
// 8042�ļĴ����˿ڵ�ַ
//
#define KEYBOARD_PORT_DATA		((PUCHAR)0x60)
#define KEYBOARD_PORT_STATUS	((PUCHAR)0x64)

typedef struct _KEYBOARD_DEVICE_EXTENSION {
	ULONG CtrlKeyState;			// Ctrl��Shift��CapsLK�Ȱ�����״̬
	PRING_BUFFER Buffer;		// ��������������
	EVENT BufferEvent;			// �������ǿ��¼������������ǿ�ʱ����signaled״̬
}KEYBOARD_DEVICE_EXTENSION, *PKEYBOARD_DEVICE_EXTENSION;

//
// Ŀǰ��֧��һ�������豸
//
PDEVICE_OBJECT KbdDevice[1] = {NULL};

//
// ���ֽڼ���ɨ�����Ӧ�������ѯ��
//
UCHAR KbdVirtualKeyMap[0x59] = {
	/* 0x00 */	0,
	/* 0x01 */	VK_ESCAPE,
	/* 0x02 */	'1',
	/* 0x03 */	'2',
	/* 0x04 */	'3',
	/* 0x05 */	'4',
	/* 0x06 */	'5',
	/* 0x07 */	'6',
	/* 0x08 */	'7',
	/* 0x09 */	'8',
	/* 0x0A */	'9',
	/* 0x0B */	'0',
	/* 0x0C */	VK_OEM_MINUS,
	/* 0x0D */	VK_OEM_PLUS,
	/* 0x0E */	VK_BACK,
	/* 0x0F */	VK_TAB,
	/* 0x10 */	'Q',
	/* 0x11 */	'W',
	/* 0x12 */	'E',
	/* 0x13 */	'R',
	/* 0x14 */	'T',
	/* 0x15 */	'Y',
	/* 0x16 */	'U',
	/* 0x17 */	'I',
	/* 0x18 */	'O',
	/* 0x19 */	'P',
	/* 0x1A */	VK_OEM_4,
	/* 0x1B */	VK_OEM_6,
	/* 0x1C */	VK_RETURN,
	/* 0x1D */	VK_LCONTROL,
	/* 0x1E */	'A',
	/* 0x1F */	'S',
	/* 0x20 */	'D',
	/* 0x21 */	'F',
	/* 0x22 */	'G',
	/* 0x23 */	'H',
	/* 0x24 */	'J',
	/* 0x25 */	'K',
	/* 0x26 */	'L',
	/* 0x27 */	VK_OEM_1,
	/* 0x28 */	VK_OEM_7,
	/* 0x29 */	VK_OEM_3,
	/* 0x2A */	VK_LSHIFT,
	/* 0x2B */	VK_OEM_5,
	/* 0x2C */	'Z',
	/* 0x2D */	'X',
	/* 0x2E */	'C',
	/* 0x2F */	'V',
	/* 0x30 */	'B',
	/* 0x31 */	'N',
	/* 0x32 */	'M',
	/* 0x33 */	VK_OEM_COMMA,
	/* 0x34 */	VK_OEM_PERIOD,
	/* 0x35 */	VK_OEM_2,
	/* 0x36 */	VK_RSHIFT,
	/* 0x37 */	VK_MULTIPLY,
	/* 0x38 */	VK_LMENU,
	/* 0x39 */	VK_SPACE,
	/* 0x3A */	VK_CAPITAL,
	/* 0x3B */	VK_F1,
	/* 0x3C */	VK_F2,
	/* 0x3D */	VK_F3,
	/* 0x3E */	VK_F4,
	/* 0x3F */	VK_F5,
	/* 0x40 */	VK_F6,
	/* 0x41 */	VK_F7,
	/* 0x42 */	VK_F8,
	/* 0x43 */	VK_F9,
	/* 0x44 */	VK_F10,
	/* 0x45 */	VK_NUMLOCK,
	/* 0x46 */	VK_SCROLL,
	/* 0x47 */	VK_NUMPAD7,
	/* 0x48 */	VK_NUMPAD8,
	/* 0x49 */	VK_NUMPAD9,
	/* 0x4A */	VK_SUBTRACT,
	/* 0x4B */	VK_NUMPAD4,
	/* 0x4C */	VK_NUMPAD5,
	/* 0x4D */	VK_NUMPAD6,
	/* 0x4E */	VK_ADD,
	/* 0x4F */	VK_NUMPAD1,
	/* 0x50 */	VK_NUMPAD2,
	/* 0x51 */	VK_NUMPAD3,
	/* 0x52 */	VK_NUMPAD0,
	/* 0x53 */	VK_DECIMAL,
	/* 0x54 */	VK_PRINT,	// Alt + PtrScr
	/* 0x55 */	0,
	/* 0x56 */	0,
	/* 0x57 */	VK_F11,
	/* 0x58 */	VK_F12,	
};

//
// ˫�ֽڼ���ɨ�����Ӧ�������ѯ��
//
UCHAR KbdVirtualKeyMapExt[128] = {
	/* 0x1C */	VK_SEPARATOR,
	/* 0x1D */	VK_RCONTROL,
	/* 0x1E */	0,
	/* 0x1F */	0,
	/* 0x20 */	0,
	/* 0x21 */	0,
	/* 0x22 */	0,
	/* 0x23 */	0,
	/* 0x24 */	0,
	/* 0x25 */	0,
	/* 0x26 */	0,
	/* 0x27 */	0,
	/* 0x28 */	0,
	/* 0x29 */	0,
	/* 0x2A */	0,
	/* 0x2B */	0,
	/* 0x2C */	0,
	/* 0x2D */	0,
	/* 0x2E */	0,
	/* 0x2F */	0,
	/* 0x30 */	0,
	/* 0x31 */	0,
	/* 0x32 */	0,
	/* 0x33 */	0,
	/* 0x34 */	0,
	/* 0x35 */	VK_DIVIDE,
	/* 0x36 */	0,
	/* 0x37 */	VK_PRINT,
	/* 0x38 */	VK_RMENU,
	/* 0x39 */	0,
	/* 0x3A */	0,
	/* 0x3B */	0,
	/* 0x3C */	0,
	/* 0x3D */	0,
	/* 0x3E */	0,
	/* 0x3F */	0,
	/* 0x40 */	0,
	/* 0x41 */	0,
	/* 0x42 */	0,
	/* 0x43 */	0,
	/* 0x44 */	0,
	/* 0x45 */	0,
	/* 0x46 */	VK_PAUSE,	// Ctrl + Pause
	/* 0x47 */	VK_HOME,
	/* 0x48 */	VK_UP,
	/* 0x49 */	VK_PRIOR,
	/* 0x4A */	0,
	/* 0x4B */	VK_LEFT,
	/* 0x4C */	0,
	/* 0x4D */	VK_RIGHT,
	/* 0x4E */	0,
	/* 0x4F */	VK_END,
	/* 0x50 */	VK_DOWN,
	/* 0x51 */	VK_NEXT,
	/* 0x52 */	VK_INSERT,
	/* 0x53 */	VK_DELETE,
	/* 0x54 */	0,
	/* 0x55 */	0,
	/* 0x56 */	0,
	/* 0x57 */	0,
	/* 0x58 */	0,
	/* 0x59 */	0,
	/* 0x5A */	0,
	/* 0x5B */	VK_LWIN,	
	/* 0x5C */	VK_RWIN,
};

STATUS
KbdAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT NextLayerDevice,
	IN USHORT DeviceNumber,
	OUT PDEVICE_OBJECT *DeviceObject
	);

STATUS
KbdCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN ULONG CreationDisposition,
	IN OUT PFILE_OBJECT FileObject
	);

STATUS
KbdRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	);

VOID
KbdIsr(
	VOID
	);

VOID
KbdUpdateLeds(
	USHORT DeviceNumber
	);

//
// ������������ĳ�ʼ��������
//
VOID
KbdInitializeDriver(
	PDRIVER_OBJECT DriverObject
	)
{
	DriverObject->AddDevice = KbdAddDevice;
	DriverObject->Create = KbdCreate;
	DriverObject->Read = KbdRead;
}

//
// ���������ṩ��AddDevice���ܺ�����
//
STATUS
KbdAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT NextLayerDevice,
	IN USHORT DeviceNumber,
	OUT PDEVICE_OBJECT *DeviceObject
	)
{
	STATUS Status;
	PKEYBOARD_DEVICE_EXTENSION Ext;

	//
	// ����û���²��豸�ҽ�֧��һ�����̡�
	//
	ASSERT(NULL == NextLayerDevice);
	ASSERT(0 == DeviceNumber && NULL == KbdDevice[DeviceNumber]);

	Status = IopCreateDevice( DriverObject,
							  sizeof(KEYBOARD_DEVICE_EXTENSION),
							  "KEYBOARD",
							  DeviceNumber,
							  FALSE,
							  DeviceObject );

	if (EOS_SUCCESS(Status)) {

		KbdDevice[DeviceNumber] = *DeviceObject;

		//
		// ��ʼ����չ�顣
		//
		Ext = (PKEYBOARD_DEVICE_EXTENSION)(*DeviceObject)->DeviceExtension;
		Ext->CtrlKeyState = 0;
		Ext->Buffer = IopCreateRingBuffer(sizeof(KEY_EVENT_RECORD) * 16);
		PsInitializeEvent(&Ext->BufferEvent, TRUE, FALSE);

		//
		// ���¼���LEDָʾ�Ƶ�״̬��
		//
		KbdUpdateLeds(DeviceNumber);

		//
		// �����ж�������ȡ��8259�ɱ�̿������Լ����жϵ����Ρ�
		//
		KeIsrKeyBoard = KbdIsr;
		KeEnableDeviceInterrupt(INT_KEYBOARD, TRUE);

		//
		// ����û�жԼ��������������ֱ�Ӳ���BIOS�Լ��̵ĳ�ʼ�������
		//
	}

	return Status;
}

//
// ���������ṩ��Create���ܺ�����
//
STATUS
KbdCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCSTR FileName,
	IN ULONG CreationDisposition,
	IN OUT PFILE_OBJECT FileObject
	)
{
	PKEYBOARD_DEVICE_EXTENSION Ext = (PKEYBOARD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (*FileName != 0) {
		return STATUS_PATH_NOT_FOUND;
	}

	//
	// �����豸�Ѿ����ڣ�ֻ��ʹ��OPEN_EXISTING��־��
	//
	if (OPEN_EXISTING != CreationDisposition) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// �����豸ֻ����
	//
	if (FileObject->WriteAccess) {
		return STATUS_ACCESS_DENIED;
	}

	FileObject->FsContext = DeviceObject;
	return STATUS_SUCCESS;
}

//
// �������������ṩ��Read���ܺ�����
//
STATUS
KbdRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PFILE_OBJECT FileObject,
	OUT PVOID Buffer,
	IN ULONG Request,
	OUT PULONG Result OPTIONAL
	)
{
	BOOL IntState;
	ULONG Count = 0;
	PKEYBOARD_DEVICE_EXTENSION Ext = (PKEYBOARD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	//
	// ��д��С���Ǽ����¼��ṹ���С����������
	//
	Request -= Request % sizeof(KEY_EVENT_RECORD);

	while (Count < Request) {

		//
		// �����ȴ�ֱ���������ǿա�
		//
		PsWaitForEvent(&Ext->BufferEvent, INFINITE);

		//
		// ��ȡ���������������������������λ�ǿ��¼���
		// ע�⣬Ҫ�ͼ����жϷ�����򻥳���ʼ����¼���������Ҫ��ֹ�жϡ�
		//
		IntState = KeEnableInterrupts(FALSE);
		
		Count += IopReadRingBuffer(Ext->Buffer, Buffer + Count, Request - Count);

		if (IopIsRingBufferEmpty(Ext->Buffer)) {
			PsResetEvent(&Ext->BufferEvent);
		}
		
		KeEnableInterrupts(IntState);
	}
	
	*Result = Count;
	return STATUS_SUCCESS;
}
#ifdef _DEBUG

volatile ULONG StopKeyboard = 0;

#endif
//
// ����(8042)�жϷ������
//
VOID
KbdIsr(
	VOID
	)
{
	static UCHAR ScanCode[3];
	static UCHAR i = 0;
	KEY_EVENT_RECORD KeyEventRecord;
	ULONG ControlKeyStateMask;
	PKEYBOARD_DEVICE_EXTENSION Ext = (PKEYBOARD_DEVICE_EXTENSION)KbdDevice[0]->DeviceExtension;

	//
	// ��8042���ݶ˿ڶ�ȡ����ɨ���롣
	//
	ScanCode[i] = READ_PORT_UCHAR((PUCHAR)KEYBOARD_PORT_DATA);
	i++;
	
#ifdef _DEBUG

	if(StopKeyboard)
	{
		StopKeyboard = 0;
		i = 0;
		return;
	}
	
#endif
	
	KeyEventRecord.VirtualKeyValue = 0;

	switch(i) {

	case 1:

		//
		// �����һ��ɨ��������չ�������̷��أ�������ȡ��һ��ɨ���롣
		//
		if (0xE0 == ScanCode[0] || 0xE1 == ScanCode[0]) {
			return;
		}

		//
		// ��¼����������
		//
		KeyEventRecord.IsKeyDown = ((ScanCode[0] & 0x80) == 0);
		ScanCode[0] &= 0x7F;

		//
		// ��ѯ������Ӧ��VKֵ��
		//
		if (ScanCode[0] <= 0x58) {
			KeyEventRecord.VirtualKeyValue = KbdVirtualKeyMap[ScanCode[0]];
		}

		break;

	case 2:

		//
		// �����3�ֽ�ɨ���������̷��أ�������ȡ��һ��ɨ���롣
		//
		if (0xE1 == ScanCode[0]) {
			return;
		}
		
		//
		// ��¼����������
		//
		KeyEventRecord.IsKeyDown = ((ScanCode[1] & 0x80) == 0);
		ScanCode[1] &= 0x7F;

		//
		// ��ѯ������Ӧ��VKֵ��
		// ע�⣺PrtScr������ΪMake CodeΪ0xE0,0x37��Break CodeΪOxE0,0xB7����
		// �������0xE0,0x2A,0xE0,0xAA�����ԡ�
		//
		if (0x1C <= ScanCode[1] && ScanCode[1] <= 0x5C) {
			KeyEventRecord.VirtualKeyValue = KbdVirtualKeyMapExt[ScanCode[1] - 0x1C];
		}

		break;

	case 3:

		//
		// ֻ��PAUSE����3�ֽ�ɨ���룬PAUSE��������ͬʱ����Make Code��Break Code��
		// ��PAUSE����̧����û���κ�ɨ����������������ǿɺ���BreakCode��
		//
		if (0x1D == ScanCode[1] && 0x45 == ScanCode[2]) {
			KeyEventRecord.IsKeyDown = TRUE;
			KeyEventRecord.VirtualKeyValue = VK_PAUSE;
		}

		break;
	}

	//
	// ��λɨ�����������
	//
	i = 0;

	//
	// ����δʶ�𰴼���
	//
	if (0 == KeyEventRecord.VirtualKeyValue) {
		return;
	}

	switch (KeyEventRecord.VirtualKeyValue) {

		case VK_RMENU:
			ControlKeyStateMask = RIGHT_ALT_PRESSED;
			break;

		case VK_LMENU:
			ControlKeyStateMask = LEFT_ALT_PRESSED;
			break;

		case VK_RCONTROL:
			ControlKeyStateMask = RIGHT_CTRL_PRESSED;
			break;

		case VK_LCONTROL:
			ControlKeyStateMask = LEFT_CTRL_PRESSED;
			break;

		case VK_RSHIFT:
		case VK_LSHIFT:
			ControlKeyStateMask = SHIFT_PRESSED;
			break;

		case VK_CAPITAL:
			ControlKeyStateMask = CAPSLOCK_ON;
			break;

		case VK_NUMLOCK:
			ControlKeyStateMask = NUMLOCK_ON;
			break;

		case VK_SCROLL:
			ControlKeyStateMask = SCROLLLOCK_ON;
			break;

		default:
			ControlKeyStateMask = 0;
			break;
	}

	if (ControlKeyStateMask != 0) {

		if ((ControlKeyStateMask & (CAPSLOCK_ON | NUMLOCK_ON | SCROLLLOCK_ON)) != 0) {

			if (KeyEventRecord.IsKeyDown) {

				//
				// �����������£��ı�����״̬�����¼���LED�ơ�
				//
				Ext->CtrlKeyState ^= ControlKeyStateMask;
				KbdUpdateLeds(0);
			}

		} else {

			//
			// ��¼ctrl��shift�ȼ��İ���״̬��
			//
			if (KeyEventRecord.IsKeyDown) {
				Ext->CtrlKeyState |= ControlKeyStateMask;
			} else {
				Ext->CtrlKeyState &= ~ControlKeyStateMask;
			}
		}
	}

	KeyEventRecord.ControlKeyState = Ext->CtrlKeyState;

	//
	// �������¼�д�뻺�����������ü��̷ǿ��¼���
	//
	IopWriteRingBuffer(Ext->Buffer, &KeyEventRecord, sizeof(KEY_EVENT_RECORD));
	PsSetEvent(&Ext->BufferEvent);
}

//
// ���ݹ��ܼ�״̬����ָ�����̵�LEDָʾ�ơ�
//
VOID
KbdUpdateLeds(
	USHORT DeviceNumber
	)
{
	UCHAR c = 0;
	PKEYBOARD_DEVICE_EXTENSION Ext = (PKEYBOARD_DEVICE_EXTENSION)KbdDevice[DeviceNumber]->DeviceExtension;

	if ((Ext->CtrlKeyState & CAPSLOCK_ON) != 0) {
		c |= 0x04;
	}
	if ((Ext->CtrlKeyState & NUMLOCK_ON) != 0) {
		c |= 0x02;
	}
	if ((Ext->CtrlKeyState & SCROLLLOCK_ON) != 0) {
		c |= 0x01;
	}

	//
	// ע��while()֮���С�;��
	//
	while((READ_PORT_UCHAR(KEYBOARD_PORT_STATUS) & 0x02) != 0);
	WRITE_PORT_UCHAR(KEYBOARD_PORT_DATA, 0xED);
	while(READ_PORT_UCHAR(KEYBOARD_PORT_DATA) != 0xFA);
	while((READ_PORT_UCHAR(KEYBOARD_PORT_STATUS) & 0x02) != 0);
	WRITE_PORT_UCHAR(KEYBOARD_PORT_DATA, c);
	while((READ_PORT_UCHAR(KEYBOARD_PORT_STATUS) & 0x02) != 0);
}

#define MAXKEYBOARDEVENTWAITCOUNT 10
typedef struct _KEYBOARDEVENT
{
	INT WaitObj[MAXKEYBOARDEVENTWAITCOUNT];
	INT Count;	
}KEYBOARDEVENT;

KEYBOARDEVENT KeyBoardEvent;

#ifdef _DEBUG

PRIVATE VOID GetKeyBoard( )
/*++

����������
	��ȡ���̵���Ϣ��

������
	

����ֵ��
	�ޡ�

--*/
{
	BOOL IntState;
	PTHREAD pThread;	
	PLIST_ENTRY pWListEntry;
	INT WaitThreadNum = 0;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	if(KbdDevice[0] != 0
		&& (*(PKEYBOARD_DEVICE_EXTENSION)(KbdDevice[0]-> DeviceExtension)).Buffer != 0)
	{
		for(WaitThreadNum = 0,
		pWListEntry = (*(PKEYBOARD_DEVICE_EXTENSION)(KbdDevice[0]-> DeviceExtension)).BufferEvent.WaitListHead.Next;
		pWListEntry != NULL
		&& pWListEntry != &((*(PKEYBOARD_DEVICE_EXTENSION)(KbdDevice[0]-> DeviceExtension)).BufferEvent.WaitListHead) 
		&& WaitThreadNum < MAXKEYBOARDEVENTWAITCOUNT;
		pWListEntry = pWListEntry->Next, WaitThreadNum++)
		{
			//
			// ����̶߳����ָ��
			//
			pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
			
			KeyBoardEvent.WaitObj[WaitThreadNum] = ObGetObjectId(pThread);
		}
		KeyBoardEvent.Count = WaitThreadNum;
	}
				
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;	
}

#endif

