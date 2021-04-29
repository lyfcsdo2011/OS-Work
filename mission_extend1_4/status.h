/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: status.h

描述: EOS 内部错误状态码常量的定义。



*******************************************************************************/


#ifndef _STATUS_
#define _STATUS_


//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+---------------------------+-------------------------------+
//  |Sev|        Reserved           |               Code            |
//  +---+---------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      Reserved - are reserved bits
//
//      Code - is the facility's status code
//

/////////////////////////////////////////////////////////////////////////
//
// Standard Success values
//
/////////////////////////////////////////////////////////////////////////

#define STATUS_SUCCESS							((STATUS)0x00000000L)
#define STATUS_TIMEOUT							((STATUS)0x00000001L)
#define STATUS_PENDING							((STATUS)0x00000002L)
#define STATUS_NOTHING_TO_TERMINATE				((STATUS)0x00000003L)

/////////////////////////////////////////////////////////////////////////
//
// Standard Information values
//
/////////////////////////////////////////////////////////////////////////

#define STATUS_OBJECT_NAME_EXISTS				((STATUS)0x40000000L)
#define STATUS_FILE_ALLREADY_EXISTS				((STATUS)0x40000001L)


/////////////////////////////////////////////////////////////////////////
//
// Standard Warning values
//
//
// 注意: 不要使用值 0x80000000L, 因为宏EOS_SUCCESS无法判断这个值属于警告
//		 值，警告值应该从1开始。
//
/////////////////////////////////////////////////////////////////////////

//
// 暂时还没有警告状态码。
//

/////////////////////////////////////////////////////////////////////////
//
// Standard Error values
//
/////////////////////////////////////////////////////////////////////////

#define STATUS_NOT_SUPPORTED					((STATUS)0xC0000001L)
#define STATUS_ACCESS_VIOLATION					((STATUS)0xC0000002L)
#define STATUS_ACCESS_DENIED					((STATUS)0xC0000003L)
#define STATUS_INVALID_PARAMETER				((STATUS)0xC0000004L)
#define STATUS_OBJECT_NAME_NOT_FOUND			((STATUS)0xC0000005L)
#define STATUS_OBJECT_NAME_COLLISION			((STATUS)0xC0000006L)
#define STATUS_OBJECT_ID_NOT_FOUND				((STATUS)0xC0000007L)
#define STATUS_OBJECT_TYPE_MISMATCH				((STATUS)0xC0000008L)
#define STATUS_INVALID_HANDLE					((STATUS)0xC000000AL)
#define STATUS_MAX_HANDLE_EXCEEDED				((STATUS)0xC000000BL)
#define STATUS_NO_MEMORY						((STATUS)0xC000000CL)
#define STATUS_FREE_VM_NOT_AT_BASE				((STATUS)0xC000000DL)
#define STATUS_MEMORY_NOT_ALLOCATED				((STATUS)0xC000000EL)
#define STATUS_INVALID_ADDRESS					((STATUS)0xC000000FL)
#define STATUS_INVALID_DESTINATION_ADDRESS		((STATUS)0xC0000010L)
#define STATUS_INVALID_SOURCE_ADDRESS			((STATUS)0xC0000011L)
#define STATUS_MUTEX_NOT_OWNED					((STATUS)0xC0000014L)
#define STATUS_SEMAPHORE_LIMIT_EXCEEDED			((STATUS)0xC0000015L)
#define STATUS_SUSPEND_COUNT_EXCEEDED			((STATUS)0xC0000016L)
#define STATUS_PATH_TOO_LONG					((STATUS)0xC0000017L)
#define STATUS_PATH_SYNTAX_BAD					((STATUS)0xC0000018L)
#define STATUS_PATH_NOT_FOUND					((STATUS)0xC0000019L)
#define STATUS_FILE_NOT_FOUND					((STATUS)0xC000001AL)
#define STATUS_FILE_NAME_COLLISION				((STATUS)0xC000001BL)
#define	STATUS_FILE_CORRUPT_ERROR				((STATUS)0xC000001CL)
#define STATUS_FLOPPY_UNKNOWN_ERROR				((STATUS)0xC000001DL)
#define STATUS_WRONG_VOLUME						((STATUS)0xC000001EL)
#define STATUS_PROCESS_IS_TERMINATING			((STATUS)0xC000001FL)
#define STATUS_SHARING_VIOLATION				((STATUS)0xC0000020L)
#define STATUS_INVALID_APP_IMAGE				((STATUS)0xC0000021L)
#define STATUS_SYMBOL_NOT_FOUND					((STATUS)0xC0000022L)
#define STATUS_INVALID_COMMAND_LINE				((STATUS)0xC0000023L)
#define STATUS_NO_SPACE							((STATUS)0xC0000024L)



#endif // _STATUS_
