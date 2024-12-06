#pragma once

#include <vector>       //vector
#include <list>         //list
#include <sys/epoll.h>  //epoll
#include <sys/socket.h>
#include <pthread.h>    //���߳�
#include <semaphore.h>  //�ź��� 
#include <atomic>       //c++11���ԭ�Ӳ���
#include <map>          //multimap
#include<mutex>
#include<memory>
#include<thread>

#include"comm.h"

#define LISTEN_BACKLOG 511  //��������ӵĶ���
#define MAX_EVENTS     512  //epoll_waitһ����������ô����¼�

typedef struct listening_s   listening_t, * lplistening_t;
typedef struct connection_s  connection_t, * lpconnection_t;
typedef class  CSocket           CSocket;

typedef void (CSocket::* event_handler_pt)(lpconnection_t c); //�����Ա����ָ��

//--------------------------------------------
//һЩר�ýṹ������������ʱ�����Ƿ�global.h����

/**
 * @struct listening_s
 * @brief �����˿���ص���Ϣ
 *
 * �ýṹ�����ڱ�������׽��������Ϣ�������˿ںš��׽��־�������ӳ��е�����ָ�롣
 */
struct listening_s 
{
	int                       port;        //�����Ķ˿ں�
	int                       fd;          //�׽��־��socket
	lpconnection_t        connection;  //���ӳ��е�һ�����ӣ�ע�����Ǹ�ָ�� 
};


//���������ṹ�Ƿǳ���Ҫ�������ṹ��������ӹٷ�nginx��д����
/**
 * @struct connection_s
 * @brief ����һ���ͻ������������TCP����
 *
 * �ýṹ����������һ��TCP���ӵ������Ϣ���������ӵ�״̬�����ݻ��������¼�����ȡ�
 */
struct connection_s
{
	connection_s();                                      //���캯��
	virtual ~connection_s();                             //��������
	void GetOneToUse();                                      //�����ȥ��ʱ���ʼ��һЩ����
	void PutOneToFree();                                     //���ջ�����ʱ����һЩ����


	int                       fd;                            //�׽��־��socket
	lplistening_t         listening;                     //���������ӱ��������һ�������׽��֣���ô�����߾�ָ������׽��ֶ�Ӧ���Ǹ�lpngx_listening_t���ڴ��׵�ַ		

	//------------------------------------	
	//unsigned                  instance:1;                    //��λ��ʧЧ��־λ��0����Ч��1��ʧЧ������ǹٷ�nginx�ṩ��������ʲô�ã�ngx_epoll_process_events()����⡿  
	uint64_t                  iCurrsequence;                 //�������һ����ţ�ÿ�η����ȥʱ+1���˷�Ҳ�п�����һ���̶��ϼ�����ϰ���������ô�ã��õ�����˵
	struct sockaddr           s_sockaddr;                    //����Է���ַ��Ϣ�õ�
	//char                      addr_text[100]; //��ַ���ı���Ϣ��100�㹻��һ����ʵ�����ipv4��ַ��255.255.255.255����ʵֻ��Ҫ20�ֽھ͹�

	//�Ͷ��йصı�־-----------------------
	//uint8_t                   r_ready;        //��׼���ñ�ǡ���ʱû�����׹ٷ�Ҫ��ô�ã�������ע�͵���
	//uint8_t                   w_ready;        //д׼���ñ��

	event_handler_pt      rhandler;                       //���¼�����ش�����
	event_handler_pt      whandler;                       //д�¼�����ش�����

	//��epoll�¼��й�
	uint32_t                  events;                         //��epoll�¼��й�  

	//���հ��й�
	unsigned char             curStat;                        //��ǰ�հ���״̬
	char                      dataHeadInfo[_DATA_BUFSIZE_];   //���ڱ����յ������ݵİ�ͷ��Ϣ			
	char* precvbuf;                      //�������ݵĻ�������ͷָ�룬���յ���ȫ�İ��ǳ����ã�������Ӧ�õĴ���
	unsigned int              irecvlen;                       //Ҫ�յ��������ݣ����������ָ������precvbuf����ʹ�ã�������Ӧ�õĴ���
	char* precvMemPointer;               //new�����������հ����ڴ��׵�ַ���ͷ��õ�

	std::mutex          logicPorcMutex;                 //�߼�������صĻ�����      

	//�ͷ����й�
	std::atomic<int>          iThrowsendCount;                //������Ϣ��������ͻ��������ˣ�����Ҫͨ��epoll�¼���������Ϣ�ļ������ͣ�����������ͻ�����������������������
	char* psendMemPointer;               //������ɺ��ͷ��õģ��������ݵ�ͷָ�룬��ʵ�� ��Ϣͷ + ��ͷ + ����
	char* psendbuf;                      //�������ݵĻ�������ͷָ�룬��ʼ ��ʵ�ǰ�ͷ+����
	unsigned int              isendlen;                       //Ҫ���Ͷ�������

	//�ͻ����й�
	time_t                    inRecyTime;                     //�뵽��Դ����վ��ȥ��ʱ��

	//���������й�
	time_t                    lastPingTime;                   //�ϴ�ping��ʱ�䡾�ϴη������������¼���

	//�����簲ȫ�й�	
	uint64_t                  FloodkickLastTime;              //Flood�����ϴ��յ�����ʱ��
	int                       FloodAttackCount;               //Flood�����ڸ�ʱ�����յ����Ĵ���ͳ��
	std::atomic<int>          iSendCount;                     //���Ͷ������е�������Ŀ������clientֻ�����գ��������ɴ����������ݴ��������߳����� 


	//--------------------------------------------------
	lpconnection_t        next;                           //���Ǹ�ָ�룬ָ����һ�������Ͷ������ڰѿ��е����ӳض�����������һ��������������ȡ��
};

/**
 * @struct _STRUC_MSG_HEADER
 * @brief ��Ϣͷ�ṹ��
 *
 * �ýṹ�����ڱ�����Ϣ��ͷ����Ϣ������������Ӧ�����ӡ�
 */
typedef struct _STRUC_MSG_HEADER
{
	lpconnection_t pConn;         //��¼��Ӧ�����ӣ�ע�����Ǹ�ָ��
	uint64_t           iCurrsequence; //�յ����ݰ�ʱ��¼��Ӧ���ӵ���ţ����������ڱȽ��Ƿ������Ѿ�������
	//......�����Ժ���չ	
}STRUC_MSG_HEADER, * LPSTRUC_MSG_HEADER;

/**
 * @class CSocket
 * @brief ���ڴ����׽������Ӻ������¼�����
 *
 * ���ฺ���ʼ���͹�����ͻ��˵����ӣ��������պͷ������ݣ����������������� epoll �¼��ȡ�
 * ���������ӳع�����Ϣ���д������簲ȫ��ʩ���� Flood ������⣩�ȹ��ܡ�
 */
class CSocket
{
public:
    CSocket();                 ///< ���캯��
    virtual ~CSocket();        ///< ��������
    virtual bool Initialize(); ///< ��ʼ������
    virtual bool Initialize_subproc(); ///< �ӽ��̳�ʼ��
    virtual void Shutdown_subproc(); ///< �ӽ�����Դ����

    void printTDInfo(); ///< ��ӡ�߳�����

    virtual void threadRecvProcFunc(char* pMsgBuf); ///< ����ͻ���������麯��
    virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time); ///< ��������ʱ���

    int epoll_init(); ///< ��ʼ�� epoll ����
    //int epoll_add_event(int fd, int readevent, int writevent, uint32_t otherflag, uint32_t eventtype, std::shared_ptr<connection_t> c); ///< ��� epoll �¼�
    int epoll_process_events(int timer); ///< ���� epoll �¼�
    int epoll_oper_event(int fd, uint32_t eventtype, uint32_t flag, int bcaction, lpconnection_t pConn);///<epoll�����¼�

protected:
    void msgSend(char* psendbuf); ///< ��������
    void zdClosesocketProc(lpconnection_t p_Conn); ///< �ر�����

private:
    void ReadConf(); ///< ��ȡ����
    bool open_listening_sockets(); ///< �򿪼����׽���
    void close_listening_sockets(); ///< �رռ����׽���
    bool setnonblocking(int sockfd); ///< ���÷�����ģʽ

    //һЩҵ������handler
    void event_accept(lpconnection_t oldc);                       //����������
    void read_request_handler(lpconnection_t pConn);              //����������ʱ�Ķ�������
    void write_request_handler(lpconnection_t pConn);             //�������ݷ���ʱ��д������
    void close_connection(lpconnection_t pConn);                  //ͨ�����ӹرպ�������Դ����������ͷš���Ϊ�����漰���ü���Ҫ�ͷŵ���Դ������д�ɺ�����

    ssize_t recvproc(lpconnection_t pConn, char* buff, ssize_t buflen); //���մӿͻ�����������ר�ú���
    void wait_request_handler_proc_p1(lpconnection_t pConn, bool& isflood);
    //��ͷ��������Ĵ������ǳ�Ϊ������׶�1��д�ɺ��������㸴��      
    void wait_request_handler_proc_plast(lpconnection_t pConn, bool& isflood);
    //�յ�һ����������Ĵ����ŵ�һ�������У��������	
    void clearMsgSendQueue();                                             //��������Ϣ����  

    ssize_t sendproc(lpconnection_t c, char* buff, ssize_t size);       //�����ݷ��͵��ͻ��� 

    //��ȡ�Զ���Ϣ���                                              
    size_t sock_ntop(struct sockaddr* sa, int port, u_char* text, size_t len);  //���ݲ���1��������Ϣ����ȡ��ַ�˿��ַ�������������ַ����ĳ���


    void initconnection(); ///< ��ʼ�����ӳ�
    void clearconnection(); ///< �������ӳ�
    lpconnection_t get_connection(int isock); ///< ��ȡ����
    void free_connection(lpconnection_t pConn); ///< �黹����
    void inRecyConnectQueue(lpconnection_t pConn);              ///<��Ҫ���յ����ӷŵ�һ����������

    void AddToTimerQueue(lpconnection_t pConn); ///< ��ӵ�ʱ�����
    time_t GetEarliestTime(); ///< ��ȡ�����ʱ��
    LPSTRUC_MSG_HEADER RemoveFirstTimer(); ///< �Ƴ�����Ķ�ʱ��
    LPSTRUC_MSG_HEADER GetOverTimeTimer(time_t cur_time); ///< ��ȡ��ʱ�Ķ�ʱ��
    void DeleteFromTimerQueue(lpconnection_t pConn); ///< ɾ����ʱ��
    void clearAllFromTimerQueue(); ///< �������ж�ʱ��

    bool TestFlood(lpconnection_t pConn); ///< �����Ƿ�Ϊ Flood ����

    static void* ServerSendQueueThread(void* threadData); ///< ������Ϣ�߳�
    static void* ServerRecyConnectionThread(void* threadData); ///< ���������߳�
    static void* ServerTimerQueueMonitorThread(void* threadData); ///< ʱ����м���߳�

protected:
    size_t m_iLenPkgHeader; ///< ���ݰ�ͷ����
    size_t m_iLenMsgHeader; ///< ��Ϣͷ����

    int m_ifTimeOutKick; ///< �Ƿ�����ʱ�߳�����
    int m_iWaitTime; ///< �ȴ�ʱ��

private:
    struct ThreadItem
    {
        std::thread _Handle; ///< �߳̾��
        CSocket* _pThis; ///< ָ�� CSocket ��ָ��
        bool ifrunning; ///< �߳��Ƿ�����

        ThreadItem(CSocket* pthis) : _pThis(pthis), ifrunning(false) {}
    };

    int m_worker_connections; ///< ���������
    int m_ListenPortCount; ///< �����˿�����
    int m_epollhandle; ///< epoll ���

    std::list<lpconnection_t> m_connectionList; ///< ���ӳ�
    std::list<lpconnection_t> m_freeconnectionList; ///< �������ӳ�
    std::atomic<int> m_total_connection_n; ///< ���ӳ���������
    std::atomic<int> m_free_connection_n; ///< ����������
    std::mutex m_connectionMutex; ///< ������ػ�����
    std::mutex m_recyconnqueueMutex; ///< ���ڱ������ӻ��ն��еĻ�����
    std::list<lpconnection_t> m_recyconnectionList; ///< �洢���ͷŵ�����
    std::atomic<int> m_totol_recyconnection_n; ///< ���������ӵ�����
    int m_RecyConnectionWaitTime; ///< �������ӵĵȴ�ʱ�䣬��λ����

    
    std::vector<std::shared_ptr<listening_t>> m_ListenSocketList;  ///<�����׽����б�
    struct epoll_event m_events[MAX_EVENTS]; ///< epoll �¼��б�

    std::list<char*> m_MsgSendQueue; ///< ������Ϣ����
    std::atomic<int> m_iSendMsgQueueCount; ///< ��Ϣ���д�С

    std::vector<std::shared_ptr<ThreadItem>> m_threadVector; ///< �̳߳�
    std::mutex m_sendMessageQueueMutex; ///< ���Ͷ��л�����
    sem_t m_semEventSendQueue; ///< ���Ͷ����ź���

    int m_ifkickTimeCount; ///< �Ƿ�������ʱ��
    std::mutex m_timequeueMutex; ///< ʱ����л�����
    std::multimap<time_t, LPSTRUC_MSG_HEADER> m_timerQueuemap; ///< ʱ�����
    size_t m_cur_size_; ///< ʱ����еĳߴ�
    time_t m_timer_value_; ///< ��ǰ��ʱ����ͷ��ʱ��ֵ

    std::atomic<int> m_onlineUserCount; ///< �����û���

    int m_floodAkEnable; ///< Flood ��������Ƿ�����
    unsigned int m_floodTimeInterval; ///< Flood �������ʱ����
    int m_floodKickCount; ///< Flood �����߳�����

    time_t m_lastprintTime; ///< �ϴδ�ӡͳ����Ϣ��ʱ��
    int m_iDiscardSendPkgCount; ///< �����ķ������ݰ�����
};

