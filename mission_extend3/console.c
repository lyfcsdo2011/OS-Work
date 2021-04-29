/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: console.c

����: ����̨ģ���ʵ�֣���������̨�ĳ�ʼ�����򿪡���д��



*******************************************************************************/

#include "iop.h"

//
// VGA �������ļĴ����˿ڵ�ַ��
//
#define CRTC_PORT_ADDR ((PUCHAR)0x03D4)
#define CRTC_PORT_DATA ((PUCHAR)0x03D5)

//
// VGA����λ�ã����ڴ�ӳ����ء�
//
#define VGA_BUFFER 0x800B8000

//
// ���뻺������С���塣
//
#define INPUT_BUFFER_SIZE 400

//
// ����̨����ṹ�塣
//
typedef struct _CONSOLE {
	PCHAR ScreenBuffer;					// ���������
	CHAR TextAttributes;				// ��ǰ������ɫ
	COORD CursorPosition;				// ��ǰ���λ��
	CHAR InputBuffer[INPUT_BUFFER_SIZE];// ���뻺����
	ULONG BytesOfInput;					// ���뻺�����Ч�ַ���
	ULONG CurrentByteOffset;			// ��ȡ���뻺��ĵ�ǰλ��
	MUTEX AccessMutex;					// ����̨������ʻ����ź���
	EVENT InputNotEmptyEvent;			// ���뻺�����ǿ��¼�
	EVENT InputRequestEvent;			// ���������¼�
} CONSOLE;

//
// ����̨�ں˶������͡�
//
POBJECT_TYPE IopConsoleType = NULL;

//
// ����̨����ָ�����顣
//
PCONSOLE IopConsoleArray[4] = {NULL}; 

//
// ��ǰ��Ŀ���ָ̨���Լ�����ָ��Ļ����ź�����
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
		// ��������̨����
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
		// ��ʼ������̨����ṹ�塣
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
		// ��ʼ����Ƶ��������
		// ��1��Ϊ�����У����׸������֣���24��Ϊ����У�Ĭ�Ϻڵ׸������֡�
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
		Title[8]++; // ���ӱ��������

		IopConsoleArray[ConsoleIndex] = Console;
	}

	PsInitializeMutex(&IopActiveMutex, FALSE);

	//
	// ��������̨��ǲ�̡߳�
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
			// ����Ƶ���һ�У��в��䡣
			//
			Position->Y += 1;
			break;

		case '\t':

			//
			// ������ɸ��ո�ֱ����һ���Ʊ��λ�á�
			//
			for (i = 8 - (Position->X & 7); i > 0; i--) {
				*ptr++ = ' ';
				*ptr++ = Attributes;
			}
			Position->X = (Position->X + 8) & ~7;

			break;

		case '\b':

			//
			// �����ǰ�ƶ�һ�У��в��䡣
			//
			if (Position->X > 0) {
				Position->X--;
			}

			break;

		case '\r':

			//
			// ����Ƶ���0�У��в��䡣
			//
			Position->X = 0;

			break;

		default:

			//
			// �ڹ�굱ǰλ����ʾ�ַ��������������ƶ�һ�С�
			//
			*ptr++ = AsciiChar;
			*ptr = Attributes;
			Position->X++;

			break;
	}

	//
	// ������ˮƽλ�ó�����Ļ��Χ���С�
	//
	if (80 == Position->X) {
		Position->X = 0;
		Position->Y++;
	}

	//
	// �����괹ֱλ�ó�����Ļ��Χ����Ļ���Ϲ���һ�С�
	//
	if (25 == Position->Y) {

		Position->Y--;
		
		//
		// ������0�еı����У���2-24�е����ݸ��Ƶ�1-23�С�
		//
		memcpy(ScreenBuffer + 160, ScreenBuffer + 320, 160 * 23);

		//
		// ����24��ˢ��Ϊ�գ�����Ϊ�ڵװ��֡�
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
	// �������̨�Ǽ���ģ���ôͬʱ��Ҫ������ʾ���ϵĹ��λ�á�
	// ע�⣺Ҫ������ʱ���IopActiveConsole��
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
	// ���������Ϊ�գ����������������¼����ȴ����뻺���Ϊ�ǿա�
	//
	if (STATUS_TIMEOUT == PsWaitForEvent(&Console->InputNotEmptyEvent, 0)) {
		PsSetEvent(&Console->InputRequestEvent);
		PsWaitForEvent(&Console->InputNotEmptyEvent, INFINITE);
	}

	//
	// ��ȡ��������
	//
	if (Console->BytesOfInput - Console->CurrentByteOffset > NumberOfBytesToRead) {
	
		*NumberOfBytesRead = NumberOfBytesToRead;
		memcpy(Buffer, Console->InputBuffer + Console->CurrentByteOffset, NumberOfBytesToRead);

		//
		// �޸ĵ�ǰ��ȡƫ��λ�á�
		//
		Console->CurrentByteOffset += NumberOfBytesToRead;

	} else {

		*NumberOfBytesRead = Console->BytesOfInput - Console->CurrentByteOffset;
		memcpy(Buffer, Console->InputBuffer + Console->CurrentByteOffset, *NumberOfBytesRead);

		//
		// ���뻺�����ѿգ���λ���뻺������
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
	// ���û���������������̷��ء�
	//
	if (STATUS_TIMEOUT == PsWaitForEvent(&Console->InputRequestEvent, 0)) {
		return;
	}

	//
	// Ŀǰ�ڰ�����̧��ʱû������Ҫ����
	//
	if (!KeyEventRecord->IsKeyDown) {
		return;
	}

	if (VK_BACK == KeyEventRecord->VirtualKeyValue) {

		if (Console->BytesOfInput > 0) {
	
			if (Console->BytesOfInput >= 2 &&
				'\t' == Console->InputBuffer[Console->BytesOfInput - 2]) {

				//
				// ��ȡ�Ʊ�����ԵĿո�����Ȼ����ȥ�������е��Ʊ����
				//
				c = Console->InputBuffer[Console->BytesOfInput - 1];
				Console->BytesOfInput -= 2;

			} else {

				//
				// ��ȥ�������е�һ����ͨ�ַ���
				//
				c = 1;
				Console->BytesOfInput -= 1;
			}

			if (Console->CursorPosition.X < c && Console->CursorPosition.Y > 1) {

				//
				// ���ص���һ�У�ע�⣬��0���Ǳ����У�������ȥ��
				//
				Console->CursorPosition.X = Console->CursorPosition.X + 80 - c;
				Console->CursorPosition.Y--;

			} else {

				Console->CursorPosition.X -= c;
			}

			//
			// �ÿո��滻���˵����ַ���
			//
			pos = Console->CursorPosition;
			IopWriteScreenBuffer(Console->ScreenBuffer, &pos, ' ', Console->TextAttributes);
			IopSetScreenCursor(Console->ScreenBuffer, Console->CursorPosition);
		}

		return;
	}
	
	if (VK_RETURN == KeyEventRecord->VirtualKeyValue || VK_SEPARATOR == KeyEventRecord->VirtualKeyValue) {
		
		//
		// �н��������ֽڡ�
		// �н������ڲ�ͬ��ϵͳ�в�����ͬ��
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
		// ���뻺�������Ѿ���һ�������ˣ���λ�����������û������ǿա�
		//
		PsResetEvent(&Console->InputRequestEvent);
		PsSetEvent(&Console->InputNotEmptyEvent);

		return;
	}
	
	if (VK_TAB == KeyEventRecord->VirtualKeyValue && Console->BytesOfInput <= INPUT_BUFFER_SIZE - 4) {

		//
		// ���������ٻ���4�ֽڲ��ܽ���Tab��1�ֽڴ���Ʊ����1�ֽڴ���Ʊ���ڻ���
		// �Ŀո�������Ҫ����2�ֽڸ��н����ַ�\r\n��
		//
		Console->InputBuffer[Console->BytesOfInput++] = '\t';
		Console->InputBuffer[Console->BytesOfInput++] = (CHAR)(8 - (Console->CursorPosition.X & 0x07));

		IopWriteScreenBuffer(Console->ScreenBuffer, &Console->CursorPosition, '\t', Console->TextAttributes);
		IopSetScreenCursor(Console->ScreenBuffer, Console->CursorPosition);

		return;
	}
	
	if (Console->BytesOfInput <= INPUT_BUFFER_SIZE - 3) {

		//
		// ���������ٻ���3�ֽڲ��ܽ���һ����ͨ�ɼ��ַ�����Ϊ��Ҫ����2�ֽڸ��н�������
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

����������
	����̨��ǲ�̺߳�����
	�߳�ѭ����ȡ���������¼�������ϵͳ�����¼������û������¼���ǲ����ǰ��Ŀ�
	��̨���ڣ�����������д�����̨�����뻺��������
	Ŀǰϵͳ�����¼��������£�
		Pause���ж��ں˵����У����ڵ����ں�ʱ��Ч����
		Ctrl + Shift + Del���ݲ������κδ���
		Ctrl + F1~F4��������Ӧ�Ŀ���̨Ϊ��ǰ�����̨��F1~F4�ֱ��Ӧ����̨1~4��

������
	Parameter - ���á�

����ֵ��
	�ޡ�

--*/
{
	STATUS Status;
	HANDLE KeyboardHandle;
	ULONG NumberOfBytesRead;
	KEY_EVENT_RECORD KeyEventRecord;

	//
	// ��ռ�򿪼����豸��
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
		// ��ȡ�����¼���
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
			// �����ں�ʱ�����Pause�������򴥷�һ���ϵ㡣
			//
#ifdef _DEBUG
			if (VK_PAUSE == KeyEventRecord.VirtualKeyValue) {
				DbgBreakPoint();
				continue;
			}
#endif

			//
			// �����ϼ�Ctrl + Shift + Del����������ϵͳ����ĿǰʲôҲ������
			// ע�⣺С�����ϵ� . ��û��NumLockʱ��Ҳ����ΪDel��ʹ�õġ�
			//
			if ((KeyEventRecord.ControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0 &&
				(KeyEventRecord.ControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0 &&
				(VK_DELETE == KeyEventRecord.VirtualKeyValue ||
				VK_DECIMAL == KeyEventRecord.VirtualKeyValue && 
				(KeyEventRecord.ControlKeyState & NUMLOCK_ON) == 0)) {

				continue;
			}

			//
			// ���Ctrl + F1~F4�����£��򽫰�����Ӧ�Ŀ���̨��Ϊ�������̨��
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
		// ���û������¼����͸���ǰ��Ŀ���̨��
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

����������
	�趨����̨���ڵĹ��λ�á�

������
	Handle - ����̨�����
	CursorPosition - ���λ�����ꡣ

����ֵ��
	����ɹ��򷵻�STATUS_SUCCESS��

--*/
{
	STATUS Status;
	PCONSOLE Console;
	BOOL IntState;

	if (CursorPosition.X < 0 || CursorPosition.X >= 80 ||
		CursorPosition.Y < 0 || CursorPosition.Y >= 24) {
		return STATUS_INVALID_PARAMETER;
	}

	// ����������
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
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(i = INPUT_BUFFER_SIZE - 1; i > 0; i--)
	{
		if((*IopConsoleArray[0]).InputBuffer[i] != 0)
		{
			break;
		}
	}
	InputBufCount = i - 1;
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

int OutputBufCount;

void GetOutputBufferCount()
{
	int i = 0;
	BOOL IntState;
	
	StopKeyboard = 1;
	IntState = KeEnableInterrupts(FALSE);	// ���ж�
	
	for(i = 4095; i > 0; i = i - 2)
	{
		if((*IopConsoleArray[0]).ScreenBuffer[i] != 7 && (*IopConsoleArray[0]).ScreenBuffer[i-1] != 32)
		{
			break;
		}
	}
	OutputBufCount = i ;
	
	KeEnableInterrupts(IntState);	// ���ж�
	StopKeyboard = 0;
}

#endif
