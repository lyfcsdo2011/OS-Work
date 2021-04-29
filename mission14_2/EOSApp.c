/*
�ṩ��ʾ��������Ϊ�˲���һ��������߽���һ�����ԣ�����������
�ȫ�ı���ʵ������˲�Ӧ��Ӧ�ó������վ��ʹ�ø�ʾ�����롣��
�ڳ�����ʾ�������Ԥ����;�����ʹ������ɵ�żȻ��̷�����ʧ��
����Ӣ��ʱ���Ƽ����޹�˾���е��κ����Ρ�
*/

#include "EOSApp.h"


#define BUFFER_SIZE 1024
BYTE Buffer[BUFFER_SIZE];

//
// main �������������壺
// argc - argv ����ĳ��ȣ���С����Ϊ 1��argc - 1 Ϊ�����в�����������
// argv - �ַ���ָ�����飬���鳤��Ϊ�����в������� + 1������ argv[0] �̶�ָ��ǰ
//        ������ִ�еĿ�ִ���ļ���·���ַ�����argv[1] ��������ָ��ָ�����������
//        ������
//        ����ͨ������������ "a:\hello.exe -a -b" �������̺�hello.exe �� main ��
//        ���Ĳ��� argc ��ֵΪ 3��argv[0] ָ���ַ��� "a:\hello.exe"��argv[1] ָ��
//        �����ַ��� "-a"��argv[2] ָ������ַ��� "-b"��
//
int main(int argc, char* argv[])
{
	//
	// �������� EOS Ӧ�ó���ǰҪ�ر�ע����������⣺
	//
	// 1�����Ҫ�ڵ���Ӧ�ó���ʱ�ܹ����Խ����ں˲���ʾ��Ӧ��Դ�룬
	//    ����ʹ�� EOS ������Ŀ����������ȫ�汾�� SDK �ļ��У�Ȼ
	//    ��ʹ�ô��ļ��и���Ӧ�ó�����Ŀ�е� SDK �ļ��У����� EOS
	//    ������Ŀ�ڴ����ϵ�λ�ò��ܸı䡣
	//

	HANDLE hFileRead = INVALID_HANDLE_VALUE;
	HANDLE hFileWrite = INVALID_HANDLE_VALUE;
	HANDLE hOutput;
	ULONG m, n;
	int Result = 1;	// ����ֵ 1����ʾִ��ʧ��

	//
	// �������﷨��A:\EOSApp.exe read_file_name [write_file_name] [-a]
	// ������A:\EOSApp.exe A:\a.txt
	//       ��ʾ�� a.txt �ļ��е������������Ļ��
	//       A:\EOSApp.exe A:\a.txt A:\b.txt
	//       ��ʾ�� a.txt �ļ��е�����д�� b.txt �ļ��С�b.txt �ļ�ԭ�е����ݻᱻ���ǡ�
	//       A:\EOSApp.exe A:\a.txt A:\b.txt -a
	//       ��ʾ�� a.txt �ļ��е����ݸ��ӵ� b.txt �ļ���ĩβ��
	//
	if (argc < 2 || argc > 4)
	{
		printf("Error: Invalid argument count!\n"
		       "Valid command line: EOSApp.exe read_file_name [write_file_name] [-a]\n");
		goto RETURN;
	}
	
	//
	// ��ֻ���ķ�ʽ����Ҫ��ȡ���ļ�
	//
	hFileRead = CreateFile(argv[1], GENERIC_READ, 0, OPEN_EXISTING, 0);
	if (INVALID_HANDLE_VALUE == hFileRead)
	{
		printf("Open file \"%s\" error: %d\n", argv[1], GetLastError());
		goto RETURN;
	}
	
	//
	// ��������в�������Ҫд����ļ�����ʹ��д���ļ������Ϊ��������
	// ����ʹ�ñ�׼�������Ļ�������Ϊ��������
	//
	if (argc > 2)
	{
		//
		// ��ֻд�ķ�ʽ����Ҫд����ļ�
		//
		hFileWrite = CreateFile(argv[2], GENERIC_WRITE, 0, OPEN_EXISTING, 0);
		if (INVALID_HANDLE_VALUE == hFileWrite)
		{
			printf("Open file \"%s\" error: %d\n", argv[2], GetLastError());
			goto RETURN;
		}
		
		//
		// ���������д�����ļ�ָ���ƶ����ļ���ĩβ��
		//
		if (4 == argc && 0 == stricmp(argv[3], "-a"))
		{
			SetFilePointer(hFileWrite, GetFileSize(hFileWrite), FILE_BEGIN);
		}
		
		hOutput = hFileWrite;
	}
	else
	{
		hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	while (TRUE)
	{
		//
		// ���Զ�ȡ BUFFER_SIZE ���ֽڣ�ʵ�ʶ�ȡ�����ֽ����� m ���ء�
		//
		ReadFile(hFileRead, Buffer, BUFFER_SIZE, &m);
		
		//
		// ��ʵ�ʶ�ȡ���� m ���ֽ�д����������
		//
		if (!WriteFile(hOutput, Buffer, m, &n))
		{
			printf("Write file error: %d\n", GetLastError());
			goto RETURN;
		}
		
		//
		// ���ʵ�ʶ�ȡ���ֽ�������Ԥ�ڣ�˵���ļ���ȡ��ϡ�
		//
		if (m < BUFFER_SIZE)
			break;
	}
	
	//
	// ����ֵ 0����ʾִ�гɹ�
	//
	Result = 0;
	
RETURN:
	//
	// �ر��ļ�
	//
	if (hFileRead != INVALID_HANDLE_VALUE)
		CloseHandle(hFileRead);
	if (hFileWrite != INVALID_HANDLE_VALUE)
		CloseHandle(hFileWrite);
	
	return Result;
}
