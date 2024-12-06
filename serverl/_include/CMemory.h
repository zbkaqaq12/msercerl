#pragma once
#include <stddef.h>  //NULL
//�ڴ������
class CMemory
{
private:
	CMemory() {}  //���캯������ΪҪ���ɵ����࣬������˽�еĹ��캯��

public:
	~CMemory() {};

private:
	static CMemory* m_instance;

public:
	static CMemory* GetInstance() //����
	{
		if (m_instance == NULL)
		{
			//��
			if (m_instance == NULL)
			{
				m_instance = new CMemory(); //��һ�ε��ò�Ӧ�÷����߳��У�Ӧ�÷����������У�����������̵߳��ó�ͻ�Ӷ�����ͬʱִ������new CMemory()
				static CGarhuishou cl;
			}
			//����
		}
		return m_instance;
	}
	class CGarhuishou
	{
	public:
		~CGarhuishou()
		{
			if (CMemory::m_instance)
			{
				delete CMemory::m_instance; //����ͷ�������ϵͳ�˳���ʱ��ϵͳ�������ͷ��ڴ��Ŷ
				CMemory::m_instance = NULL;
			}
		}
	};
	//-------

public:
	void* AllocMemory(int memCount, bool ifmemset);
	void FreeMemory(void* point);

};

