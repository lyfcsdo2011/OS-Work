/***

Copyright (c) 2008 北京英真时代科技有限公司。保留所有权利。

只有您接受 EOS 核心源代码协议（参见 License.txt）中的条款才能使用这些代码。
如果您不接受，不能使用这些代码。

文件名: list.c

描述: 链表数据结构模块。



*******************************************************************************/

#include "rtl.h"

PLIST_ENTRY
ListRemoveHead(
	PLIST_ENTRY ListHead
	)
/*++

功能描述：
	移除链表的第一项。

参数：
	ListHead -- 链表头指针。

返回值：
	返回被移除的链表项的指针，如果链表为空，返回 NULL。

--*/
{
	if (ListIsEmpty(ListHead)) {
		return NULL;
	}
	
	//
	// 链表的第一项是链表头指向的下一项，而不是链表头。
	//
	return ListRemoveEntry(ListHead->Next);
}

PLIST_ENTRY
ListRemoveTail(
	PLIST_ENTRY ListHead
	)
/*++

功能描述：
	移除链表的最后一项。

参数：
	ListHead -- 链表头指针。

返回值：
	返回被移除的链表项的指针，如果链表为空，返回 NULL。

--*/
{
	if(ListIsEmpty(ListHead)) {
		return NULL;
	}

	//
	// 链表的最后一项就是链表头指向的前一项。
	//
	return ListRemoveEntry(ListHead->Prev);
}

ULONG
ListGetCount(
	PLIST_ENTRY ListHead
	)
/*++

功能描述：
	得到链表中链表项的数目。注意，链表头不计算在内。

参数：
	ListHead -- 链表头指针。

返回值：
	返回链表项的数目。

--*/
{
	PLIST_ENTRY ListEntry;
	ULONG Count = 0;
	
	for (ListEntry = ListHead->Next;
		ListEntry != ListHead;
		ListEntry = ListEntry->Next) {
		
		Count++;
	}
	
	return Count;
}

PLIST_ENTRY
ListInitializeHead(
	PLIST_ENTRY ListHead
	)
/*++

功能描述：
	初始化链表头，也就是将链表初始化为空。

参数：
	ListHead -- 链表头指针。

返回值：
	初始化完毕的链表头指针。

--*/
{
	//
	// 一个空的双向循环链表，其链表头的前一项指针
	// 和后一项指针都指向链表头自己。
	//
	ListHead->Next = ListHead->Prev = ListHead;

	return ListHead;
}

BOOL
ListIsEmpty(
	PLIST_ENTRY ListHead
	)
/*++

功能描述：
	判断双向循环链表是否为空。

参数：
	ListHead -- 链表头指针。

返回值：
	双向循环链表为空，返回 TRUE，否则返回 FALSE。

--*/
{
	//
	// 如果双向循环链表头的后一项指针指向链表头自己，就说明链表为空。
	// 这样判断的原因，可以参考初始化链表头函数 ListInitializeHead。
	//
	return ListHead->Next == ListHead;
}

PLIST_ENTRY
ListRemoveEntry(
	PLIST_ENTRY ListEntry
	)
/*++

功能描述：
	从双向循环链表中移除指定的链表项。

参数：
	ListEntry -- 链表项指针。

返回值：
	移除的链表项的指针。

提示：
	双向循环链表的优势就在于，当需要从链表中移除一个链表项时，
	只需要有这个链表项的指针即可完成操作。

--*/
{
	ListEntry->Next->Prev = ListEntry->Prev;
	ListEntry->Prev->Next = ListEntry->Next;
	
	ListEntry->Next = NULL;
	ListEntry->Prev = NULL;

	return ListEntry;
}

PLIST_ENTRY
ListInsertBefore(
	PLIST_ENTRY Pos,
	PLIST_ENTRY ListEntry
	)
/*++

功能描述：
	在双向循环链表中指定链表项的前面插入一个链表项。

参数：
	Pos -- 在此链表项前面插入一个链表项。
	ListEntry -- 插入的链表项指针。

返回值：
	插入的链表项指针。

--*/
{
	ListEntry->Next = Pos;
	ListEntry->Prev = Pos->Prev;
	Pos->Prev->Next = ListEntry;
	Pos->Prev = ListEntry;

	return ListEntry;
}

PLIST_ENTRY
ListInsertAfter(
	PLIST_ENTRY Pos,
	PLIST_ENTRY ListEntry
	)
/*++

功能描述：
	在双向循环链表中指定链表项的后面插入一个链表项。

参数：
	Pos -- 在此链表项后面插入一个链表项。
	ListEntry -- 插入的链表项指针。

返回值：
	插入的链表项指针。

--*/
{
	return ListInsertBefore(Pos->Next, ListEntry);
}

PLIST_ENTRY
ListInsertHead(
	PLIST_ENTRY ListHead,
	PLIST_ENTRY ListEntry
	)
/*++

功能描述：
	在双向循环链表的开始插入一个链表项。也就是在链表头的后面插入一个链表项。

参数：
	ListHead -- 链表头指针。
	ListEntry -- 插入的链表项指针。

返回值：
	插入的链表项指针。

--*/
{
	return ListInsertAfter(ListHead, ListEntry);
}

PLIST_ENTRY
ListInsertTail(
	PLIST_ENTRY ListHead,
	PLIST_ENTRY ListEntry
	)
/*++

功能描述：
	在双向循环链表的末尾插入一个链表项。也就是在链表头的前面插入一个链表项。

参数：
	ListHead -- 链表头指针。
	ListEntry -- 插入的链表项指针。

返回值：
	插入的链表项指针。

--*/
{
	return ListInsertBefore(ListHead, ListEntry);
}

//////////////////////////////////////////////////////////////////////////
//
// 单向链表操作函数。
//

PSINGLE_LIST_ENTRY
SListInitializeHead(
	PSINGLE_LIST_ENTRY ListHead
	)
/*++

功能描述：
	初始化单向链表的表头，也就是将链表初始化为空。

参数：
	ListHead -- 链表头指针。

返回值：
	初始化完毕的链表头指针。

--*/
{
	ListHead->Next = NULL;

	return ListHead;
}

PSINGLE_LIST_ENTRY
SListPopEntry(
	PSINGLE_LIST_ENTRY ListHead
	)
/*++

功能描述：
	移除单向链表的第一个链表项。

参数：
	ListHead -- 链表头指针。

返回值：
	移除的链表项的指针。

--*/
{
	//
	// 单向链表的第一个链表项是链表头指向的下一个链表项，而不是链表头。
	//
    PSINGLE_LIST_ENTRY ListEntry = ListHead->Next;
    
	if (ListEntry != NULL) {
        ListHead->Next = ListEntry->Next;
		ListEntry->Next = NULL;
	}
	
	return ListEntry;
}

PSINGLE_LIST_ENTRY
SListPushEntry(
	PSINGLE_LIST_ENTRY ListHead,
	PSINGLE_LIST_ENTRY ListEntry
	)
/*++

功能描述：
	在单向链表的开始插入一个链表项。也就是在链表头的后面插入一个链表项。

参数：
	ListHead -- 链表头指针。
	ListEntry -- 插入的链表项指针。

返回值：
	插入的链表项的指针。

--*/
{
	ListEntry->Next = ListHead->Next;
	ListHead->Next = ListEntry;

	return ListEntry;
}
