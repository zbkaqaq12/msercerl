#pragma once
//�շ�����궨��

#define _CMD_START	                    0  
#define _CMD_PING				   	    _CMD_START + 0   //ping�����������
#define _CMD_REGISTER 		            _CMD_START + 5   //ע��
#define _CMD_LOGIN 		                _CMD_START + 6   //��¼



//�ṹ����------------------------------------
#pragma pack (1) //���뷽ʽ,1�ֽڶ��롾�ṹ֮���Ա�����κ��ֽڶ��룺���ܵ�������һ��

typedef struct _STRUCT_REGISTER
{
	int           iType;          //����
	char          username[56];   //�û��� 
	char          password[40];   //����

}STRUCT_REGISTER, * LPSTRUCT_REGISTER;

typedef struct _STRUCT_LOGIN
{
	char          username[56];   //�û��� 
	char          password[40];   //����

}STRUCT_LOGIN, * LPSTRUCT_LOGIN;


#pragma pack() //ȡ��ָ�����룬�ָ�ȱʡ����