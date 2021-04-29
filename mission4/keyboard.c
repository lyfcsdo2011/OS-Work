/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: keyboard.c

描述: 键盘驱动的实现。



*******************************************************************************/

#include "iop.h"

//
// 8042的寄存器端口地址
//
#define KEYBOARD_PORT_DATA		((PUCHAR)0x60)
#define KEYBOARD_PORT_STATUS	((PUCHAR)0x64)

typedef struct _KEYBOARD_DEVICE_EXTENSION {
	ULONG CtrlKeyState;			// Ctrl、Shift、CapsLK等按键的状态
	PRING_BUFFER Buffer;		// 按键动作缓冲区
	EVENT BufferEvent;			// 缓冲区非空事件，当缓冲区非空时处于signaled状态
}KEYBOARD_DEVICE_EXTENSION, *PKEYBOARD_DEVICE_EXTENSION;

//
// 目前仅支持一个键盘设备
//
PDEVICE_OBJECT KbdDevice[1] = {NULL};

//
// 单字节键盘扫描码对应的虚键查询表。
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
// 双字节键盘扫描码对应的虚键查询表。
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
// 键盘驱动对象的初始化函数。
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
// 键盘驱动提供的AddDevice功能函数。
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
	// 键盘没有下层设备且仅支持一个键盘。
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
		// 初始化扩展块。
		//
		Ext = (PKEYBOARD_DEVICE_EXTENSION)(*DeviceObject)->DeviceExtension;
		Ext->CtrlKeyState = 0;
		Ext->Buffer = IopCreateRingBuffer(sizeof(KEY_EVENT_RECORD) * 16);
		PsInitializeEvent(&Ext->BufferEvent, TRUE, FALSE);

		//
		// 更新键盘LED指示灯的状态。
		//
		KbdUpdateLeds(DeviceNumber);

		//
		// 设置中断向量，取消8259可编程控制器对键盘中断的屏蔽。
		//
		KeIsrKeyBoard = KbdIsr;
		KeEnableDeviceInterrupt(INT_KEYBOARD, TRUE);

		//
		// 这里没有对键盘做多余操作，直接采用BIOS对键盘的初始化结果。
		//
	}

	return Status;
}

//
// 键盘驱动提供的Create功能函数。
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
	// 键盘设备已经存在，只能使用OPEN_EXISTING标志。
	//
	if (OPEN_EXISTING != CreationDisposition) {
		return STATUS_INVALID_PARAMETER;
	}

	//
	// 键盘设备只读。
	//
	if (FileObject->WriteAccess) {
		return STATUS_ACCESS_DENIED;
	}

	FileObject->FsContext = DeviceObject;
	return STATUS_SUCCESS;
}

//
// 键盘驱动程序提供的Read功能函数。
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
	// 读写大小须是键盘事件结构体大小的整数倍。
	//
	Request -= Request % sizeof(KEY_EVENT_RECORD);

	while (Count < Request) {

		//
		// 阻塞等待直到缓冲区非空。
		//
		PsWaitForEvent(&Ext->BufferEvent, INFINITE);

		//
		// 读取缓冲区，如果缓冲区被读空了则复位非空事件。
		// 注意，要和键盘中断服务程序互斥访问键盘事件缓冲区，要禁止中断。
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
// 键盘(8042)中断服务程序。
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
	// 从8042数据端口读取键盘扫描码。
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
		// 如果第一个扫描码是扩展码则立刻返回，继续获取下一个扫描码。
		//
		if (0xE0 == ScanCode[0] || 0xE1 == ScanCode[0]) {
			return;
		}

		//
		// 记录按键动作。
		//
		KeyEventRecord.IsKeyDown = ((ScanCode[0] & 0x80) == 0);
		ScanCode[0] &= 0x7F;

		//
		// 查询按键对应的VK值。
		//
		if (ScanCode[0] <= 0x58) {
			KeyEventRecord.VirtualKeyValue = KbdVirtualKeyMap[ScanCode[0]];
		}

		break;

	case 2:

		//
		// 如果是3字节扫描码则立刻返回，继续获取下一个扫描码。
		//
		if (0xE1 == ScanCode[0]) {
			return;
		}
		
		//
		// 记录按键动作。
		//
		KeyEventRecord.IsKeyDown = ((ScanCode[1] & 0x80) == 0);
		ScanCode[1] &= 0x7F;

		//
		// 查询按键对应的VK值。
		// 注意：PrtScr键被简化为Make Code为0xE0,0x37而Break Code为OxE0,0xB7来处
		// 理，其余的0xE0,0x2A,0xE0,0xAA被忽略。
		//
		if (0x1C <= ScanCode[1] && ScanCode[1] <= 0x5C) {
			KeyEventRecord.VirtualKeyValue = KbdVirtualKeyMapExt[ScanCode[1] - 0x1C];
		}

		break;

	case 3:

		//
		// 只有PAUSE键是3字节扫描码，PAUSE键被按下同时产生Make Code和Break Code，
		// 而PAUSE键被抬起则没有任何扫描吗产生，所以我们可忽略BreakCode。
		//
		if (0x1D == ScanCode[1] && 0x45 == ScanCode[2]) {
			KeyEventRecord.IsKeyDown = TRUE;
			KeyEventRecord.VirtualKeyValue = VK_PAUSE;
		}

		break;
	}

	//
	// 复位扫描码计数器。
	//
	i = 0;

	//
	// 忽略未识别按键。
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
				// 锁定键被按下，改变锁定状态并更新键盘LED灯。
				//
				Ext->CtrlKeyState ^= ControlKeyStateMask;
				KbdUpdateLeds(0);
			}

		} else {

			//
			// 记录ctrl、shift等键的按下状态。
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
	// 将键盘事件写入缓冲区，并设置键盘非空事件。
	//
	IopWriteRingBuffer(Ext->Buffer, &KeyEventRecord, sizeof(KEY_EVENT_RECORD));
	PsSetEvent(&Ext->BufferEvent);
}

//
// 根据功能键状态点亮指定键盘的LED指示灯。
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
	// 注意while()之后有‘;’
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

功能描述：
	获取键盘的信息。

参数：
	

返回值：
	无。

--*/
{
	BOOL IntState;
	PTHREAD pThread;	
	PLIST_ENTRY pWListEntry;
	INT WaitThreadNum = 0;
	
	const char* ThreadState = NULL;
	
	StopKeyboard = 1;	
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
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
			// 获得线程对象的指针
			//
			pThread = CONTAINING_RECORD(pWListEntry, THREAD, StateListEntry);
			
			KeyBoardEvent.WaitObj[WaitThreadNum] = ObGetObjectId(pThread);
		}
		KeyBoardEvent.Count = WaitThreadNum;
	}
				
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;	
}

#endif

