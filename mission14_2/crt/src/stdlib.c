/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: stdlib.c

����: C ����ʱ��� stdlib ģ�顣



*******************************************************************************/

#include "stdlib.h"

char* itoa(int value, char *str, int radix)
{
	register char *p1;
	register char *p2;
	char c = 0;

	//
	// �������������Χ [2��36]���������ַ���Ϊ�մ������ء�
	//
	if(radix < 2 || 36 < radix ) {
		*str = 0;
		return str;
	}

	p1 = str;
	if(0 == value) {

		*p1++ = '0';

	} else {
		//
		// �����10Ϊ�����������ݴ�����Ų�ȡ�����ֵ��
		//
		if(10 == radix && value < 0) {
			c = '-';
			value = -value;
		}

		while(value) {

			if((*p1 = '0' + (unsigned int)value % radix) > '9')
				*p1 += 'A' - '9' - 1;

			value = (unsigned int)value / radix;
			p1++;
		}

		if('-' == c) {
			*p1++ = c;
		}
	}
	*p1 = '\0';		// �����ַ���


	//
	// reverse
	//
	p2 = str;
	while(--p1 > p2) {

		c = *p1;
		*p1 = *p2;
		*p2 = c;

		p2++;
	}

	return str;
}
