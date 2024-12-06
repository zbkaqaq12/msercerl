#include"CSocket.h"
#include"global.h"
#include"CMemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO��
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno
//#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>

//---------------------------------------------------------------
/**
 * @brief ���캯������ʼ�����Ӷ���
 * @details ��ʼ�����Ӷ���ĳ�Ա�������趨��ʼ״̬��`iCurrsequence` ����Ϊ0��`fd` ����Ϊ-1����ʾ��δ�������ӡ�
 */
connection_s::connection_s()//���캯��
{
    iCurrsequence = 0;
    //pthread_mutex_init(&logicPorcMutex, NULL); //��������ʼ��
}
/**
 * @brief ����������������Դ
 * @details �������������ͷŸ����Ӷ������Դ���˺���Ŀǰ���ͷ��˻�������ע�͵�����ش��룩��
 */
connection_s::~connection_s()//��������
{
    //pthread_mutex_destroy(&logicPorcMutex);    //�������ͷ�
}

/**
 * @brief ��ʼ�����Ӷ���׼��ʹ��
 * @details ��һ�����ӱ������ȥʱ�����ô˺�������ʼ�����ӵ�״̬�ͱ�Ҫ�ĳ�Ա��������Ҫ�����¹�����
 * - ���ӵ�ǰ���к� `iCurrsequence`
 * - �� `fd` ����Ϊ-1
 * - ��ʼ����ͷ����״̬�ͽ��ջ����� `precvbuf`
 * - ��ʼ�����ͻ�����ָ�� `psendMemPointer`
 * - ��ʼ�����Ͷ��м����� `iThrowsendCount`
 * - ����ʱ��� `lastPingTime` �ͷ�ֹFlood��������ؼ���
 */
void connection_s::GetOneToUse()
{
    ++iCurrsequence;

    fd = -1;                                         //��ʼ�ȸ�-1
    curStat = _PKG_HD_INIT;                           //�հ�״̬���� ��ʼ״̬��׼���������ݰ�ͷ��״̬����
    precvbuf = dataHeadInfo;                          //�հ���Ҫ���յ�����������Ϊ��Ҫ���հ�ͷ�����������ݵ�buffֱ�Ӿ���dataHeadInfo
    irecvlen = sizeof(COMM_PKG_HEADER);               //����ָ�������ݵĳ��ȣ�������Ҫ���հ�ͷ��ô���ֽڵ�����

    precvMemPointer = NULL;                         //��Ȼûnew�ڴ棬����Ȼָ����ڴ��ַ�ȸ�NULL
    iThrowsendCount = 0;                            //ԭ�ӵ�
    psendMemPointer = NULL;                         //��������ͷָ���¼
    events = 0;                            //epoll�¼��ȸ�0 
    lastPingTime = time(NULL);                   //�ϴ�ping��ʱ��

    FloodkickLastTime = 0;                            //Flood�����ϴ��յ�����ʱ��
    FloodAttackCount = 0;	                          //Flood�����ڸ�ʱ�����յ����Ĵ���ͳ��
    iSendCount = 0;                            //���Ͷ������е�������Ŀ������clientֻ�����գ��������ɴ����������ݴ��������߳����� 
}

/**
 * @brief �������Ӳ����������Դ
 * @details �����ӱ�����ʱ�����ô˺������ͷ��ѷ������Դ����Ҫ�����¹�����
 * - ���ӵ�ǰ���к� `iCurrsequence`
 * - ������ջ������ڴ汻����������ͷŸ��ڴ�
 * - ������ͻ������ڴ汻����������ͷŸ��ڴ�
 * - ���㷢�ͼ����� `iThrowsendCount`
 */
void connection_s::PutOneToFree()
{
    ++iCurrsequence;
    if (precvMemPointer != NULL)//����������������ӷ�����������ݵ��ڴ棬��Ҫ�ͷ��ڴ�
    {
        CMemory::GetInstance()->FreeMemory(precvMemPointer);
        precvMemPointer = NULL;
    }
    if (psendMemPointer != NULL) //����������ݵĻ������������ݣ���Ҫ�ͷ��ڴ�
    {
        CMemory::GetInstance()->FreeMemory(psendMemPointer);
        psendMemPointer = NULL;
    }

    iThrowsendCount = 0;                              //���ò����øо�����         
}

//---------------------------------------------------------------
/**
 * @brief ��ʼ�����ӳ�
 * @details ��������ʼ�����ӳء��ڳ�ʼ�������У�
 * - Ϊÿ�����ӷ����ڴ棬�����ù��캯������ʼ�����Ӷ���
 * - ��ʼ�����������б���������б�
 * - �������ӳص��������� `m_total_connection_n` �Ϳ��������� `m_free_connection_n`
 */
void CSocket::initconnection()
{
    lpconnection_t p_Conn;
    CMemory* p_memory = CMemory::GetInstance();

    int ilenconnpool = sizeof(connection_t);
    for (int i = 0; i < m_worker_connections; ++i) //�ȴ�����ô������ӣ���������������
    {
        p_Conn = (lpconnection_t)p_memory->AllocMemory(ilenconnpool, true); //�����ڴ� , ��Ϊ��������ڴ�new char���޷�ִ�й��캯�����������£�
        //�ֹ����ù��캯������ΪAllocMemory���޷����ù��캯��
        p_Conn = new(p_Conn) connection_t();  //��λnew��������ٶȡ����ͷ�����ʽ����p_Conn->~ngx_connection_t();		
        p_Conn->GetOneToUse();
        m_connectionList.push_back(p_Conn);     //�������ӡ������Ƿ���С����������list
        m_freeconnectionList.push_back(p_Conn); //�������ӻ�������list
    } //end for
    m_free_connection_n = m_total_connection_n = m_connectionList.size(); //��ʼ�������б�һ����
    return;
}

/**
 * @brief ���ղ��������ӳ��е���������
 * @details �������ӳ��е��������ӣ��ֶ�����ÿ�����ӵ��������������ͷ��ڴ档�˺������������ӳ�ʱ�����á�
 */
void CSocket::clearconnection()
{
    lpconnection_t p_Conn;
    CMemory* p_memory = CMemory::GetInstance();

    while (!m_connectionList.empty())
    {
        p_Conn = m_connectionList.front();
        m_connectionList.pop_front();
        p_Conn->~connection_t();     //�ֹ�������������
        p_memory->FreeMemory(p_Conn);
    }
}

/**
 * @brief �����ӳ��л�ȡһ����������
 * @param isock �����ӵ��׽���
 * @return lpconnection_t ����һ�����õ����Ӷ���
 * @details ������ӳ����п������ӣ���ӿ��������б��з���һ������ʼ�������û�п������ӣ��򴴽�һ���µ����Ӳ����ء�ÿ�����ӽ���һ��TCP���ӵ��׽��֡�
 */
lpconnection_t CSocket::get_connection(int isock)
{
    //��Ϊ�����������߳�Ҫ����m_freeconnectionList��m_connectionList�����������ר�ŵ��ͷ��߳�Ҫ�ͷ�/�������߳�Ҫ�ͷš�֮��ģ�����Ӧ���ٽ�һ��
    // ʹ�� std::lock_guard �����Զ������ͽ���
    std::lock_guard<std::mutex> lock(m_connectionMutex);

    if (!m_freeconnectionList.empty())
    {
        //�п��еģ���Ȼ�Ǵӿ��е���ժȡ
        lpconnection_t p_Conn = m_freeconnectionList.front(); //���ص�һ��Ԫ�ص������Ԫ�ش������
        m_freeconnectionList.pop_front();                         //�Ƴ���һ��Ԫ�ص�������	
        p_Conn->GetOneToUse();
        --m_free_connection_n;
        p_Conn->fd = isock;
        return p_Conn;
    }

    //�ߵ������ʾû���е������ˣ��ǾͿ������´���һ������
    CMemory* p_memory = CMemory::GetInstance();
    lpconnection_t p_Conn = (lpconnection_t)p_memory->AllocMemory(sizeof(connection_t), true);
    p_Conn = new(p_Conn) connection_t();
    p_Conn->GetOneToUse();
    m_connectionList.push_back(p_Conn); //�뵽�ܱ��������������뵽���б���������Ϊ���ǵ���������ģ��϶���Ҫ��������ӵ�
    ++m_total_connection_n;
    p_Conn->fd = isock;
    return p_Conn;

    
}

/**
 * @brief �����ӹ黹�����ӳ�
 * @param pConn ��Ҫ�黹�����Ӷ���
 * @details ��ָ�������ӹ黹�����ӳأ����Ӷ���������Դ�����ͷţ��������ӷ�����������б��С�
 *          ʹ�û�������ȷ���ڶ��̻߳����°�ȫ�������ӳء�
 */
void CSocket::free_connection(lpconnection_t pConn)
{
    //��Ϊ���߳̿���Ҫ�����ӳ������ӣ������ں�����Ҳ�Ǳ�Ҫ��
    // ʹ�� std::lock_guard �����Զ������ͽ���
    std::lock_guard<std::mutex> lock(m_connectionMutex);

    //������ȷһ�㣬���ӣ���������ȫ������m_connectionList�
    pConn->PutOneToFree();

    //�ӵ����������б���
    m_freeconnectionList.push_back(pConn);

    //����������+1
    ++m_free_connection_n;

   
    return;
}


/**
 * @brief �����ӷ�������ն��У��Ժ���ר���̴߳���
 * @param pConn ��Ҫ���յ����Ӷ���
 * @details ������Ӷ���û�б����չ������������������Ӷ��� `m_recyconnectionList` �У�����¼����ʱ�䡣
 *          �ö����е����ӽ����Ժ��� `ServerRecyConnectionThread` �߳̽��д���ʹ�û�������ȷ�����̻߳����°�ȫ�������ն��С�
 */
void CSocket::inRecyConnectQueue(lpconnection_t pConn)
{
  

    std::list<lpconnection_t>::iterator pos;
    bool iffind = false;

    // ʹ�� std::lock_guard �����Զ������ͽ���
    std::lock_guard<std::mutex> lock(m_connectionMutex); //������ӻ����б�Ļ���������Ϊ�߳�ServerRecyConnectionThread()Ҳ��Ҫ�õ���������б�

    //�����жϷ�ֹ���ӱ�����ӵ�����վ����
    for (pos = m_recyconnectionList.begin(); pos != m_recyconnectionList.end(); ++pos)
    {
        if ((*pos) == pConn)
        {
            iffind = true;
            break;
        }
    }
    if (iffind == true) //�ҵ��ˣ�����������
    {
        //��������֤���ֻ��һ����
        return;
    }

    pConn->inRecyTime = time(NULL);        //��¼����ʱ��
    ++pConn->iCurrsequence;
    m_recyconnectionList.push_back(pConn); //�ȴ�ServerRecyConnectionThread�߳��Իᴦ�� 
    ++m_totol_recyconnection_n;            //���ͷ����Ӷ��д�С+1
    --m_onlineUserCount;                   //�����û�����-1
    return;
}

/**
 * @brief �������ӻ��յ��߳�
 * @param threadData �߳����ݣ������߳���ص���Ϣ
 * @return ���ؿ�ָ��
 * @details �˺�����һ���߳�ѭ�������ڼ�鲢�����ѵ�����ʱ������ӡ�ÿ200����ִ��һ�μ�飬
 *          ������������������������Ӵ����ն������Ƴ����黹�����ӳ��С�
 *          ����������������δ���յ����ӽ���ǿ�ƻ��ա�
 */
void* CSocket::ServerRecyConnectionThread(void* threadData)
{
    ThreadItem* pThread = static_cast<ThreadItem*>(threadData);
    CSocket* pSocketObj = pThread->_pThis;

    time_t currtime;
    //int err;
    std::list<lpconnection_t>::iterator pos, posend;
    lpconnection_t p_Conn;

    while (true)
    {
        // ÿ����Ϣ200����
        usleep(200 * 1000);  // ��λ��΢�200����

        // �������ӻ���
        if (pSocketObj->m_totol_recyconnection_n > 0)
        {
            currtime = time(NULL);

            // ʹ�� std::lock_guard ���Զ�������
            {
                std::lock_guard<std::mutex> lock(pSocketObj->m_recyconnqueueMutex);

                pos = pSocketObj->m_recyconnectionList.begin();
                posend = pSocketObj->m_recyconnectionList.end();

                while (pos != posend)
                {
                    p_Conn = (*pos);

                    // �ж������Ƿ��ѵ�����ʱ��
                    if ((p_Conn->inRecyTime + pSocketObj->m_RecyConnectionWaitTime) > currtime && g_stopEvent == 0)
                    {
                        ++pos;
                        continue; // û������ʱ�䣬����
                    }

                    // ������ʱ�䣬�� iThrowsendCount == 0 ���ܻ���
                    if (p_Conn->iThrowsendCount > 0)
                    {
                        globallogger->clog(LogLevel::ERROR, "CSocekt::ServerRecyConnectionThread()�е��ͷ�ʱ��ȴ����p_Conn.iThrowsendCount != 0��������÷���");
                    }

                    // ִ�����ӻ���
                    --pSocketObj->m_totol_recyconnection_n;
                    pSocketObj->m_recyconnectionList.erase(pos); // ɾ���ѻ��յ�����
                    pSocketObj->free_connection(p_Conn); // �黹����

                    // ��������ʣ�������
                    pos = pSocketObj->m_recyconnectionList.begin();
                    posend = pSocketObj->m_recyconnectionList.end();
                }
            }
        }

        // �������Ҫ�˳�
        if (g_stopEvent == 1)
        {
            // ǿ���ͷ���������
            if (pSocketObj->m_totol_recyconnection_n > 0)
            {
                std::lock_guard<std::mutex> lock(pSocketObj->m_recyconnqueueMutex);

                pos = pSocketObj->m_recyconnectionList.begin();
                posend = pSocketObj->m_recyconnectionList.end();

                while (pos != posend)
                {
                    p_Conn = (*pos);
                    --pSocketObj->m_totol_recyconnection_n;
                    pSocketObj->m_recyconnectionList.erase(pos);
                    pSocketObj->free_connection(p_Conn);
                    pos = pSocketObj->m_recyconnectionList.begin();
                    posend = pSocketObj->m_recyconnectionList.end();
                }
            }
            break; // �����˳����˳�ѭ��
        }
    }

    return nullptr;
}

/**
 * @brief �رղ��ͷ�����
 * @param pConn ��Ҫ�رպ��ͷŵ����Ӷ���
 * @details �ú������ڹر����Ӳ��ͷ������Դ�����Ƚ����ӹ黹�����ӳ��У�������ӵ��ļ���������fd����Ч��
 *          ��ر����Ӳ�������Ϊ��Ч��fd = -1����
 */
void CSocket::close_connection(lpconnection_t pConn)
{
    //pConn->fd = -1; //�ٷ�nginx��ôд����ôд�����壻    ��Ҫ�������������ʱ��Ҫ���׶�������ߵ�����
    free_connection(pConn);
    if (pConn->fd != -1)
    {
        close(pConn->fd);
        pConn->fd = -1;
    }
    return;
}