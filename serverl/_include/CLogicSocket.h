#pragma once
#include"CSocket.h"
#include"logiccomm.h"

class CLogicSocket :public CSocket
{
public:
	CLogicSocket();                                                         //���캯��
	virtual ~CLogicSocket();                                                //�ͷź���
	virtual bool Initialize();                                              //��ʼ������

public:

	//ͨ���շ�������غ���
	void  SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, unsigned short iMsgCode);

	//����ҵ���߼���غ�������֮��
	bool _HandleRegister(lpconnection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength);
	bool _HandleLogIn(lpconnection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength);
	bool _HandlePing(lpconnection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength);

	virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time);      //���������ʱ�䵽����ȥ����������Ƿ�ʱ�����ˣ�������ֻ�ǰ��ڴ��ͷţ�����Ӧ���������ȸú�����ʵ�־�����ж϶���

public:
	virtual void threadRecvProcFunc(char* pMsgBuf);
};

