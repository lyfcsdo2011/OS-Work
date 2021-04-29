/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: io.h

描述: IO 模块公共头文件。



*******************************************************************************/


#ifndef _IO_
#define _IO_

#include "eosdef.h"

//
// 初始化IO管理模块第一步。
//
VOID
IoInitializeSystem1(
	VOID
	);

//
// 初始化IO管理模块第二步。
//
VOID
IoInitializeSystem2(
	VOID
	);

//
// 创建或打开I/O对象。
//
STATUS 
IoCreateFile(
	IN PSTR FileName, 
	IN ULONG DesiredAccess, 
	IN ULONG ShareMode, 
	IN ULONG CreationDisposition, 
	IN ULONG FlagsAndAttributes, 
	OUT PHANDLE Handle
	);

//
// 删除文件。
//
STATUS
IoDeleteFile(
	IN PSTR FileName
	);

//
// 得到文件的时间戳。
//
STATUS
IoGetFileTime(
	IN HANDLE FileHandle,
	OUT PFILETIME CreationTime,
	OUT PFILETIME LastAccessTime,
	OUT PFILETIME LastWriteTime
	);

//
// 得到文件的长度。
//
STATUS 
IoGetFileSize(
	IN HANDLE FileHandle,
	OUT PULONG FileSize
	);

//
// 设置文件读写位置。
//
STATUS
IoSetFilePointer(
	IN HANDLE FileHandle,
	IN LONG DistanceToMove,
	IN ULONG MoveMethod,
	OUT PULONG NewFilePointer
	);

//
// 得到文件的属性值。
//
STATUS
IoGetFileAttributes(
	IN PSTR FileName,
	OUT PULONG FileAttributes
	);

//
// 修改文件的属性值。
//
STATUS
IoSetFileAttributes(
	IN PSTR FileName,
	IN ULONG FileAttributes
	);

//
// 创建一个目录。
//
STATUS
IoCreateDirectory(
	IN PSTR PathName
	);

//
// 删除一个目录。
//
STATUS
IoRemoveDirectory(
	IN PSTR PathName
	);

//
// 打开指定序号的控制台。
//
STATUS
IoOpenConsole(
	IN ULONG ConsoleIndex,
	OUT PHANDLE Handle
	);

//
// 设定控制台窗口的光标位置。
//
STATUS
IoSetConsoleCursorPosition(
	IN HANDLE Handle,
	IN COORD CursorPosition
	);

#endif // _IO_
