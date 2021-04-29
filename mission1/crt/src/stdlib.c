/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: stdlib.c

描述: C 运行时库的 stdlib 模块。



*******************************************************************************/

#include "stdlib.h"

char* itoa(int value, char *str, int radix)
{
	register char *p1;
	register char *p2;
	char c = 0;

	//
	// 如果基数超出范围 [2，36]，则设置字符串为空串并返回。
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
		// 如果以10为基负数，则暂存其符号并取其绝对值。
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
	*p1 = '\0';		// 结束字符串


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
