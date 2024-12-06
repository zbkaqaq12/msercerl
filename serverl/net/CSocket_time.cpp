#include"global.h"
#include"CSocket.h"
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include <pthread.h>   //���߳�
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
#include "CMemory.h"

//�����߳�ʱ��(��multimap������������)���û��������ֳɹ����룬Ȼ�����ǿ��������˿��ء�Sock_WaitTimeEnable = 1������ô�����������ã�
/**
 * @brief ��������ӵ���ʱ��������
 * @details ���û��ɹ����������ҿ��������˿���ʱ���˺��������Ӷ�����ӵ���ʱ�������У�����һ��δ����ʱ�䣬����ָ����ʱ�䵽��ʱ����Ƿ���Ҫ�߳������ӡ�
 *
 * @param pConn ��Ҫ��ӵ������е����Ӷ���
 */
void CSocket::AddToTimerQueue(lpconnection_t pConn)
{
	CMemory* p_memory = CMemory::GetInstance();

	time_t futtime = time(NULL);
	futtime += m_iWaitTime;  //20��֮���ʱ��

	//CLock lock(&m_timequeueMutex); //���⣬��ΪҪ����m_timeQueuemap��
	std::lock_guard<std::mutex> lock(m_timequeueMutex);
	LPSTRUC_MSG_HEADER tmpMsgHeader = (LPSTRUC_MSG_HEADER)p_memory->AllocMemory(m_iLenMsgHeader, false);
	tmpMsgHeader->pConn = pConn;
	tmpMsgHeader->iCurrsequence = pConn->iCurrsequence;
	m_timerQueuemap.insert(std::make_pair(futtime, tmpMsgHeader)); //���� �Զ����� С->��
	m_cur_size_++;  //��ʱ���гߴ�+1
	m_timer_value_ = GetEarliestTime(); //��ʱ����ͷ��ʱ��ֵ���浽m_timer_value_��
	return;
}

//��multimap��ȡ�������ʱ�䷵��ȥ�������߸��𻥳⣬���Ա��������û��⣬������ȷ��m_timeQueuemap��һ����Ϊ��
/**
 * @brief ��ȡ��ʱ�������е�����ʱ��
 * @details �Ӷ�ʱ�������л�ȡ�����ʱ��ֵ�������ȵ��ڵ�ʱ�䣩��
 * 
 * @return time_t ���ض����������ʱ��ֵ
 */
time_t CSocket::GetEarliestTime()
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;
	pos = m_timerQueuemap.begin();
	return pos->first;
}

//��m_timeQueuemap�Ƴ������ʱ�䣬�����������ʱ�����ڵ����ֵ����Ӧ��ָ�� ���أ������߸��𻥳⣬���Ա��������û��⣬
/**
 * @brief �Ƴ���ʱ������������Ľڵ�
 * @details �Ӷ�ʱ���������Ƴ����絽�ڵĽڵ㣬�����ظýڵ�����ݡ���������Ҫȷ�����ô˺���ʱ���в�Ϊ�ա�
 *
 * @return LPSTRUC_MSG_HEADER ���ر��Ƴ��Ľڵ��ָ�룬�������Ϊ���򷵻�NULL
 */
LPSTRUC_MSG_HEADER CSocket::RemoveFirstTimer()
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;
	LPSTRUC_MSG_HEADER p_tmp;
	if (m_cur_size_ <= 0)
	{
		return NULL;
	}
	pos = m_timerQueuemap.begin(); //�����߸��𻥳�ģ�����ֱ�Ӳ���û�����
	p_tmp = pos->second;
	m_timerQueuemap.erase(pos);
	--m_cur_size_;
	return p_tmp;
}

//���ݸ��ĵ�ǰʱ�䣬��m_timeQueuemap�ҵ������ʱ����ϣ����磩�Ľڵ㡾1��������ȥ����Щ�ڵ㶼��ʱ�䳬���ˣ�Ҫ����Ľڵ�
//�����߸��𻥳⣬���Ա��������û���
/**
 * @brief ��ȡ��ʱ�Ķ�ʱ���ڵ�
 * @details ���ݵ�ǰʱ��Ӷ�ʱ�������л�ȡ���г�ʱ�Ľڵ㡣��ʱ�Ľڵ�ᱻ�Ƴ������أ���������Ҫ�������ĳ�ʱ�߼���
 *
 * @param cur_time ��ǰʱ��
 * @return LPSTRUC_MSG_HEADER ���س�ʱ�Ķ�ʱ���ڵ�ָ�룬���û�г�ʱ�ڵ��򷵻�NULL
 */
LPSTRUC_MSG_HEADER CSocket::GetOverTimeTimer(time_t cur_time)
{
	CMemory* p_memory = CMemory::GetInstance();
	LPSTRUC_MSG_HEADER ptmp;

	if (m_cur_size_ == 0 || m_timerQueuemap.empty())
		return NULL; //����Ϊ��

	time_t earliesttime = GetEarliestTime(); //��multimap��ȥ��ѯ
	if (earliesttime <= cur_time)
	{
		//���ȷʵ���е�ʱ����ˡ���ʱ�Ľڵ㡿
		ptmp = RemoveFirstTimer();    //�������ʱ�Ľڵ�� m_timerQueuemap ɾ������������ڵ�ĵڶ��������

		if (/*m_ifkickTimeCount == 1 && */m_ifTimeOutKick != 1)  //�ܵ��õ���������һ�������϶����������Ե�һ�������Ӳ�������ν����Ҫ�ǵڶ�������
		{
			//�������Ҫ��ʱ������������������£�

			//��Ϊ�´γ�ʱ��ʱ������Ҳ��ȻҪ�жϣ����Ի�Ҫ������ڵ�ӻ���        
			time_t newinqueutime = cur_time + (m_iWaitTime);
			LPSTRUC_MSG_HEADER tmpMsgHeader = (LPSTRUC_MSG_HEADER)p_memory->AllocMemory(sizeof(STRUC_MSG_HEADER), false);
			tmpMsgHeader->pConn = ptmp->pConn;
			tmpMsgHeader->iCurrsequence = ptmp->iCurrsequence;
			m_timerQueuemap.insert(std::make_pair(newinqueutime, tmpMsgHeader)); //�Զ����� С->��			
			m_cur_size_++;
		}

		if (m_cur_size_ > 0) //����ж�������Ҫ����Ϊ�Ժ����ǿ��������������Ĵ���
		{
			m_timer_value_ = GetEarliestTime(); //��ʱ����ͷ��ʱ��ֵ���浽m_timer_value_��
		}
		return ptmp;
	}
	return NULL;
}

/**
 * @brief �Ӷ�ʱ���������Ƴ�ָ������
 * @details ���ݴ�������Ӷ��󣬴Ӷ�ʱ�������в��Ҳ��Ƴ������ӵĶ�ʱ���ڵ㣬�ͷ�����ڴ档
 *
 * @param pConn ��Ҫ�Ӷ�ʱ���������Ƴ������Ӷ���
 */
void CSocket::DeleteFromTimerQueue(lpconnection_t pConn)
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos, posend;
	CMemory* p_memory = CMemory::GetInstance();

	std::lock_guard<std::mutex> lock(m_timequeueMutex);

	//��Ϊʵ��������ܱȽϸ��ӣ��������ܻ��������ȵȣ������������Ǳ������������� һȦ���������ҵ�һ�ξ��������������ʲô��©
lblMTQM:
	pos = m_timerQueuemap.begin();
	posend = m_timerQueuemap.end();
	for (; pos != posend; ++pos)
	{
		if (pos->second->pConn == pConn)
		{
			p_memory->FreeMemory(pos->second);  //�ͷ��ڴ�
			m_timerQueuemap.erase(pos);
			--m_cur_size_; //��ȥһ��Ԫ�أ���ȻҪ�ѳߴ����1��;								
			goto lblMTQM;
		}
	}
	if (m_cur_size_ > 0)
	{
		m_timer_value_ = GetEarliestTime();
	}
	return;
}

/**
 * @brief ��ն�ʱ������
 * @details ��ն�ʱ�������е����нڵ㣬�ͷ�������ص��ڴ档
 */
void CSocket::clearAllFromTimerQueue()
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos, posend;

	CMemory* p_memory = CMemory::GetInstance();
	pos = m_timerQueuemap.begin();
	posend = m_timerQueuemap.end();
	for (; pos != posend; ++pos)
	{
		p_memory->FreeMemory(pos->second);
		--m_cur_size_;
	}
	m_timerQueuemap.clear();
}

/**
 * @brief ��ʱ�����м���߳�
 * @details ���̶߳��ڼ�鶨ʱ�����У������ѳ�ʱ�����ӡ���ʱ�����ӻᱻ��Ⲣ���������߳�����
 *
 * @param threadData �̴߳�������ݣ�����ָ��`CSocket`�����ָ�룩
 * @return void* ����ֵΪ0����ʾ�߳�ִ�����
 */
void* CSocket::ServerTimerQueueMonitorThread(void* threadData)
{
	ThreadItem* pThread = static_cast<ThreadItem*>(threadData);
    CSocket* pSocketObj = pThread->_pThis;

    time_t absolute_time, cur_time;
    int err;

    while (g_stopEvent == 0) // �߳��˳��ı�־
    {
        if (pSocketObj->m_cur_size_ > 0) // ���в�Ϊ��
        {
            absolute_time = pSocketObj->m_timer_value_; // ���һ���¼���ʱ��
            cur_time = time(NULL);
            if (absolute_time < cur_time) // ���ʱ�䵽��
            {
                std::list<LPSTRUC_MSG_HEADER> m_lsIdleList; // ����Ҫ���������
                LPSTRUC_MSG_HEADER result;

                // ��������ȡ���г�ʱ�Ľڵ�
				{
					std::lock_guard<std::mutex> lock(pSocketObj->m_timequeueMutex);
					// ��ȡ���г�ʱ�Ľڵ�
					
					while ((result = pSocketObj->GetOverTimeTimer(cur_time)) != NULL)
					{
						m_lsIdleList.push_back(result);
					}
				}
                // ����ʱ������
                LPSTRUC_MSG_HEADER tmpmsg;
                while (!m_lsIdleList.empty())
                {
                    tmpmsg = m_lsIdleList.front();
                    m_lsIdleList.pop_front();
                    pSocketObj->procPingTimeOutChecking(tmpmsg, cur_time); // ������ʱ���
                }
            }
        }

        usleep(500 * 1000); // ���� 500 ����
    }

	return (void*)0;
}

//���������ʱ�䵽����ȥ����������Ƿ�ʱ�����ˣ�������ֻ�ǰ��ڴ��ͷţ�����Ӧ���������ȸú�����ʵ�־�����ж϶���
void CSocket::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time)
{
	CMemory* p_memory = CMemory::GetInstance();
	p_memory->FreeMemory(tmpmsg);
}

