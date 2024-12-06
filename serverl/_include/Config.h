#pragma once
#include<iostream>
#include<vector>
#include"tinyxml2.h"


/**
 * @sa _CConfItem
 * @brief ������Ϣ��ṹ�壬�洢����������ƺ����ݡ�
 */
typedef struct _CConfItem
{
	std::string ItemName;
	std::string ItemContent;
}CConfItem, * LPCConfItem;

/**
 * @sa Config
 * @brief ���ù����࣬�ṩ���������ļ��ͻ�ȡ������Ĺ��ܡ�
 */
class Config
{
private:
	Config();
public:
	~Config();

private:
	static Config* m_instance;

public:
	static Config* GetInstance()
	{
		if (m_instance == nullptr)
		{
			if (m_instance == nullptr)
			{
				m_instance = new Config();
				static Garhuishou c1;
			}
		}
		return m_instance;
	}

	class Garhuishou  //�������࣬�����ͷŶ���
	{
	public:
		~Garhuishou()
		{
			if (Config::m_instance)
			{
				delete Config::m_instance;
				Config::m_instance = NULL;
			}
		}
	};

public:
	bool Load(const char* pconfName); //װ�������ļ�
	const char* GetString(const char* p_itemname);
	int  GetIntDefault(const char* p_itemname, const int def);

public:
	std::vector<LPCConfItem> m_ConfigItemList; //�洢������Ϣ���б�

};