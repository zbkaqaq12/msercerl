#include "Config.h"

// ��̬��Ա��ֵ
Config* Config::m_instance = NULL;

/**
 * @brief ���캯������ʼ��������ʵ����
 */
Config::Config()
{
}

/**
 * @brief �����������ͷ�������������ڴ沢����������б�
 */
Config::~Config()
{
    std::vector<LPCConfItem>::iterator pos;
    for (pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
    {
        delete (*pos); // ɾ�����������
    }
    m_ConfigItemList.clear(); // ����������б�
}

/**
 * @brief ���������ļ����������е������
 *
 * �ú���������ָ���� XML �����ļ����������ļ��еĸ����������������洢�� `m_ConfigItemList` �С�
 *
 * @param pconfName �����ļ���·����
 * @return bool ������سɹ����� true�����򷵻� false��
 */
bool Config::Load(const char* pconfName)
{
    tinyxml2::XMLDocument xmlDoc;

    // ���� XML �����ļ�
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(pconfName);
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load config file: " << pconfName << std::endl;
        return false;
    }

    // ��ȡ XML ���ڵ�
    tinyxml2::XMLElement* rootElement = xmlDoc.RootElement();
    if (rootElement == nullptr) {
        std::cerr << "no root element found in the xml file: " << pconfName << std::endl;
        return false;
    }

    m_ConfigItemList.clear(); // ���֮ǰ��������

    // ����һ����Ԫ�أ��� Log, Proc, Net �ȣ�
    for (tinyxml2::XMLElement* section = rootElement->FirstChildElement(); section; section = section->NextSiblingElement()) {
        // ��ȡһ����Ԫ�ص����ƣ����ࣩ
        const char* sectionName = section->Value();
        if (!sectionName) continue;

        // �����÷����µĶ�����Ԫ�أ�ʵ�ʵ������
        for (tinyxml2::XMLElement* item = section->FirstChildElement(); item; item = item->NextSiblingElement()) {
            // ��ȡÿ������������ƺ�ֵ
            const char* itemName = item->Value();
            const char* itemValue = item->GetText();
            if (itemName != nullptr && itemValue != nullptr) {
                LPCConfItem confItem = new CConfItem;
                confItem->ItemName = std::string(itemName);
                confItem->ItemContent = itemValue;
                m_ConfigItemList.push_back(confItem); // ����������ӵ��б���
            }
        }
    }
    return true; // �����ļ����ز������ɹ�
}

/**
 * @brief ��ȡָ����������ַ������ݡ�
 *
 * ���ݴ�������������ƣ��������Ӧ���������ݣ��ַ�����ʽ�������δ�ҵ���Ӧ����������� nullptr��
 *
 * @param p_itemname ����������ơ�
 * @return const char* ����ҵ���Ӧ���������������������ݣ����򷵻� nullptr��
 */
const char* Config::GetString(const char* p_itemname)
{
    for (const auto& item : m_ConfigItemList) {
        if (item->ItemName == p_itemname) {
            return item->ItemContent.c_str(); // ��������������
        }
    }
    return nullptr; // ���û���ҵ���Ӧ����������ؿ�ָ��
}

/**
 * @brief ��ȡָ�������������ֵ��
 *
 * ���ݴ�������������ƣ���ȡ���Ӧ������ֵ�������������ڣ��򷵻��ṩ��Ĭ��ֵ��
 *
 * @param p_itemname ����������ơ�
 * @param def Ĭ��ֵ�������������ʱ���ظ�ֵ��
 * @return int ���������������ֵ��Ĭ��ֵ��
 */
int Config::GetIntDefault(const char* p_itemname, const int def)
{
    const char* itemValue = GetString(p_itemname); // ��ȡ��������ַ���ֵ
    if (itemValue != nullptr) {
        return std::stoi(itemValue); // ���ַ���ת��Ϊ����������
    }
    return def; // ���û���ҵ����������Ĭ��ֵ
}
