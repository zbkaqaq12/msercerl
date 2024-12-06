#include "CMemory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//�ྲ̬��Ա��ֵ
CMemory* CMemory::m_instance = NULL;

//�����ڴ�
//memCount��������ֽڴ�С
//ifmemset���Ƿ�Ҫ�ѷ�����ڴ��ʼ��Ϊ0��
void* CMemory::AllocMemory(int memCount, bool ifmemset)
{
	void* tmpData = (void*)new char[memCount]; //�Ҳ������ж�new�Ƿ�ɹ������newʧ�ܣ����������Ӧ�ü������У������������Է��������Ŵ��
	if (ifmemset) //Ҫ���ڴ���0
	{
		memset(tmpData, 0, memCount);
	}
	return tmpData;
}

//�ڴ��ͷź���
void CMemory::FreeMemory(void* point)
{
	//delete [] point;  //��ôɾ���������־��棺warning: deleting ��void*�� is undefined [-Wdelete-incomplete]
	delete[]((char*)point); //new��ʱ����char *������Ū��char *�����������
}