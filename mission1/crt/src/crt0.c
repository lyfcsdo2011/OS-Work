/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: crt0.c

����: CRT ���п�� CRT0 ģ�飬����Ӧ�ó����������ʼ����



*******************************************************************************/

#include <eos.h>

typedef void (*func_ptr) (void);

extern func_ptr __DTOR_LIST__[];

static void __do_global_dtors (void)
{
	func_ptr *p;

	//
	// ���ε���ȫ�ֶ��������������
	//
	for (p = __DTOR_LIST__ + 1; *p; p++) {
		(*(p)) ();
	}
}

extern int main(int argc, char* argv[]);

void _start (void)
{
	int retv;
	int argc;
	char *ptr;

	static char ImageName[MAX_PATH];
	static char CmdLine[1024];
	static char* argv[512];

	//
	// �õ������в������ָ�Ϊ����ַ��������ݸ�main������
	//
	GetImageNameAndCmdLine(ImageName, CmdLine);

	argc = 1;
	argv[0] = ImageName;

	for (ptr = CmdLine; *ptr != 0; ) {

		//
		// ���˿ո�
		//
		while (*ptr != 0 && *ptr == ' ') {
			ptr++;
		}

		if (*ptr == 0) {
			break;
		}

		argv[argc++] = ptr;

		while (*ptr != 0 && *ptr != ' '){
			ptr++;
		}

		//
		// �����ַ�����
		//
		if (' ' == *ptr) {
			*ptr++ = 0;
		}
	}

	//
	// ����main������
	//
	retv = main(argc, argv);

	//
	// ����ȫ�ֶ�������������
	//
	__do_global_dtors();

	//
	// �˳����̡�
	//
	ExitProcess(retv);
}
