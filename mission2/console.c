/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: console.c

描述: 控制台模块的实现，包括控制台的初始化、打开、读写。



*******************************************************************************/

#include "iop.h"

//
// VGA 控制器的寄存器端口地址。
//
#define CRTC_PORT_ADDR ((PUCHAR)0x03D4)
#define CRTC_PORT_DATA ((PUCHAR)0x03D5)

//
// VGA缓冲位置，和内存映射相关。
//
#define VGA_BUFFER 0x800B8000

//
// 输入缓冲区大小定义。
//
#define INPUT_BUFFER_SIZE 400

//
// 控制台对象结构体。
//
typedef struct _CONSOLE {
	PCHAR ScreenBuffer;					// 输出缓冲区
	CHAR TextAttributes;				// 当前字体颜色
	COORD CursorPosition;				// 当前光标位置
	CHAR InputBuffer[INPUT_BUFFER_SIZE];// 输入缓冲区
	ULONG BytesOfInput;					// 输入缓冲的有效字符数
	ULONG CurrentByteOffset;			// 读取输入缓冲的当前位置
	MUTEX AccessMutex;					// 控制台对象访问互斥信号量
	EVENT InputNotEmptyEvent;			// 输入缓冲区非空事件
	EVENT InputRequestEvent;			// 请求输入事件
} CONSOLE;

//
// 控制台内核对象类型。
//
POBJECT_TYPE IopConsoleType = NULL;

//
// 控制台对象指针数组。
//
PCONSOLE IopConsoleArray[4] = {NULL}; 

//
// 当前活动的控制台指针以及访问指针的互斥信号量。
//
volatile PCONSOLE IopActiveConsole = NULL;
MUTEX IopActiveMutex;

#ifdef _DEBUG
volatile BOOL IsModuleInitialized = FALSE;
#endif


ULONG
IopConsoleDispatchThread(
	IN PVOID Parameter
	);

VOID
IopInitializeConsole(
	VOID
	)
{
	STATUS Status;
	PCONSOLE Console;
	ULONG ConsoleIndex;
	CHAR Title[] = "CONSOLE-1 (Press Ctrl+F1~F4 to switch console window...)";
	ULONG TileLength = strlen(Title);
	HANDLE ThreadHandle;
	ULONG i;

	for (ConsoleIndex = 0; ConsoleIndex < 4; ConsoleIndex++) {

		//
		// 创建控制台对象。
		//
		Status = ObCreateObject( IopConsoleType,
								 NULL,
								 sizeof(CONSOLE),
								 0,
								 (PVOID*)&Console);

		if (!EOS_SUCCESS(Status)) {
			KeBugCheck("Failed to create console!");
		}

		//
		// 初始化控制台对象结构体。
		//
		Console->TextAttributes = 0x0F;
		Console->CursorPosition.X = 0;
		Console->CursorPosition.Y = 1;
		Console->ScreenBuffer = (PCHAR)VGA_BUFFER + 4096 * ConsoleIndex;
		Console->BytesOfInput = 0;
		Console->CurrentByteOffset = 0;

		PsInitializeMutex(&Console->AccessMutex, FALSE);
		PsInitializeEvent(&Console->InputNotEmptyEvent, TRUE, FALSE);
		PsInitializeEvent(&Console->InputRequestEvent, TRUE, FALSE);

		//
		// 初始化视频缓冲区。
		// 第1行为标题行，蓝底高亮白字，后24行为输出行，默认黑底高亮白字。
		//
		for (i = 0; i < 160; ) {
			Console->ScreenBuffer[i++] = ' ';
			Console->ScreenBuffer[i++] = 0x1F;
		}

		while (i < 80 * 25 * 2) {
			Console->ScreenBuffer[i++] = ' ';
			Console->ScreenBuffer[i++] = 0x0F;
		}

		for (i = 0; i < TileLength; i++) {
			Console->ScreenBuffer[i * 2] = Title[i];
		}
		Title[8]++; // 增加标题中序号

		IopConsoleArray[ConsoleIndex] = Console;
	}

	PsInitializeMutex(&IopActiveMutex, FALSE);

	//
	// 创建控制台派遣线程。
	//
	Status = PsCreateThread( PAGE_SIZE,
							 IopConsoleDispatchThread,
							 NULL,
							 FALSE,
							 &ThreadHandle,
							 NULL );
	
	ASSERT(EOS_SUCCESS(Status));

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("Failed to create console thread!");
	}

	ObCloseHandle(ThreadHandle);

#ifdef _DEBUG
	IsModuleInitialized = TRUE;
#endif
}

STATUS
IoOpenConsole(
	IN ULONG ConsoleIndex,
	OUT PHANDLE ConsoleHandle
	)
{
	STATUS Status;

	ASSERT(IsModuleInitialized);

	if (ConsoleIndex < 0 || ConsoleIndex > 3) {
		return STATUS_INVALID_PARAMETER;
	}

	if (NULL == IopConsoleArray[ConsoleIndex]) {
		return STATUS_FILE_NOT_FOUND;
	}

	ObRefObject(IopConsoleArray[ConsoleIndex]);

	Status = ObCreateHandle(IopConsoleArray[ConsoleIndex], ConsoleHandle);

	if (!EOS_SUCCESS(Status)) {
		ObDerefObject(IopConsoleArray[ConsoleIndex]);
	}

	return Status;
}

PRIVATE VOID
IopSetScreenBuffer(
	IN PVOID ScreenBuffer
	)
{
	USHORT pos;

	pos = (USHORT)((ULONG_PTR)ScreenBuffer - VGA_BUFFER) / 2;

	WRITE_PORT_UCHAR(CRTC_PORT_ADDR, 0x0C);
	WRITE_PORT_UCHAR(CRTC_PORT_DATA, (UCHAR)(pos >> 8));
	WRITE_PORT_UCHAR(CRTC_PORT_ADDR, 0x0D);
	WRITE_PORT_UCHAR(CRTC_PORT_DATA, (UCHAR)pos);
}

PRIVATE VOID
IopSetScreenCursor(
	IN PVOID ScreenBuffer,
	IN COORD CursorPosition
	)
{
	USHORT pos;

	ASSERT(CursorPosition.X >= 0 && CursorPosition.X < 80 &&
		CursorPosition.Y >= 0 && CursorPosition.Y < 25);

	pos = (USHORT)((ULONG_PTR)ScreenBuffer - VGA_BUFFER) / 2 + CursorPosition.Y * 80 + CursorPosition.X;

	WRITE_PORT_UCHAR(CRTC_PORT_ADDR, 0x0E);
	WRITE_PORT_UCHAR(CRTC_PORT_DATA, (UCHAR)(pos>>8));
	WRITE_PORT_UCHAR(CRTC_PORT_ADDR, 0x0F);
	WRITE_PORT_UCHAR(CRTC_PORT_DATA, (UCHAR)pos);
}

PRIVATE VOID
IopSetActiveConsole(
	PCONSOLE Console
	)
{
	PsWaitForMutex(&IopActiveMutex, INFINITE);

	if (IopActiveConsole != Console) {

		IopActiveConsole = Console;

		IopSetScreenBuffer(IopActiveConsole->ScreenBuffer);
		IopSetScreenCursor(IopActiveConsole->ScreenBuffer, IopActiveConsole->CursorPosition);
	}

	PsReleaseMutex(&IopActiveMutex);
}

PRIVATE VOID
IopWriteScreenBuffer(
	IN PCHAR ScreenBuffer,
	IN OUT PCOORD Position,
	IN CHAR AsciiChar,
	IN CHAR Attributes
	)
{
	USHORT i; 
	PCHAR ptr;

	ptr = ScreenBuffer + Position->Y * 160 + Position->X * 2;

	switch(AsciiChar) {

		case '\n':

			//
			// 光标移到下一行，列不变。
			//
			Position->Y += 1;
			break;

		case '\t':

			//
			// 输出若干个空格直到下一个制表符位置。
			//
			for (i = 8 - (Position->X & 7); i > 0; i--) {
				*ptr++ = ' ';
				*ptr++ = Attributes;
			}
			Position->X = (Position->X + 8) & ~7;

			break;

		case '\b':

			//
			// 光标向前移动一列，行不变。
			//
			if (Position->X > 0) {
				Position->X--;
			}

			break;

		case '\r':

			//
			// 光标移到第0列，行不变。
			//
			Position->X = 0;

			break;

		default:

			//
			// 在光标当前位置显示字符，并将光标向后移动一列。
			//
			*ptr++ = AsciiChar;
			*ptr = Attributes;
			Position->X++;

			break;
	}

	//
	// 如果光标水平位置超出屏幕范围则换行。
	//
	if (80 == Position->X) {
		Position->X = 0;
		Position->Y++;
	}

	//
	// 如果光标垂直位置超出屏幕范围则将屏幕向上滚动一行。
	//
	if (25 == Position->Y) {

		Position->Y--;
		
		//
		// 跳过第0行的标题行，将2-24行的内容复制到1-23行。
		//
		memcpy(ScreenBuffer + 160, ScreenBuffer + 320, 160 * 23);

		//
		// 将第24行刷新为空，属性为黑底白字。
		//
		ptr = ScreenBuffer + 160 * 24;
		for (i = 0; i < 80; i++) {
			*ptr++ = ' ';
			*ptr++ = 0x0F;
		}
	}
}

STATUS
IopWriteConsoleOutput(
	IN PCONSOLE Console,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	)
{
	ULONG i;

	PsWaitForMutex(&Console->AccessMutex, INFINITE);
	
	for (i = 0; i < NumberOfBytesToWrite; i++) {

		IopWriteScreenBuffer( Console->ScreenBuffer,
							  &Console->CursorPosition,
							  ((PCHAR)Buffer)[i],
							  Console->TextAttributes);
	}

	//
	// 如果控制台是激活的，那么同时还要更新显示器上的光标位置。
	// 注意：要互斥访问变量IopActiveConsole。
	//
	PsWaitForMutex(&IopActiveMutex, INFINITE);

	if (IopActiveConsole == Console) {
		IopSetScreenCursor(Console->ScreenBuffer, Console->CursorPosition);
	}

	PsReleaseMutex(&IopActiveMutex);

	PsReleaseMutex(&Console->AccessMutex);

	*NumberOfBytesWritten = NumberOfBytesToWrite;

	return STATUS_SUCCESS;
}

STATUS
IopReadConsoleInput(
	IN PCONSOLE Console,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	)
{
	PsWaitForMutex(&Console->AccessMutex, INFINITE);

	//
	// 如果缓冲区为空，则设置输入请求事件并等待输入缓冲变为非空。
	//
	if (STATUS_TIMEOUT == PsWaitForEvent(&Console->InputNotEmptyEvent, 0)) {
		PsSetEvent(&Console->InputRequestEvent);
		PsWaitForEvent(&Console->InputNotEmptyEvent, INFINITE);
	}

	//
	// 读取缓冲区。
	//
	if (Console->BytesOfInput - Console->CurrentByteOffset > NumberOfBytesToRead) {
	
		*NumberOfBytesRead = NumberOfBytesToRead;
		memcpy(Buffer, Console->InputBuffer + Console->CurrentByteOffset, NumberOfBytesToRead);

		//
		// 修改当前读取偏移位置。
		//
		Console->CurrentByteOffset += NumberOfBytesToRead;

	} else {

		*NumberOfBytesRead = Console->BytesOfInput - Console->CurrentByteOffset;
		memcpy(Buffer, Console->InputBuffer + Console->CurrentByteOffset, *NumberOfBytesRead);

		//
		// 输入缓冲区已空，复位输入缓冲区。
		//
		Console->BytesOfInput = 0;
		Console->CurrentByteOffset = 0;

		PsResetEvent(&Console->InputNotEmptyEvent);
	}

	PsReleaseMutex(&Console->AccessMutex);

	return STATUS_SUCCESS;
}

VOID
IopWriteConsoleInput(
	IN PCONSOLE Console,
	IN PKEY_EVENT_RECORD KeyEventRecord
	)
{
	CHAR c;
	COORD pos;

	//
	// 如果没有输入请求则立刻返回。
	//
	if (STATUS_TIMEOUT == PsWaitForEvent(&Console->InputRequestEvent, 0)) {
		return;
	}

	//
	// 目前在按键被抬起时没有事情要处理。
	//
	if (!KeyEventRecord->IsKeyDown) {
		return;
	}

	if (VK_BACK == KeyEventRecord->VirtualKeyValue) {

		if (Console->BytesOfInput > 0) {
	
			if (Console->BytesOfInput >= 2 &&
				'\t' == Console->InputBuffer[Console->BytesOfInput - 2]) {

				//
				// 读取制表符回显的空格数，然后退去缓冲区中的制表符。
				//
				c = Console->InputBuffer[Console->BytesOfInput - 1];
				Console->BytesOfInput -= 2;

			} else {

				//
				// 退去缓冲区中的一个普通字符。
				//
				c = 1;
				Console->BytesOfInput -= 1;
			}

			if (Console->CursorPosition.X < c && Console->CursorPosition.Y > 1) {

				//
				// 光标回到上一行，注意，第0行是标题行，不能退去。
				//
				Console->CursorPosition.X = Console->CursorPosition.X + 80 - c;
				Console->CursorPosition.Y--;

			} else {

				Console->CursorPosition.X -= c;
			}

			//
			// 用空格替换被退掉的字符。
			//
			pos = Console->CursorPosition;
			IopWriteScreenBuffer(Console->ScreenBuffer, &pos, ' ', Console->TextAttributes);
			IopSetScreenCursor(Console->ScreenBuffer, Console->CursorPosition);
		}

		return;
	}
	
	if (VK_RETURN == KeyEventRecord->VirtualKeyValue || VK_SEPARATOR == KeyEventRecord->VirtualKeyValue) {
		
		//
		// 行结束符两字节。
		// 行结束符在不同的系统中并不相同：
		// Windows -- \r\n
		// Unix/Linux -- \n
		// MacOS -- \r
		//
		Console->InputBuffer[Console->BytesOfInput++] = '\r';
		Console->InputBuffer[Console->BytesOfInput++] = '\n';

		IopWriteScreenBuffer(Console->ScreenBuffer, &Console->CursorPosition, '\r', Console->TextAttributes);
		IopWriteScreenBuffer(Console->ScreenBuffer, &Console->CursorPosition, '\n', Console->TextAttributes);
		IopSetScreenCursor(Console->ScreenBuffer, Console->CursorPosition);

		//
		// 输入缓冲区中已经有一行数据了，复位输入请求并设置缓冲区非空。
		//
		PsResetEvent(&Console->InputRequestEvent);
		PsSetEvent(&Console->InputNotEmptyEvent);

		return;
	}
	
	if (VK_TAB == KeyEventRecord->VirtualKeyValue && Console->BytesOfInput <= INPUT_BUFFER_SIZE - 4) {

		//
		// 缓冲区至少还空4字节才能接收Tab，1字节存放制表符，1字节存放制表符在回显
		// 的空格数，还要保留2字节给行结束字符\r\n。
		//
		Console->InputBuffer[Console->BytesOfInput++] = '\t';
		Console->InputBuffer[Console->BytesOfInput++] = (CHAR)(8 - (Console->CursorPosition.X & 0x07));

		IopWriteScreenBuffer(Console->ScreenBuffer, &Console->CursorPosition, '\t', Console->TextAttributes);
		IopSetScreenCursor(Console->ScreenBuffer, Console->CursorPosition);

		return;
	}
	
	if (Console->BytesOfInput <= INPUT_BUFFER_SIZE - 3) {

		//
		// 缓冲区至少还空3字节才能接受一个普通可见字符，因为还要保留2字节给行结束符。
		//
		if (VK_SPACE == KeyEventRecord->VirtualKeyValue) {

			c = ' ';

		} else {

			c = TranslateKeyToChar(KeyEventRecord->VirtualKeyValue, KeyEventRecord->ControlKeyState, 0);

			if (0 == c) {
				return;
			}
		}

		Console->InputBuffer[Console->BytesOfInput++] = c;
		IopWriteScreenBuffer(Console->ScreenBuffer, &Console->CursorPosition, c, Console->TextAttributes);
		IopSetScreenCursor(Console->ScreenBuffer, Console->CursorPosition);
	}
}

ULONG
IopConsoleDispatchThread(
	IN PVOID Parameter
	)
/*++

功能描述：
	控制台派遣线程函数。
	线程循环读取键盘输入事件，过滤系统键盘事件，将用户键盘事件派遣给当前活动的控
	制台窗口（将键盘输入写入控制台的输入缓冲区）。
	目前系统键盘事件包括如下：
		Pause：中断内核的运行（仅在调试内核时有效）。
		Ctrl + Shift + Del：暂不进行任何处理；
		Ctrl + F1~F4：设置相应的控制台为当前活动控制台，F1~F4分别对应控制台1~4。

参数：
	Parameter - 无用。

返回值：
	无。

--*/
{
	STATUS Status;
	HANDLE KeyboardHandle;
	ULONG NumberOfBytesRead;
	KEY_EVENT_RECORD KeyEventRecord;

	//
	// 独占打开键盘设备。
	//
	Status = IoCreateFile( "KEYBOARD",
						   GENERIC_READ,
						   0,
						   OPEN_EXISTING,
						   0,
						   &KeyboardHandle );

	if (!EOS_SUCCESS(Status)) {
		KeBugCheck("%s:%d:Failed to open keyboard device!", __FILE__, __LINE__);
	}

	IopSetActiveConsole(IopConsoleArray[0]);

	for(;;) {

		//
		// 读取键盘事件。
		//
		Status = ObRead( KeyboardHandle,
						 &KeyEventRecord,
						 sizeof(KEY_EVENT_RECORD),
						 &NumberOfBytesRead );

		if (!EOS_SUCCESS(Status) || NumberOfBytesRead != sizeof(KEY_EVENT_RECORD)) {
			continue;
		}

		if (KeyEventRecord.IsKeyDown) {

			//
			// 调试内核时，如果Pause被按下则触发一个断点。
			//
#ifdef _DEBUG
			if (VK_PAUSE == KeyEventRecord.VirtualKeyValue) {
				DbgBreakPoint();
				continue;
			}
#endif

			//
			// 如果组合键Ctrl + Shift + Del被按下则由系统处理，目前什么也不做。
			// 注意：小键盘上的 . 在没有NumLock时，也是作为Del键使用的。
			//
			if ((KeyEventRecord.ControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0 &&
				(KeyEventRecord.ControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0 &&
				(VK_DELETE == KeyEventRecord.VirtualKeyValue ||
				VK_DECIMAL == KeyEventRecord.VirtualKeyValue && 
				(KeyEventRecord.ControlKeyState & NUMLOCK_ON) == 0)) {

				continue;
			}

			//
			// 如果Ctrl + F1~F4被按下，则将按键对应的控制台设为激活控制台。
			//
			if ((KeyEventRecord.ControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0 &&
				KeyEventRecord.VirtualKeyValue >= VK_F1 && KeyEventRecord.VirtualKeyValue <= VK_F4) {

				if (NULL != IopConsoleArray[KeyEventRecord.VirtualKeyValue - VK_F1]) {
					IopSetActiveConsole(IopConsoleArray[KeyEventRecord.VirtualKeyValue - VK_F1]);
				}

				continue;
			}
		}

		//
		// 将用户键盘事件发送给当前活动的控制台。
		//
		IopWriteConsoleInput(IopActiveConsole, &KeyEventRecord);
	}
}

STATUS
IoSetConsoleCursorPosition(
	IN HANDLE Handle,
	IN COORD CursorPosition
	)
/*++

功能描述：
	设定控制台窗口的光标位置。

参数：
	Handle - 控制台句柄。
	CursorPosition - 光标位置坐标。

返回值：
	如果成功则返回STATUS_SUCCESS。

--*/
{
	STATUS Status;
	PCONSOLE Console;
	BOOL IntState;

	if (CursorPosition.X < 0 || CursorPosition.X >= 80 ||
		CursorPosition.Y < 0 || CursorPosition.Y >= 24) {
		return STATUS_INVALID_PARAMETER;
	}

	// 跳过标题行
	CursorPosition.Y++;

	Status = ObRefObjectByHandle(Handle, IopConsoleType, (PVOID*)&Console);

	if (EOS_SUCCESS(Status)) {

		IntState = KeEnableInterrupts(FALSE);

		Console->CursorPosition = CursorPosition;

		if (Console == IopActiveConsole) {
			IopSetScreenCursor(Console->ScreenBuffer, CursorPosition);
		}

		KeEnableInterrupts(IntState);
		
		ObDerefObject(Console);
	}

	return Status;
}

#ifdef _DEBUG

int InputBufCount;

void GetInputBufferCount()
{
	int i = 0;
	BOOL IntState;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	for(i = INPUT_BUFFER_SIZE - 1; i > 0; i--)
	{
		if((*IopConsoleArray[0]).InputBuffer[i] != 0)
		{
			break;
		}
	}
	InputBufCount = i - 1;
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

int OutputBufCount;

void GetOutputBufferCount()
{
	int i = 0;
	BOOL IntState;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// 关中断
	
	for(i = 4095; i > 0; i = i - 2)
	{
		if((*IopConsoleArray[0]).ScreenBuffer[i] != 7 && (*IopConsoleArray[0]).ScreenBuffer[i-1] != 32)
		{
			break;
		}
	}
	OutputBufCount = i ;
	
	KeEnableInterrupts(IntState);	// 开中断
	StopKeyboard = 0;
}

#endif
