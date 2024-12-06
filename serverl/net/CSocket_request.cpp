#include"global.h"
#include"CSocket.h"
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include <pthread.h>   //���߳�
#include "CMemory.h"

/**
 * @brief ������յ������ݰ�
 * @details �ú������ڴ��������н��յ������ݡ��������������ݵ���ʱ��`epoll_process_events()` ����ô˺����������ݵĽ����ʹ���
 *          �˺������������״̬������հ�ͷ�����հ��壩��ͬ��ִ�в�ͬ�Ĵ����߼���
 *          ���������Flood������⣬������Ƿ����Flood������
 *
 * @param pConn ���Ӷ��󣬰�����ǰ���ӵ����ݽ��ջ������ͽ���״̬����Ϣ
 */
void CSocket::read_request_handler(lpconnection_t pConn)
{
    bool isflood = false; //�Ƿ�flood������

    //�հ���ע�������õĵڶ����͵����������������õ�ʼ����������������������Ǳ��뱣֤ c->precvbufָ����ȷ���հ�λ�ã���֤c->irecvlenָ����ȷ���հ����
    ssize_t reco = recvproc(pConn, pConn->precvbuf, pConn->irecvlen);
    if (reco <= 0)
    {
        return;//�ô�����ϱ����recvproc()����������ˣ�����<=0��ֱ��return        
    }

    //�ߵ����˵���ɹ��յ���һЩ�ֽڣ�>0������Ҫ��ʼ�ж��յ��˶���������     
    if (pConn->curStat == _PKG_HD_INIT) //���ӽ�������ʱ�϶������״̬����Ϊ��get_connection()���Ѿ���curStat��Ա��ֵ��_PKG_HD_INIT��
    {
        if (reco == m_iLenPkgHeader)//�����յ�������ͷ���������ͷ
        {
            wait_request_handler_proc_p1(pConn, isflood); //�Ǿ͵���ר����԰�ͷ���������ĺ���ȥ����ѡ�
        }
        else
        {
            //�յ��İ�ͷ������--���ǲ���Ԥ��ÿ�����ĳ��ȣ�Ҳ����Ԥ�ϸ��ֲ��/ճ������������յ���������ͷ��Ҳ����ȱ�����Ǻܿ��ܵģ�
            pConn->curStat = _PKG_HD_RECVING;                 //���հ�ͷ�У���ͷ���������������հ�ͷ��	
            pConn->precvbuf = pConn->precvbuf + reco;              //ע���պ��������ڴ�������
            pConn->irecvlen = pConn->irecvlen - reco;              //Ҫ�յ����ݵ�ȻҪ���٣���ȷ��ֻ�յ������İ�ͷ��
        } //end  if(reco == m_iLenPkgHeader)
    }
    else if (pConn->curStat == _PKG_HD_RECVING) //���հ�ͷ�У���ͷ�����������������У���������Ż����
    {
        if (pConn->irecvlen == reco) //Ҫ���յ��Ŀ�Ⱥ���ʵ���յ��Ŀ�����
        {
            //��ͷ��������
            wait_request_handler_proc_p1(pConn, isflood); //�Ǿ͵���ר����԰�ͷ���������ĺ���ȥ����ѡ�
        }
        else
        {
            //��ͷ����û�������������հ�ͷ
            //pConn->curStat        = _PKG_HD_RECVING;                 //û��Ҫ
            pConn->precvbuf = pConn->precvbuf + reco;              //ע���պ��������ڴ�������
            pConn->irecvlen = pConn->irecvlen - reco;              //Ҫ�յ����ݵ�ȻҪ���٣���ȷ��ֻ�յ������İ�ͷ��
        }
    }
    else if (pConn->curStat == _PKG_BD_INIT)
    {
        //��ͷ�պ����꣬׼�����հ���
        if (reco == pConn->irecvlen)
        {
            //�յ��Ŀ�ȵ���Ҫ�յĿ�ȣ�����Ҳ��������
            if (m_floodAkEnable == 1)
            {
                //Flood��������Ƿ���
                isflood = TestFlood(pConn);
            }
            wait_request_handler_proc_plast(pConn, isflood);
        }
        else
        {
            //�յ��Ŀ��С��Ҫ�յĿ��
            pConn->curStat = _PKG_BD_RECVING;
            pConn->precvbuf = pConn->precvbuf + reco;
            pConn->irecvlen = pConn->irecvlen - reco;
        }
    }
    else if (pConn->curStat == _PKG_BD_RECVING)
    {
        //���հ����У����岻����������������
        if (pConn->irecvlen == reco)
        {
            //������������
            if (m_floodAkEnable == 1)
            {
                //Flood��������Ƿ���
                isflood = TestFlood(pConn);
            }
            wait_request_handler_proc_plast(pConn, isflood);
        }
        else
        {
            //����û��������������
            pConn->precvbuf = pConn->precvbuf + reco;
            pConn->irecvlen = pConn->irecvlen - reco;
        }
    }  //end if(c->curStat == _PKG_HD_INIT)

    if (isflood == true)
    {
        //�ͻ���flood����������ֱ�Ӱѿͻ����ߵ�
        //log_stderr(errno,"���ֿͻ���flood���ɵ��ÿͻ���!");
        zdClosesocketProc(pConn);
    }

    return;
}


/**
 * @brief �������ݲ�������մ���
 * @details �ú������ڴӿͻ��˽������ݲ����д�����������չ����з������������ӹرա�ϵͳ����ȣ������Զ��ر����Ӳ��ͷ���Դ��������ճɹ����򷵻�ʵ�ʽ��յ����ֽ�����
 *
 * @param c ���Ӷ��󣬰������ӵ��ļ���������������Ϣ
 * @param buff �洢�������ݵĻ�����
 * @param buflen ��������С����ʾ�����յ�������
 * @return ���ؽ��յ����ֽ�����������������򷵻�-1���ر�����
 */
ssize_t CSocket::recvproc(lpconnection_t c, char* buff, ssize_t buflen)
{
	ssize_t n;

	n = recv(c->fd, buff, buflen, 0); //recv()ϵͳ������ ���һ������flag��һ��Ϊ0��     
	if (n == 0)
	{
		//�ͻ��˹رա�Ӧ�������������4�λ��֡�������߾�ֱ�ӻ����������ӣ��ر�socket���� 
		//log_stderr(0,"���ӱ��ͻ��������ر�[4·���ֹر�]��");
		close_connection(c);
		return -1;
	}
	//�ͻ���û�ϣ������� 
	if (n < 0) //�ⱻ��Ϊ�д�����
	{
		//EAGAIN��EWOULDBLOCK[�����Ӧ�ó�����hp�ϡ�Ӧ����һ����ֵ����ʾû�յ����ݣ�һ����������ETģʽ�»�������������ΪETģʽ���ǲ�ͣ��recv�϶���һ��ʱ���յ����errno����LTģʽ��һ�������¼����գ����Բ��ó����������ֵ
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			//����ΪLTģʽ���ó������errno�����������ʵҲ���Ǵ������Բ�����������
			globallogger->flog(LogLevel::ERROR, "CSocket::recvproc()��errno == EAGAIN || errno == EWOULDBLOCK���������������ϣ�");//epollΪLTģʽ��Ӧ�ó����������ֵ������ֱ�Ӵ�ӡ��������
			return -1; //������������ֻ�Ǽ򵥷���
		}
		//EINTR����Ĳ�������������ĳ����ϵͳ���õ�һ�����̲���ĳ���ź�����Ӧ�źŴ���������ʱ����ϵͳ���ÿ��ܷ���һ��EINTR����
		//���磺��socket�������ˣ��������źŲ�����ƣ����ӽ��̣����ڸ�������������ϵͳ����ʱ�ɸ����̲�����һ����Ч�ź�ʱ���ں˻���ʹaccept����һ��EINTR����(���жϵ�ϵͳ����)��
		if (errno == EINTR)  //�������������Ҳο��ٷ�nginx���ٷ�nginx����Ͳ������
		{
			//����ΪLTģʽ���ó������errno�����������ʵҲ���Ǵ������Բ�����������
			globallogger->flog(LogLevel::ERROR, "CSocket::recvproc()��errno == EINTR���������������ϣ�");//epollΪLTģʽ��Ӧ�ó����������ֵ������ֱ�Ӵ�ӡ��������
			return -1; //������������ֻ�Ǽ򵥷���
		}

		//���д������������Ĵ��󣬶���Ϊ�쳣����ζ������Ҫ�رտͻ����׽���Ҫ�������ӳ������ӣ�

		//errno�ο���http://dhfapiran1.360drm.com        
		if (errno == ECONNRESET)  //#define ECONNRESET 104 /* Connection reset by peer */
		{
			//����ͻ���û�������ر�socket���ӣ�ȴ�ر����������г������ǹ��ֱ�����ģ�Ӧ����ֱ�Ӹ�����������rst��������4�λ��ְ�������ӶϿ�������ô������������            
			//10054(WSAECONNRESET)--Զ�̳����������ӵ�ʱ��رջ�����������--Զ������ǿ�ȹر���һ�����е�����
			//�㳣�����ɡ���ͨ��Ϣ�͡�����־�����ô�ӡ��ûɶ��˼��̫��ͨ�Ĵ���
			//do nothing

			//....һЩ��������ĺ���ͨ�Ĵ�����Ϣ��Ҳ�������������Ӹ��֣�����Ҫ�������ƣ�һ����λ�������ܣ��ܶ���������򾭹��ܶ�������ƲűȽ�Բ����
		}
		else
		{
			//���ߵ�����ģ�����ʾ�����Ҵ�ӡһ����־��ϣ��֪��һ����ɶ������׼����ӡ����Ļ��
			globallogger->flog(LogLevel::ERROR, "CSocket::recvproc()�з��������Ҵ�ӡ����������ɶ����");  //��ʽ��Ӫʱ���Կ�����Щ��־��ӡȥ��
		}

		//log_stderr(0,"���ӱ��ͻ��� �� �����رգ�");

		//���������Ĵ����Ҫ��ֱ�ӹر��׽��֣��ͷ����ӳ���������
		close_connection(c);
		return -1;
	}

	//���ߵ�����ģ�����Ϊ�յ�����Ч����
	return n; //�����յ����ֽ���
}


/**
 * @brief ��ͷ��������Ĵ�����ʼ�������
 * @details �ڽ��յ������İ�ͷ�󣬻��жϰ��ĺϷ��ԣ������ݰ��ĳ��ȷ����ڴ���а�����ա������ͷ��Ч����������հ��壻�����ͷ���Ϸ�����������Ӳ�����״̬��
 *
 * @param pConn ��ǰ����
 * @param isflood ���ò���������Flood�������
 */
void CSocket::wait_request_handler_proc_p1(lpconnection_t pConn, bool& isflood)
{
    CMemory* p_memory = CMemory::GetInstance();

    LPCOMM_PKG_HEADER pPkgHeader;
    pPkgHeader = (LPCOMM_PKG_HEADER)pConn->dataHeadInfo; //�����յ���ͷʱ����ͷ��Ϣ�϶�����dataHeadInfo�

    unsigned short e_pkgLen;
    e_pkgLen = ntohs(pPkgHeader->pkgLen);  //ע������������ת���������д��䵽�����ϵ�2�ֽ����ݣ���Ҫ��htons()ת�����������д��������յ���2�ֽ����ݣ���Ҫ��ntohs()ת�ɱ�����
    //ntohs/htons��Ŀ�ľ��Ǳ�֤��ͬ����ϵͳ����֮���շ�����ȷ�ԣ������ܿͻ���/��������ʲô����ϵͳ�����͵������Ƕ��٣��յ��ľ��Ƕ��١�
    //�����׵�ͬѧ��ֱ�Ӱٶ�����"�����ֽ���" "�����ֽ���" "c++ ���" "c++ С��"
//��������ߴ�������ж�
    if (e_pkgLen < m_iLenPkgHeader)
    {
        //α���/���߰����󣬷�������������ô���ܱȰ�ͷ��С�������������ǰ�ͷ+���壬�������Ϊ0�ֽڣ���ô����e_pkgLen == m_iLenPkgHeader��
        //�����ܳ��� < ��ͷ���ȣ��϶��Ƿ��û����ϰ�
        //״̬�ͽ���λ�ö���ԭ����Щֵ���б�Ҫ����Ϊ�п���������״̬����_PKG_HD_RECVING״̬�������������
        pConn->curStat = _PKG_HD_INIT;
        pConn->precvbuf = pConn->dataHeadInfo;
        pConn->irecvlen = m_iLenPkgHeader;
    }
    else if (e_pkgLen > (_PKG_MAX_LENGTH - 1000))   //�ͻ��˷�������Ȼ˵������ > 29000?�϶��Ƕ����
    {
        //�������̫���϶��Ƿ��û����ϰ�����ͷ��˵������ܳ�����ô���ⲻ�С�
        //״̬�ͽ���λ�ö���ԭ����Щֵ���б�Ҫ����Ϊ�п���������״̬����_PKG_HD_RECVING״̬�������������
        pConn->curStat = _PKG_HD_INIT;
        pConn->precvbuf = pConn->dataHeadInfo;
        pConn->irecvlen = m_iLenPkgHeader;
    }
    else
    {
        //�Ϸ��İ�ͷ����������
        //������Ҫ�����ڴ濪ʼ�հ��壬��Ϊ���峤�Ȳ����ǹ̶��ģ������ڴ�϶�Ҫnew������
        char* pTmpBuffer = (char*)p_memory->AllocMemory(m_iLenMsgHeader + e_pkgLen, false); //�����ڴ桾������ ��Ϣͷ����  + ��ͷ���� + ���峤�ȡ����������ȸ�false����ʾ�ڴ治��Ҫmemset;        
        pConn->precvMemPointer = pTmpBuffer;  //�ڴ濪ʼָ��

        //a)����д��Ϣͷ����
        LPSTRUC_MSG_HEADER ptmpMsgHeader = (LPSTRUC_MSG_HEADER)pTmpBuffer;
        ptmpMsgHeader->pConn = pConn;
        ptmpMsgHeader->iCurrsequence = pConn->iCurrsequence; //�յ���ʱ�����ӳ���������ż�¼����Ϣͷ�������Ա������ã�
        //b)����д��ͷ����
        pTmpBuffer += m_iLenMsgHeader;                 //��������������Ϣͷ��ָ���ͷ
        memcpy(pTmpBuffer, pPkgHeader, m_iLenPkgHeader); //ֱ�Ӱ��յ��İ�ͷ��������
        if (e_pkgLen == m_iLenPkgHeader)
        {
            //�ñ���ֻ�а�ͷ�ް��塾��������һ����ֻ�а�ͷ��û�а��塿
            //���൱���������ˣ���ֱ������Ϣ���д�����ҵ���߼��߳�ȥ�����
            if (m_floodAkEnable == 1)
            {
                //Flood��������Ƿ���
                isflood = TestFlood(pConn);
            }
            wait_request_handler_proc_plast(pConn, isflood);
        }
        else
        {
            //��ʼ�հ��壬ע���ҵ�д��
            pConn->curStat = _PKG_BD_INIT;                   //��ǰ״̬�����ı䣬��ͷ�պ����꣬׼�����հ���	    
            pConn->precvbuf = pTmpBuffer + m_iLenPkgHeader;  //pTmpBufferָ���ͷ������ + m_iLenPkgHeader��ָ����� weizhi
            pConn->irecvlen = e_pkgLen - m_iLenPkgHeader;    //e_pkgLen������������ͷ+���塿��С��-m_iLenPkgHeader����ͷ��  = ����
        }
    }  //end if(e_pkgLen < m_iLenPkgHeader) 

    return;
}

/**
 * @brief �����յ��������������׶�
 * @details �ú����������յ��������󣬽���������Ϣ���в������̳߳ش��������⵽Flood������������������ͷ��ڴ档������Ϻ󣬻ָ����ӵ�״̬�Ա������һ������
 *
 * @param pConn ��ǰ���Ӷ���
 * @param isflood ���ò���������ָʾ�Ƿ��⵽Flood����
 */
void CSocket::wait_request_handler_proc_plast(lpconnection_t pConn, bool& isflood)
{
    //������ڴ�ŵ���Ϣ����������
    //int irmqc = 0;  //��Ϣ���е�ǰ��Ϣ����
    //inMsgRecvQueue(c->precvMemPointer,irmqc); //������Ϣ���е�ǰ��Ϣ����irmqc���ǵ��ñ����������Ϣ��������Ϣ����
    //�����̳߳��е�ĳ���߳�������ҵ���߼�
    //g_threadpool.Call(irmqc);

    if (isflood == false)
    {
        g_threadpool.inMsgRecvQueueAndSignal(pConn->precvMemPointer); //����Ϣ���в������̴߳�����Ϣ
    }
    else
    {
        //�����й�������Ķ��ˣ��Ȱ����İ�����
        CMemory* p_memory = CMemory::GetInstance();
        p_memory->FreeMemory(pConn->precvMemPointer); //ֱ���ͷŵ��ڴ棬����������Ϣ������
    }

    pConn->precvMemPointer = NULL;
    pConn->curStat = _PKG_HD_INIT;     //�հ�״̬����״̬�ָ�Ϊԭʼ̬��Ϊ����һ������׼��                    
    pConn->precvbuf = pConn->dataHeadInfo;  //���ú��հ���λ��
    pConn->irecvlen = m_iLenPkgHeader;  //���ú�Ҫ�������ݵĴ�С
    return;
}

/**
 * @brief �������ݲ�������״̬
 * @details �ú������������ӷ������ݡ���������ַ��ͽ���������ɹ����͡����ͻ��������ͶԶ˶Ͽ����ӵ������������ͻ�������������-1������Զ˶Ͽ�������0����������������󣬷���-2��
 *
 * @param c ��ǰ���Ӷ���
 * @param buff Ҫ���͵����ݻ�����
 * @param size �������ݵĴ�С
 * @return ���سɹ����͵��ֽ��������ʾ����ĸ�ֵ��
 *         > 0: ���ͳɹ����ֽ���
 *         = 0: �Զ��ѹر�����
 *         -1: ���ͻ���������EAGAIN��
 *         -2: ������������
 */
ssize_t CSocket::sendproc(lpconnection_t c, char* buff, ssize_t size)  //ssize_t���з������ͣ���32λ�����ϵ�ͬ��int����64λ�����ϵ�ͬ��long int��size_t�����޷����͵�ssize_t
{
    //����ο�����˹ٷ�nginx����unix_send()��д��
    ssize_t   n;

    for (;; )
    {
        n = send(c->fd, buff, size, 0); //send()ϵͳ������ ���һ������flag��һ��Ϊ0�� 
        if (n > 0) //�ɹ�������һЩ����
        {
            //���ͳɹ�һЩ���ݣ��������˶��٣��������ﲻ���ģ�Ҳ����Ҫ�ٴ�send
            //�������������
            //(1) n == sizeҲ�����뷢�Ͷ��ٶ����ͳɹ��ˣ����ʾ��ȫ�������
            //(2) n < size û������ϣ��ǿ϶��Ƿ��ͻ��������ˣ�����Ҳ����Ҫ���Է��ͣ�ֱ�ӷ��ذ�
            return n; //���ر��η��͵��ֽ���
        }

        if (n == 0)
        {
            //send()����0�� һ��recv()����0��ʾ�Ͽ�,send()����0���������ֱ�ӷ���0�ɡ��õ����ߴ������Ҹ�����Ϊsend()����0��Ҫô�㷢�͵��ֽ���0��Ҫô�Զ˿��ܶϿ���
            //���������ϣ�send=0��ʾ��ʱ���Է������ر������ӹ���
            //����д����Ҫ��ѭһ��ԭ�����ӶϿ������ǲ�����send�����ﴦ������ر�socket���ֶ��������е�recv���ﴦ������send,recv�������������ӶϿ��ر�socket�������
            //���ӶϿ�epoll��֪ͨ���� recvproc()��ᴦ���������ﴦ��
            return 0;
        }

        if (errno == EAGAIN)  //�ⶫ��Ӧ�õ���EWOULDBLOCK
        {
            //�ں˻�������������������
            return -1;  //��ʾ���ͻ���������
        }

        if (errno == EINTR)
        {
            //���Ӧ��Ҳ������� ���յ�ĳ���źŵ���send�����������
            //�ο��ٷ���д������ӡ����־������ɶҲû�ɣ��Ǿ��ǵ��´�forѭ������send��һ����
            globallogger->clog(LogLevel::ERROR, "CSocket::sendproc()��send()ʧ��.");  //��ӡ����־����ɶʱ����������
            //��������Ҫ��ʲô�����´�forѭ����            
        }
        else
        {
            //�ߵ������ʾ�����������룬����ʾ���󣬴�����Ҳ���Ͽ�socket����Ҳ��Ȼ�ȴ�recv()��ͳһ����Ͽ�����Ϊ���Ƕ��̣߳�send()Ҳ����Ͽ���recv()Ҳ����Ͽ������Ѵ����
            return -2;
        }
    } //end for
}

/**
 * @brief �������ݷ�������
 * @details �����ݿ�дʱ����epoll֪ͨ�˺������ú����᳢�Է������ݣ�������ͻ��������򷵻�-1�����ͳɹ����������ʣ������ݡ����ݷ�����Ϻ󣬻��epoll���Ƴ�д�¼���
 *          ������͹����з������󣬻��¼������־�����������ȫ������ϣ����֪ͨ�����̼߳���ִ�С�
 *
 * @param pConn ��ǰ���Ӷ��󣬰����������ݵĻ����������ݳ��ȵ���Ϣ
 */
void CSocket::write_request_handler(lpconnection_t pConn)
{
    CMemory* p_memory = CMemory::GetInstance();

    //��Щ�������д���Բ��� void* CSocket::ServerSendQueueThread(void* threadData)
    ssize_t sendsize = sendproc(pConn, pConn->psendbuf, pConn->isendlen);

    if (sendsize > 0 && sendsize != pConn->isendlen)
    {
        //û��ȫ��������ϣ�����ֻ����ȥ��һ���֣���ô���͵������ʣ����٣�������¼�������´�sendproc()ʱʹ��
        pConn->psendbuf = pConn->psendbuf + sendsize;
        pConn->isendlen = pConn->isendlen - sendsize;
        return;
    }
    else if (sendsize == -1)
    {
        //�ⲻ̫���ܣ����Է�������ʱ֪ͨ�ҷ������ݣ��ҷ���ʱ��ȴ֪ͨ�ҷ��ͻ���������
        globallogger->clog(LogLevel::ERROR, "CSocket::write_request_handler()ʱif(sendsize == -1)��������ܹ��졣"); //��ӡ����־������Ȳ���ɶ
        return;
    }

    if (sendsize > 0 && sendsize == pConn->isendlen) //�ɹ�������ϣ�����֪ͨ�ǿ��Եģ�
    {
        //����ǳɹ��ķ���������ݣ����д�¼�֪ͨ��epoll�иɵ��ɣ�����������Ǿ��Ƕ����ˣ�����ϵͳ�ں˰����ӴӺ�����иɵ����ɣ�
        if (epoll_oper_event(
            pConn->fd,          //socket���
            EPOLL_CTL_MOD,      //�¼����ͣ��������޸ġ���Ϊ����׼����ȥд֪ͨ��
            EPOLLOUT,           //��־���������Ҫ��ȥ�ı�־,EPOLLOUT����д����д��ʱ��֪ͨ�ҡ�
            1,                  //�����¼�����Ϊ���ӵģ�EPOLL_CTL_MOD��Ҫ�������, 0������   1��ȥ�� 2����ȫ����
            pConn               //���ӳ��е�����
        ) == -1)
        {
            //���������������ɱȽ��鷳��������do nothing
            globallogger->clog(LogLevel::ERROR, "CSocket::write_request_handler()��epoll_oper_event()ʧ�ܡ�");
        }

        //log_stderr(0,"CSocket::write_request_handler()�����ݷ�����ϣ��ܺá�"); //������ʾ�ɣ�����ʱ���Ըɵ�

    }

    //���������ģ�Ҫô���ݷ�������ˣ�Ҫô�Զ˶Ͽ��ˣ���ôִ����β�����ɣ�

    /* 2019.4.2ע�͵���������˳�򣬸о����˳��̫��
    //���ݷ�����ϣ����߰���Ҫ���͵����ݸɵ�����˵�����ͻ����������еط��ˣ��÷����߳��������ж��ܷ���������
    if(sem_post(&m_semEventSendQueue)==-1)
        log_stderr(0,"CSocket::write_request_handler()��sem_post(&m_semEventSendQueue)ʧ��.");


    p_memory->FreeMemory(pConn->psendMemPointer);  //�ͷ��ڴ�
    pConn->psendMemPointer = NULL;
    --pConn->iThrowsendCount;  //����������ִ��
    */
    //2019.4.2��������˳��
    p_memory->FreeMemory(pConn->psendMemPointer);  //�ͷ��ڴ�
    pConn->psendMemPointer = NULL;
    --pConn->iThrowsendCount;//���ֵ�ָ��ˣ���������һ�е��ź�����������
    if (sem_post(&m_semEventSendQueue) == -1)
        globallogger->clog(LogLevel::ERROR, "CSocket::write_request_handler()��sem_post(&m_semEventSendQueue)ʧ��.");

    return;
}

/**
 * @brief ������յ���TCP��Ϣ
 * @details �ú���ר�����ڴ�����յ���TCP��Ϣ����Ϣ��ʽ�Խ��ͣ�ͨ����ͷ���Լ����������ĳ��ȡ��ú���Ŀǰ��һ��ռλ�����������Ϣ�����߼�δʵ�֡�
 *
 * @param pMsgBuf ���յ�����Ϣ������
 */
void CSocket::threadRecvProcFunc(char* pMsgBuf)
{
    return;
}


