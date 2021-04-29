/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: error.h

描述: EOS 错误码常量的定义。



*******************************************************************************/

#ifndef _ERROR_
#define _ERROR_

#define NO_ERROR						0L
#define ERROR_FILE_NOT_FOUND			2L
#define ERROR_PATH_NOT_FOUND			3L
#define ERROR_ACCESS_DENIED				5L
#define ERROR_INVALID_HANDLE			6L
#define ERROR_NOT_ENOUGH_MEMORY			8L
#define ERROR_SHARING_VIOLATION			32L
#define ERROR_WRONG_DISK				34L
#define ERROR_NOT_SUPPORTED				50L
#define ERROR_INVALID_PARAMETER			87L
#define ERROR_PROC_NOT_FOUND			127L
#define ERROR_SIGNAL_REFUSED			156L
#define ERROR_BAD_PATHNAME				161L
#define ERROR_ALREADY_EXISTS			183L
#define ERROR_BAD_EXE_FORMAT			193L
#define ERROR_FILENAME_EXCED_RANGE		206L
#define WAIT_TIMEOUT					258L
#define ERROR_NOT_OWNER					288L
#define ERROR_TOO_MANY_POSTS			298L
#define ERROR_INVALID_ADDRESS			487L
#define ERROR_IO_PENDING				997L 
#define ERROR_NOACCESS					998L
#define ERROR_FLOPPY_UNKNOWN_ERROR		1124L
#define ERROR_NOT_FOUND					1168L
#define ERROR_FILE_CORRUPT				1392L
#define ERROR_TIMEOUT					1460L
#define ERROR_INVALID_COMMAND_LINE		1639L


#define ERROR_UNKNOWN					7499L

#endif // _ERROR_
