/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: list.c

����: �������ݽṹģ�顣



*******************************************************************************/

#include "rtl.h"

PLIST_ENTRY
ListRemoveHead(
	PLIST_ENTRY ListHead
	)
/*++

����������
	�Ƴ�����ĵ�һ�

������
	ListHead -- ����ͷָ�롣

����ֵ��
	���ر��Ƴ����������ָ�룬�������Ϊ�գ����� NULL��

--*/
{
	if (ListIsEmpty(ListHead)) {
		return NULL;
	}
	
	//
	// ����ĵ�һ��������ͷָ�����һ�����������ͷ��
	//
	return ListRemoveEntry(ListHead->Next);
}

PLIST_ENTRY
ListRemoveTail(
	PLIST_ENTRY ListHead
	)
/*++

����������
	�Ƴ���������һ�

������
	ListHead -- ����ͷָ�롣

����ֵ��
	���ر��Ƴ����������ָ�룬�������Ϊ�գ����� NULL��

--*/
{
	if(ListIsEmpty(ListHead)) {
		return NULL;
	}

	//
	// ��������һ���������ͷָ���ǰһ�
	//
	return ListRemoveEntry(ListHead->Prev);
}

ULONG
ListGetCount(
	PLIST_ENTRY ListHead
	)
/*++

����������
	�õ����������������Ŀ��ע�⣬����ͷ���������ڡ�

������
	ListHead -- ����ͷָ�롣

����ֵ��
	�������������Ŀ��

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

����������
	��ʼ������ͷ��Ҳ���ǽ������ʼ��Ϊ�ա�

������
	ListHead -- ����ͷָ�롣

����ֵ��
	��ʼ����ϵ�����ͷָ�롣

--*/
{
	//
	// һ���յ�˫��ѭ������������ͷ��ǰһ��ָ��
	// �ͺ�һ��ָ�붼ָ������ͷ�Լ���
	//
	ListHead->Next = ListHead->Prev = ListHead;

	return ListHead;
}

BOOL
ListIsEmpty(
	PLIST_ENTRY ListHead
	)
/*++

����������
	�ж�˫��ѭ�������Ƿ�Ϊ�ա�

������
	ListHead -- ����ͷָ�롣

����ֵ��
	˫��ѭ������Ϊ�գ����� TRUE�����򷵻� FALSE��

--*/
{
	//
	// ���˫��ѭ������ͷ�ĺ�һ��ָ��ָ������ͷ�Լ�����˵������Ϊ�ա�
	// �����жϵ�ԭ�򣬿��Բο���ʼ������ͷ���� ListInitializeHead��
	//
	return ListHead->Next == ListHead;
}

PLIST_ENTRY
ListRemoveEntry(
	PLIST_ENTRY ListEntry
	)
/*++

����������
	��˫��ѭ���������Ƴ�ָ���������

������
	ListEntry -- ������ָ�롣

����ֵ��
	�Ƴ����������ָ�롣

��ʾ��
	˫��ѭ����������ƾ����ڣ�����Ҫ���������Ƴ�һ��������ʱ��
	ֻ��Ҫ������������ָ�뼴����ɲ�����

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

����������
	��˫��ѭ��������ָ���������ǰ�����һ�������

������
	Pos -- �ڴ�������ǰ�����һ�������
	ListEntry -- �����������ָ�롣

����ֵ��
	�����������ָ�롣

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

����������
	��˫��ѭ��������ָ��������ĺ������һ�������

������
	Pos -- �ڴ�������������һ�������
	ListEntry -- �����������ָ�롣

����ֵ��
	�����������ָ�롣

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

����������
	��˫��ѭ������Ŀ�ʼ����һ�������Ҳ����������ͷ�ĺ������һ�������

������
	ListHead -- ����ͷָ�롣
	ListEntry -- �����������ָ�롣

����ֵ��
	�����������ָ�롣

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

����������
	��˫��ѭ�������ĩβ����һ�������Ҳ����������ͷ��ǰ�����һ�������

������
	ListHead -- ����ͷָ�롣
	ListEntry -- �����������ָ�롣

����ֵ��
	�����������ָ�롣

--*/
{
	return ListInsertBefore(ListHead, ListEntry);
}

//////////////////////////////////////////////////////////////////////////
//
// �����������������
//

PSINGLE_LIST_ENTRY
SListInitializeHead(
	PSINGLE_LIST_ENTRY ListHead
	)
/*++

����������
	��ʼ����������ı�ͷ��Ҳ���ǽ������ʼ��Ϊ�ա�

������
	ListHead -- ����ͷָ�롣

����ֵ��
	��ʼ����ϵ�����ͷָ�롣

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

����������
	�Ƴ���������ĵ�һ�������

������
	ListHead -- ����ͷָ�롣

����ֵ��
	�Ƴ����������ָ�롣

--*/
{
	//
	// ��������ĵ�һ��������������ͷָ�����һ�����������������ͷ��
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

����������
	�ڵ�������Ŀ�ʼ����һ�������Ҳ����������ͷ�ĺ������һ�������

������
	ListHead -- ����ͷָ�롣
	ListEntry -- �����������ָ�롣

����ֵ��
	������������ָ�롣

--*/
{
	ListEntry->Next = ListHead->Next;
	ListHead->Next = ListEntry;

	return ListEntry;
}
