/***

Copyright (c) 2008 ����Ӣ��ʱ���Ƽ����޹�˾����������Ȩ����

ֻ�������� EOS ����Դ����Э�飨�μ� License.txt���е��������ʹ����Щ���롣
����������ܣ�����ʹ����Щ���롣

�ļ���: obmethod.c

����: �����麯���ĵ��ã����� Wait��Read �� Write��



*******************************************************************************/

#include "obp.h"

STATUS
ObWaitForObject(
	IN HANDLE Handle,
	IN ULONG Milliseconds
	)
/*++

����������
	�ȴ�һ��֧��ͬ�����ܵ��ں˶������绥���ź�������

������
	Handle -- ֧��ͬ�����ܵĶ���ľ����
	Milliseconds -- �ȴ���ʱ���ޣ���λ���롣

����ֵ��
	����ֵ���������Ͷ�����

--*/
{
	STATUS Status;
	PVOID Object;
	POBJECT_TYPE Type;

	//
	// �ɶ������õ�����ָ��
	//
	Status = ObRefObjectByHandle(Handle, NULL, &Object);

	if (EOS_SUCCESS(Status)) {

		//
		// �ɶ���ָ��õ�ע��Ķ�������
		//
		Type = OBJECT_TO_OBJECT_TYPE(Object);

		if (NULL != Type->Wait) {
			//
			// ���ö���������ע��� Wait ����
			//
			Status = Type->Wait(Object, Milliseconds);
		} else {
			Status = STATUS_INVALID_HANDLE;
		}

		ObDerefObject(Object);
	}

	return Status;
}

STATUS
ObRead(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToRead,
	OUT PULONG NumberOfBytesRead
	)
/*++

����������
	��֧�ֶ��������ں˶��������ļ�������̨���ܵ��������豸�ȶ���

������
	Handle -- ֧�ֶ������Ķ���ľ����
	Buffer -- ָ�룬ָ�����ڱ����ȡ����Ļ�������
	NumberOfBytesToRead -- ������ȡ���ֽ�����
	NumberOfBytesRead -- ָ�룬ָ�����ڱ���ʵ�ʶ�ȡ�ֽ��������α�����

����ֵ��
	���������Ͷ�����

--*/
{
	STATUS Status;
	PVOID Object;
	POBJECT_TYPE Type;

	//
	// �ɶ������õ�����ָ��
	//
	Status = ObRefObjectByHandle(Handle, NULL, &Object);

	if (EOS_SUCCESS(Status)) {

		//
		// �ɶ���ָ��õ�ע��Ķ�������
		//
		Type = OBJECT_TO_OBJECT_TYPE(Object);

		if (NULL != Type->Read) {
			//
			// ���ö���������ע��� Read ����
			//
			Status = Type->Read(Object, Buffer, NumberOfBytesToRead, NumberOfBytesRead);
		} else {
			Status = STATUS_INVALID_HANDLE;
		}

		ObDerefObject(Object);
	}

	return Status;
}

STATUS
ObWrite(
	IN HANDLE Handle,
	IN PVOID Buffer,
	IN ULONG NumberOfBytesToWrite,
	OUT PULONG NumberOfBytesWritten
	)
/*++

����������
	д֧��д�������ں˶��������ļ�������̨���ܵ��������豸�ȶ���

������
	Handle -- ֧��д�����Ķ���ľ����
	Buffer -- ָ�룬ָ�����ڱ����д���ݵĻ�������
	NumberOfBytesToWrite -- ����д���ֽ�����
	NumberOfBytesWritten -- ָ�룬ָ�����ڱ���ʵ��д���ֽ��������α�����

����ֵ��
	���������Ͷ�����

--*/
{
	STATUS Status;
	PVOID Object;
	POBJECT_TYPE Type;

	//
	// �ɶ������õ�����ָ��
	//
	Status = ObRefObjectByHandle(Handle, NULL, &Object);

	if (EOS_SUCCESS(Status)) {

		//
		// �ɶ���ָ��õ�ע��Ķ�������
		//
		Type = OBJECT_TO_OBJECT_TYPE(Object);

		if (NULL != Type->Write) {
			//
			// ���ö���������ע��� Write ����
			//
			Status = Type->Write(Object, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
		} else {
			Status = STATUS_INVALID_HANDLE;
		}

		ObDerefObject(Object);
	}

	return Status;
}
