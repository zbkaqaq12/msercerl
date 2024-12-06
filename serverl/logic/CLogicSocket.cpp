#include "CLogicSocket.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <mutex>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <functional>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>


#include "macro.h"
#include "global.h"
#include "CMemory.h"
#include "CCRC32.h"

//�����Աָ�뺯��
using handler = bool (CLogicSocket::*)(lpconnection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength);


//�������� ��Ա����ָ�� ����ô������
static const handler statusHandler[] =
{
    //����ǰ5��Ԫ�أ��������Ա���������һЩ��������������
    &CLogicSocket::_HandlePing,                             //��0������������ʵ��
    NULL,                                                   //��1�����±��0��ʼ
    NULL,                                                   //��2�����±��0��ʼ
    NULL,                                                   //��3�����±��0��ʼ
    NULL,                                                   //��4�����±��0��ʼ

    //��ʼ��������ҵ���߼�
    &CLogicSocket::_HandleRegister,                         //��5����ʵ�־����ע�Ṧ��
    &CLogicSocket::_HandleLogIn,                            //��6����ʵ�־���ĵ�¼����
    //......��������չ������ʵ�ֹ������ܣ�ʵ�ּ�Ѫ���ܵȵȣ�


};
#define AUTH_TOTAL_COMMANDS sizeof(statusHandler)/sizeof(handler) //���������ж��ٸ�������ʱ����֪��

//���캯��
CLogicSocket::CLogicSocket()
{

}
//��������
CLogicSocket::~CLogicSocket()
{

}

//��ʼ��������fork()�ӽ���֮ǰ������¡�
//�ɹ�����true��ʧ�ܷ���false
bool CLogicSocket::Initialize()
{
    //��һЩ�ͱ�����صĳ�ʼ������
    //....�պ������Ҫ��չ        
    bool bParentInit = CSocket::Initialize();  //���ø����ͬ������
    return bParentInit;
}

void CLogicSocket::SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, unsigned short iMsgCode)
{
    CMemory* p_memory = CMemory::GetInstance();

    char* p_sendbuf = (char*)p_memory->AllocMemory(m_iLenMsgHeader + m_iLenPkgHeader, false);
    char* p_tmpbuf = p_sendbuf;

    memcpy(p_tmpbuf, pMsgHeader, m_iLenMsgHeader);
    p_tmpbuf += m_iLenMsgHeader;

    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)p_tmpbuf;	  //ָ�������Ҫ���ͳ�ȥ�İ��İ�ͷ	
    pPkgHeader->msgCode = htons(iMsgCode);
    pPkgHeader->pkgLen = htons(m_iLenPkgHeader);
    pPkgHeader->crc32 = 0;
    msgSend(p_sendbuf);
    return;
}

bool CLogicSocket::_HandleRegister(lpconnection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength)
{
    //(1)�����жϰ���ĺϷ���
    if (pPkgBody == NULL) //���忴�ͻ��˷�����Լ�������Լ���������[msgCode]��������壬��ô����������壬����Ϊ�Ƕ������ֱ�Ӳ�����    
    {
        return false;
    }

    int iRecvLen = sizeof(STRUCT_REGISTER);
    if (iRecvLen != iBodyLength) //���͹����Ľṹ��С���ԣ���Ϊ�Ƕ������ֱ�Ӳ�����
    {
        return false;
    }

    //(2)����ͬһ���û�������ͬʱ��������������������ɶ���߳�ͬʱΪ�� �û����񣬱���������Ϊ�����û�Ҫ���̵�����A��Ʒ������B��Ʒ�����û���Ǯ ֻ����A����B������ͬʱ��A��B�أ�
       //������û����͹��������������һ��A��������һ��B������������߳���ִ��ͬһ���û��������β�ͬ�Ĺ�������ܿ����������û�����ɹ��� A���ֹ���ɹ��� B
       //���ԣ�Ϊ��������������ĳ���û����������һ�㶼Ҫ����,������Ҫ�����ٽ�ı�����ngx_connection_s�ṹ��
    //CLock lock(&pConn->logicPorcMutex); //���Ǻͱ��û��йصķ��ʶ�����
    std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);
    //(3)ȡ�����������͹���������
    LPSTRUCT_REGISTER p_RecvInfo = (LPSTRUCT_REGISTER)pPkgBody;
    p_RecvInfo->iType = ntohl(p_RecvInfo->iType);          //������ֵ��,short,int,long,uint64_t,int64_t���ִ�Ҷ���Ҫ���Ǵ���֮ǰ�����������յ�������ת������
    p_RecvInfo->username[sizeof(p_RecvInfo->username) - 1] = 0;//��ǳ��ؼ�����ֹ�ͻ��˷��͹������ΰ������·�����ֱ��ʹ��������ݳ��ִ��� 
    p_RecvInfo->password[sizeof(p_RecvInfo->password) - 1] = 0;//��ǳ��ؼ�����ֹ�ͻ��˷��͹������ΰ������·�����ֱ��ʹ��������ݳ��ִ��� 


    //(4)�������Ҫ���� ����ҵ���߼�����һ���ж��յ������ݵĺϷ��ԣ�
       //��ǰ����ҵ�״̬�Ƿ��ʺ��յ�������ݵȵȡ���������û�û��½�����Ͳ��ʺϹ�����Ʒ�ȵȡ�
        //�������Լ����ӣ��Լ�����ҵ����Ҫ��������룬��ʦ�Ͳ����Ŵ�������ˡ�����������������������
    //����������������

    //(5)���ͻ��˷�������ʱ��һ��Ҳ�Ƿ���һ���ṹ������ṹ���ݾ����ɿͻ���/������Э�̣��������Ǿ��Ը��ͻ���Ҳ����ͬ���� STRUCT_REGISTER �ṹ������    
    //LPSTRUCT_REGISTER pFromPkgHeader =  (LPSTRUCT_REGISTER)(((char *)pMsgHeader)+m_iLenMsgHeader);	//ָ���յ��İ��İ�ͷ���������ݺ�������Ҫ�õ�
    LPCOMM_PKG_HEADER pPkgHeader;
    CMemory* p_memory = CMemory::GetInstance();
    CCRC32* p_crc32 = CCRC32::GetInstance();
    int iSendLen = sizeof(STRUCT_REGISTER);
    //a)����Ҫ���ͳ�ȥ�İ����ڴ�

    //iSendLen = 65000; //unsigned���Ҳ�������ֵ
    char* p_sendbuf = (char*)p_memory->AllocMemory(m_iLenMsgHeader + m_iLenPkgHeader + iSendLen, false);//׼�����͵ĸ�ʽ�������� ��Ϣͷ+��ͷ+����
    //b)�����Ϣͷ
    memcpy(p_sendbuf, pMsgHeader, m_iLenMsgHeader);                   //��Ϣͷֱ�ӿ�����������
    //c)����ͷ
    pPkgHeader = (LPCOMM_PKG_HEADER)(p_sendbuf + m_iLenMsgHeader);    //ָ���ͷ
    pPkgHeader->msgCode = _CMD_REGISTER;	                        //��Ϣ���룬����ͳһ��ngx_logiccomm.h�ж���
    pPkgHeader->msgCode = htons(pPkgHeader->msgCode);	            //htons������ת������ 
    pPkgHeader->pkgLen = htons(m_iLenPkgHeader + iSendLen);        //�������ĳߴ硾��ͷ+����ߴ硿 
    //d)������
    LPSTRUCT_REGISTER p_sendInfo = (LPSTRUCT_REGISTER)(p_sendbuf + m_iLenMsgHeader + m_iLenPkgHeader);	//������Ϣͷ��������ͷ�����ǰ�����
    //�������������������Ҫ�����Ҫ���ظ��ͻ��˵�����,int����Ҫʹ��htonl()ת��short����Ҫʹ��htons()ת��

    //e)��������ȫ��ȷ���ú󣬼�������crc32ֵ
    pPkgHeader->crc32 = p_crc32->Get_CRC((unsigned char*)p_sendInfo, iSendLen);
    pPkgHeader->crc32 = htonl(pPkgHeader->crc32);

    //f)�������ݰ�
    msgSend(p_sendbuf);
    /*if(ngx_epoll_oper_event(
                                pConn->fd,          //socekt���
                                EPOLL_CTL_MOD,      //�¼����ͣ�����������
                                EPOLLOUT,           //��־���������Ҫ���ӵı�־,EPOLLOUT����д
                                0,                  //�����¼�����Ϊ���ӵģ�EPOLL_CTL_MOD��Ҫ�������, 0������   1��ȥ�� 2����ȫ����
                                pConn               //���ӳ��е�����
                                ) == -1)
    {
        ngx_log_stderr(0,"1111111111111111111111111111111111111111111111111111111111111!");
    } */

    /*
    sleep(100);  //��Ϣ��ô��ʱ��
    //������ӻ����ˣ���϶���iCurrsequence������
    if(pMsgHeader->iCurrsequence != pConn->iCurrsequence)
    {
        //Ӧ���ǲ��ȣ���Ϊ��������Ѿ���������
        ngx_log_stderr(0,"��������,%L--%L",pMsgHeader->iCurrsequence,pConn->iCurrsequence);
    }
    else
    {
        ngx_log_stderr(0,"�������Ŷ,%L--%L",pMsgHeader->iCurrsequence,pConn->iCurrsequence);
    }

    */
    //ngx_log_stderr(0,"ִ����CLogicSocket::_HandleRegister()�����ؽ��!");
    return true;
}

bool CLogicSocket::_HandleLogIn(lpconnection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength)
{
    if (pPkgBody == NULL)
    {
        return false;
    }
    int iRecvLen = sizeof(STRUCT_LOGIN);
    if (iRecvLen != iBodyLength)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);

    LPSTRUCT_LOGIN p_RecvInfo = (LPSTRUCT_LOGIN)pPkgBody;
    p_RecvInfo->username[sizeof(p_RecvInfo->username) - 1] = 0;
    p_RecvInfo->password[sizeof(p_RecvInfo->password) - 1] = 0;

    LPCOMM_PKG_HEADER pPkgHeader;
    CMemory* p_memory = CMemory::GetInstance();
    CCRC32* p_crc32 = CCRC32::GetInstance();

    int iSendLen = sizeof(STRUCT_LOGIN);
    char* p_sendbuf = (char*)p_memory->AllocMemory(m_iLenMsgHeader + m_iLenPkgHeader + iSendLen, false);
    memcpy(p_sendbuf, pMsgHeader, m_iLenMsgHeader);
    pPkgHeader = (LPCOMM_PKG_HEADER)(p_sendbuf + m_iLenMsgHeader);
    pPkgHeader->msgCode = _CMD_LOGIN;
    pPkgHeader->msgCode = htons(pPkgHeader->msgCode);
    pPkgHeader->pkgLen = htons(m_iLenPkgHeader + iSendLen);
    LPSTRUCT_LOGIN p_sendInfo = (LPSTRUCT_LOGIN)(p_sendbuf + m_iLenMsgHeader + m_iLenPkgHeader);
    pPkgHeader->crc32 = p_crc32->Get_CRC((unsigned char*)p_sendInfo, iSendLen);
    pPkgHeader->crc32 = htonl(pPkgHeader->crc32);
    //ngx_log_stderr(0,"�ɹ��յ��˵�¼�����ؽ����");
    msgSend(p_sendbuf);
    return true;
}

bool CLogicSocket::_HandlePing(lpconnection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, unsigned short iBodyLength)
{
    //������Ҫ��û�а��壻
    if (iBodyLength != 0)  //�а�������Ϊ�� �Ƿ���
        return false;

    std::lock_guard<std::mutex> lock(pConn->logicPorcMutex); //���Ǻͱ��û��йصķ��ʶ������û��⣬������û�ͬʱ���͹�����������ﵽ��������Ŀ��
    pConn->lastPingTime = time(NULL);   //���¸ñ���

    //������Ҳ���� һ��ֻ�а�ͷ�����ݰ����ͻ��ˣ���Ϊ���ص�����
    SendNoBodyPkgToClient(pMsgHeader, _CMD_PING);

    //ngx_log_stderr(0,"�ɹ��յ��������������ؽ����");
    return true;
}

void CLogicSocket::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time)
{
    CMemory* p_memory = CMemory::GetInstance();

    if (tmpmsg->iCurrsequence == tmpmsg->pConn->iCurrsequence) //������û��
    {
        lpconnection_t p_Conn = tmpmsg->pConn;

        if (/*m_ifkickTimeCount == 1 && */m_ifTimeOutKick == 1)  //�ܵ��õ���������һ�������϶����������Ե�һ�������Ӳ�������ν����Ҫ�ǵڶ�������
        {
            //��ʱ��ֱ���߳�ȥ������
            zdClosesocketProc(p_Conn);
        }
        else if ((cur_time - p_Conn->lastPingTime) > (m_iWaitTime * 3 + 10)) //��ʱ�ߵ��жϱ�׼���� ÿ�μ���ʱ����*3���������ʱ��û���������������ߡ���ҿ��Ը���ʵ����������趨��
        {
            //�߳�ȥ�������ʱ�˿̸��û����ö��ߣ������socket�����������������������Ӹ���  �����������ô��ù������������ˣ���ô���ܴ��ߣ����߾ʹ��ߡ�            
            //ngx_log_stderr(0,"ʱ�䵽�������������߳�ȥ!");   //�о�OK
            zdClosesocketProc(p_Conn);
        }

        p_memory->FreeMemory(tmpmsg);//�ڴ�Ҫ�ͷ�
    }
    else //�����Ӷ���
    {
        p_memory->FreeMemory(tmpmsg);//�ڴ�Ҫ�ͷ�
    }
    return;
}

//�����յ������ݰ������̳߳������ñ���������������һ���������̣߳�
//pMsgBuf����Ϣͷ + ��ͷ + ���� ���Խ��ͣ�
void CLogicSocket::threadRecvProcFunc(char* pMsgBuf)
{
    LPSTRUC_MSG_HEADER pMsgHeader = (LPSTRUC_MSG_HEADER)pMsgBuf;                  //��Ϣͷ
    LPCOMM_PKG_HEADER  pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuf + m_iLenMsgHeader); //��ͷ
    void* pPkgBody;                                                              //ָ������ָ��
    unsigned short pkglen = ntohs(pPkgHeader->pkgLen);                            //�ͻ���ָ���İ���ȡ���ͷ+���塿

    if (m_iLenPkgHeader == pkglen)
    {
        //û�а��壬ֻ�а�ͷ
        if (pPkgHeader->crc32 != 0) //ֻ�а�ͷ��crcֵ��0
        {
            return; //crc��ֱ�Ӷ���
        }
        pPkgBody = NULL;
    }
    else
    {
        //�а��壬�ߵ�����
        pPkgHeader->crc32 = ntohl(pPkgHeader->crc32);		          //���4�ֽڵ����ݣ�������ת������
        pPkgBody = (void*)(pMsgBuf + m_iLenMsgHeader + m_iLenPkgHeader); //������Ϣͷ �Լ� ��ͷ ��ָ�����

        //ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()���յ�����crcֵΪ%d!",pPkgHeader->crc32);

        //����crcֵ�жϰ���������        
        int calccrc = CCRC32::GetInstance()->Get_CRC((unsigned char*)pPkgBody, pkglen - m_iLenPkgHeader); //���㴿�����crcֵ
        if (calccrc != pPkgHeader->crc32) //�������˸��ݰ������crcֵ���Ϳͻ��˴��ݹ����İ�ͷ�е�crc32��Ϣ�Ƚ�
        {
            globallogger->clog(LogLevel::ERROR, "CLogicSocket::threadRecvProcFunc()��CRC����[������:%d/�ͻ���:%d]����������!", calccrc, pPkgHeader->crc32);    //��ʽ�����п��Ըɵ������Ϣ
            return; //crc��ֱ�Ӷ���
        }
        else
        {
            //ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()��CRC��ȷ[������:%d/�ͻ���:%d]������!",calccrc,pPkgHeader->crc32);
        }
    }

    //��crcУ��OK�����ߵ�����    	
    unsigned short imsgCode = ntohs(pPkgHeader->msgCode); //��Ϣ�����ó���
    lpconnection_t p_Conn = pMsgHeader->pConn;        //��Ϣͷ�в������ӳ������ӵ�ָ��

    //����Ҫ��һЩ�ж�
    //(1)������յ��ͻ��˷������İ������������ͷ�һ���̳߳��е��̴߳���ð��Ĺ����У��ͻ��˶Ͽ��ˣ�����Ȼ�������յ��İ����ǾͲ��ش����ˣ�    
    if (p_Conn->iCurrsequence != pMsgHeader->iCurrsequence)   //�����ӳ��������Ա�����tcp���ӡ�����socket��ռ�ã���˵��ԭ���� �ͻ��˺ͱ������������Ӷ��ˣ����ְ�ֱ�Ӷ�������
    {
        return; //�����������ְ��ˡ��ͻ��˶Ͽ��ˡ�
    }

    //(2)�ж���Ϣ������ȷ�ģ���ֹ�ͻ��˶����ֺ����Ƿ�����������һ���������Ƿ���������Χ�ڵ���Ϣ��
    if (imsgCode >= AUTH_TOTAL_COMMANDS) //�޷�����������<0
    {
        globallogger->clog(LogLevel::ERROR, "CLogicSocket::threadRecvProcFunc()��imsgCode=%d��Ϣ�벻��!", imsgCode); //�����ж���������ߴ�������İ���ϣ����ӡ����������˭�ɵ�
        return; //�����������ְ�����������ߴ������
    }

    //���ߵ�����ģ���û���ڣ������⣬�Ǻü����ж��Ƿ�����Ӧ�Ĵ�����
    //(3)�ж�Ӧ����Ϣ��������
    if (statusHandler[imsgCode] == NULL) //������imsgCode�ķ�ʽ����ʹ����Ҫִ�еĳ�Ա����Ч���ر��
    {
        globallogger->clog(LogLevel::ERROR, "CLogicSocket::threadRecvProcFunc()��imsgCode=%d��Ϣ���Ҳ�����Ӧ�Ĵ�����!", imsgCode); //�����ж���������ߴ�������İ���ϣ����ӡ����������˭�ɵ�
        return;  //û����صĴ�����
    }

    //һ����ȷ�����Է��Ĵ󵨵Ĵ�����
    //(4)������Ϣ���Ӧ�ĳ�Ա����������
    (this->*statusHandler[imsgCode])(p_Conn, pMsgHeader, (char*)pPkgBody, pkglen - m_iLenPkgHeader);
    return;
}