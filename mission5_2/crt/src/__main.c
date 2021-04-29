/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: __main.c

描述: 全局变量构造函数的调用者。



*******************************************************************************/

typedef void (*func_ptr) (void);

extern func_ptr __CTOR_LIST__[];

void __main(void)
{
	unsigned long i;

	i = (unsigned long) __CTOR_LIST__[0];

	//
	// 如果第0个元素没有指出数组的长度则统计数组的长度。
	//
	if (i == -1) {
		for (i = 0; __CTOR_LIST__[i + 1]; i++);
	}

	//
	// 逆序调用全局对象的构造函数。
	//
	while (i > 0) {
		__CTOR_LIST__[i--] ();
	}
}
