#include"global.h"
#include"CSocket.h"
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include <pthread.h>   //多线程
#include "CMemory.h"

/**
 * @brief 处理接收到的数据包
 * @details 该函数用于处理连接中接收到的数据。当连接上有数据到达时，`epoll_process_events()` 会调用此函数进行数据的解析和处理。
 *          此函数会根据连接状态（如接收包头、接收包体）不同，执行不同的处理逻辑。
 *          如果启用了Flood攻击检测，则会检查是否存在Flood攻击。
 *
 * @param pConn 连接对象，包含当前连接的数据接收缓冲区和接收状态等信息
 */
void CSocket::read_request_handler(lpconnection_t pConn)
{
    bool isflood = false; //是否flood攻击；

    //收包，注意我们用的第二个和第三个参数，我们用的始终是这两个参数，因此我们必须保证 c->precvbuf指向正确的收包位置，保证c->irecvlen指向正确的收包宽度
    ssize_t reco = recvproc(pConn, pConn->precvbuf, pConn->irecvlen);
    if (reco <= 0)
    {
        return;//该处理的上边这个recvproc()函数处理过了，这里<=0是直接return        
    }

    //走到这里，说明成功收到了一些字节（>0），就要开始判断收到了多少数据了     
    if (pConn->curStat == _PKG_HD_INIT) //连接建立起来时肯定是这个状态，因为在get_connection()中已经把curStat成员赋值成_PKG_HD_INIT了
    {
        if (reco == m_iLenPkgHeader)//正好收到完整包头，这里拆解包头
        {
            wait_request_handler_proc_p1(pConn, isflood); //那就调用专门针对包头处理完整的函数去处理把。
        }
        else
        {
            //收到的包头不完整--我们不能预料每个包的长度，也不能预料各种拆包/粘包情况，所以收到不完整包头【也算是缺包】是很可能的；
            pConn->curStat = _PKG_HD_RECVING;                 //接收包头中，包头不完整，继续接收包头中	
            pConn->precvbuf = pConn->precvbuf + reco;              //注意收后续包的内存往后走
            pConn->irecvlen = pConn->irecvlen - reco;              //要收的内容当然要减少，以确保只收到完整的包头先
        } //end  if(reco == m_iLenPkgHeader)
    }
    else if (pConn->curStat == _PKG_HD_RECVING) //接收包头中，包头不完整，继续接收中，这个条件才会成立
    {
        if (pConn->irecvlen == reco) //要求收到的宽度和我实际收到的宽度相等
        {
            //包头收完整了
            wait_request_handler_proc_p1(pConn, isflood); //那就调用专门针对包头处理完整的函数去处理把。
        }
        else
        {
            //包头还是没收完整，继续收包头
            //pConn->curStat        = _PKG_HD_RECVING;                 //没必要
            pConn->precvbuf = pConn->precvbuf + reco;              //注意收后续包的内存往后走
            pConn->irecvlen = pConn->irecvlen - reco;              //要收的内容当然要减少，以确保只收到完整的包头先
        }
    }
    else if (pConn->curStat == _PKG_BD_INIT)
    {
        //包头刚好收完，准备接收包体
        if (reco == pConn->irecvlen)
        {
            //收到的宽度等于要收的宽度，包体也收完整了
            if (m_floodAkEnable == 1)
            {
                //Flood攻击检测是否开启
                isflood = TestFlood(pConn);
            }
            wait_request_handler_proc_plast(pConn, isflood);
        }
        else
        {
            //收到的宽度小于要收的宽度
            pConn->curStat = _PKG_BD_RECVING;
            pConn->precvbuf = pConn->precvbuf + reco;
            pConn->irecvlen = pConn->irecvlen - reco;
        }
    }
    else if (pConn->curStat == _PKG_BD_RECVING)
    {
        //接收包体中，包体不完整，继续接收中
        if (pConn->irecvlen == reco)
        {
            //包体收完整了
            if (m_floodAkEnable == 1)
            {
                //Flood攻击检测是否开启
                isflood = TestFlood(pConn);
            }
            wait_request_handler_proc_plast(pConn, isflood);
        }
        else
        {
            //包体没收完整，继续收
            pConn->precvbuf = pConn->precvbuf + reco;
            pConn->irecvlen = pConn->irecvlen - reco;
        }
    }  //end if(c->curStat == _PKG_HD_INIT)

    if (isflood == true)
    {
        //客户端flood服务器，则直接把客户端踢掉
        //log_stderr(errno,"发现客户端flood，干掉该客户端!");
        zdClosesocketProc(pConn);
    }

    return;
}


/**
 * @brief 接收数据并处理接收错误
 * @details 该函数用于从客户端接收数据并进行错误处理。如果接收过程中发生错误（如连接关闭、系统错误等），会自动关闭连接并释放资源。如果接收成功，则返回实际接收到的字节数。
 *
 * @param c 连接对象，包含连接的文件描述符和其他信息
 * @param buff 存储接收数据的缓冲区
 * @param buflen 缓冲区大小，表示最多接收的数据量
 * @return 返回接收到的字节数；如果发生错误，则返回-1并关闭连接
 */
ssize_t CSocket::recvproc(lpconnection_t c, char* buff, ssize_t buflen)
{
	ssize_t n;

	n = recv(c->fd, buff, buflen, 0); //recv()系统函数， 最后一个参数flag，一般为0；     
	if (n == 0)
	{
		//客户端关闭【应该是正常完成了4次挥手】，我这边就直接回收连接连接，关闭socket即可 
		//log_stderr(0,"连接被客户端正常关闭[4路挥手关闭]！");
		close_connection(c);
		return -1;
	}
	//客户端没断，走这里 
	if (n < 0) //这被认为有错误发生
	{
		//EAGAIN和EWOULDBLOCK[【这个应该常用在hp上】应该是一样的值，表示没收到数据，一般来讲，在ET模式下会出现这个错误，因为ET模式下是不停的recv肯定有一个时刻收到这个errno，但LT模式下一般是来事件才收，所以不该出现这个返回值
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			//我认为LT模式不该出现这个errno，而且这个其实也不是错误，所以不当做错误处理
			globallogger->flog(LogLevel::ERROR, "CSocket::recvproc()中errno == EAGAIN || errno == EWOULDBLOCK成立，出乎我意料！");//epoll为LT模式不应该出现这个返回值，所以直接打印出来瞧瞧
			return -1; //不当做错误处理，只是简单返回
		}
		//EINTR错误的产生：当阻塞于某个慢系统调用的一个进程捕获某个信号且相应信号处理函数返回时，该系统调用可能返回一个EINTR错误。
		//例如：在socket服务器端，设置了信号捕获机制，有子进程，当在父进程阻塞于慢系统调用时由父进程捕获到了一个有效信号时，内核会致使accept返回一个EINTR错误(被中断的系统调用)。
		if (errno == EINTR)  //这个不算错误，是我参考官方nginx，官方nginx这个就不算错误；
		{
			//我认为LT模式不该出现这个errno，而且这个其实也不是错误，所以不当做错误处理
			globallogger->flog(LogLevel::ERROR, "CSocket::recvproc()中errno == EINTR成立，出乎我意料！");//epoll为LT模式不应该出现这个返回值，所以直接打印出来瞧瞧
			return -1; //不当做错误处理，只是简单返回
		}

		//所有从这里走下来的错误，都认为异常：意味着我们要关闭客户端套接字要回收连接池中连接；

		//errno参考：http://dhfapiran1.360drm.com        
		if (errno == ECONNRESET)  //#define ECONNRESET 104 /* Connection reset by peer */
		{
			//如果客户端没有正常关闭socket连接，却关闭了整个运行程序【真是够粗暴无理的，应该是直接给服务器发送rst包而不是4次挥手包完成连接断开】，那么会产生这个错误            
			//10054(WSAECONNRESET)--远程程序正在连接的时候关闭会产生这个错误--远程主机强迫关闭了一个现有的连接
			//算常规错误吧【普通信息型】，日志都不用打印，没啥意思，太普通的错误
			//do nothing

			//....一些大家遇到的很普通的错误信息，也可以往这里增加各种，代码要慢慢完善，一步到位，不可能，很多服务器程序经过很多年的完善才比较圆满；
		}
		else
		{
			//能走到这里的，都表示错误，我打印一下日志，希望知道一下是啥错误，我准备打印到屏幕上
			globallogger->flog(LogLevel::ERROR, "CSocket::recvproc()中发生错误，我打印出来看看是啥错误！");  //正式运营时可以考虑这些日志打印去掉
		}

		//log_stderr(0,"连接被客户端 非 正常关闭！");

		//这种真正的错误就要，直接关闭套接字，释放连接池中连接了
		close_connection(c);
		return -1;
	}

	//能走到这里的，就认为收到了有效数据
	return n; //返回收到的字节数
}


/**
 * @brief 包头收完整后的处理，开始处理包体
 * @details 在接收到完整的包头后，会判断包的合法性，并根据包的长度分配内存进行包体接收。如果包头有效，则继续接收包体；如果包头不合法，则回收连接并清理状态。
 *
 * @param pConn 当前连接
 * @param isflood 引用参数，用于Flood攻击检测
 */
void CSocket::wait_request_handler_proc_p1(lpconnection_t pConn, bool& isflood)
{
    CMemory* p_memory = CMemory::GetInstance();

    LPCOMM_PKG_HEADER pPkgHeader;
    pPkgHeader = (LPCOMM_PKG_HEADER)pConn->dataHeadInfo; //正好收到包头时，包头信息肯定是在dataHeadInfo里；

    unsigned short e_pkgLen;
    e_pkgLen = ntohs(pPkgHeader->pkgLen);  //注意这里网络序转本机序，所有传输到网络上的2字节数据，都要用htons()转成网络序，所有从网络上收到的2字节数据，都要用ntohs()转成本机序
    //ntohs/htons的目的就是保证不同操作系统数据之间收发的正确性，【不管客户端/服务器是什么操作系统，发送的数字是多少，收到的就是多少】
    //不明白的同学，直接百度搜索"网络字节序" "主机字节序" "c++ 大端" "c++ 小端"
//恶意包或者错误包的判断
    if (e_pkgLen < m_iLenPkgHeader)
    {
        //伪造包/或者包错误，否则整个包长怎么可能比包头还小？（整个包长是包头+包体，就算包体为0字节，那么至少e_pkgLen == m_iLenPkgHeader）
        //报文总长度 < 包头长度，认定非法用户，废包
        //状态和接收位置都复原，这些值都有必要，因为有可能在其他状态比如_PKG_HD_RECVING状态调用这个函数；
        pConn->curStat = _PKG_HD_INIT;
        pConn->precvbuf = pConn->dataHeadInfo;
        pConn->irecvlen = m_iLenPkgHeader;
    }
    else if (e_pkgLen > (_PKG_MAX_LENGTH - 1000))   //客户端发来包居然说包长度 > 29000?肯定是恶意包
    {
        //恶意包，太大，认定非法用户，废包【包头中说这个包总长度这么大，这不行】
        //状态和接收位置都复原，这些值都有必要，因为有可能在其他状态比如_PKG_HD_RECVING状态调用这个函数；
        pConn->curStat = _PKG_HD_INIT;
        pConn->precvbuf = pConn->dataHeadInfo;
        pConn->irecvlen = m_iLenPkgHeader;
    }
    else
    {
        //合法的包头，继续处理
        //我现在要分配内存开始收包体，因为包体长度并不是固定的，所以内存肯定要new出来；
        char* pTmpBuffer = (char*)p_memory->AllocMemory(m_iLenMsgHeader + e_pkgLen, false); //分配内存【长度是 消息头长度  + 包头长度 + 包体长度】，最后参数先给false，表示内存不需要memset;        
        pConn->precvMemPointer = pTmpBuffer;  //内存开始指针

        //a)先填写消息头内容
        LPSTRUC_MSG_HEADER ptmpMsgHeader = (LPSTRUC_MSG_HEADER)pTmpBuffer;
        ptmpMsgHeader->pConn = pConn;
        ptmpMsgHeader->iCurrsequence = pConn->iCurrsequence; //收到包时的连接池中连接序号记录到消息头里来，以备将来用；
        //b)再填写包头内容
        pTmpBuffer += m_iLenMsgHeader;                 //往后跳，跳过消息头，指向包头
        memcpy(pTmpBuffer, pPkgHeader, m_iLenPkgHeader); //直接把收到的包头拷贝进来
        if (e_pkgLen == m_iLenPkgHeader)
        {
            //该报文只有包头无包体【我们允许一个包只有包头，没有包体】
            //这相当于收完整了，则直接入消息队列待后续业务逻辑线程去处理吧
            if (m_floodAkEnable == 1)
            {
                //Flood攻击检测是否开启
                isflood = TestFlood(pConn);
            }
            wait_request_handler_proc_plast(pConn, isflood);
        }
        else
        {
            //开始收包体，注意我的写法
            pConn->curStat = _PKG_BD_INIT;                   //当前状态发生改变，包头刚好收完，准备接收包体	    
            pConn->precvbuf = pTmpBuffer + m_iLenPkgHeader;  //pTmpBuffer指向包头，这里 + m_iLenPkgHeader后指向包体 weizhi
            pConn->irecvlen = e_pkgLen - m_iLenPkgHeader;    //e_pkgLen是整个包【包头+包体】大小，-m_iLenPkgHeader【包头】  = 包体
        }
    }  //end if(e_pkgLen < m_iLenPkgHeader) 

    return;
}

/**
 * @brief 处理收到的完整包的最后阶段
 * @details 该函数用于在收到完整包后，将包放入消息队列并触发线程池处理。如果检测到Flood攻击，则丢弃恶意包并释放内存。处理完毕后，恢复连接的状态以便接收下一个包。
 *
 * @param pConn 当前连接对象
 * @param isflood 引用参数，用于指示是否检测到Flood攻击
 */
void CSocket::wait_request_handler_proc_plast(lpconnection_t pConn, bool& isflood)
{
    //把这段内存放到消息队列中来；
    //int irmqc = 0;  //消息队列当前信息数量
    //inMsgRecvQueue(c->precvMemPointer,irmqc); //返回消息队列当前信息数量irmqc，是调用本函数后的消息队列中消息数量
    //激发线程池中的某个线程来处理业务逻辑
    //g_threadpool.Call(irmqc);

    if (isflood == false)
    {
        g_threadpool.inMsgRecvQueueAndSignal(pConn->precvMemPointer); //入消息队列并触发线程处理消息
    }
    else
    {
        //对于有攻击倾向的恶人，先把他的包丢掉
        CMemory* p_memory = CMemory::GetInstance();
        p_memory->FreeMemory(pConn->precvMemPointer); //直接释放掉内存，根本不往消息队列入
    }

    pConn->precvMemPointer = NULL;
    pConn->curStat = _PKG_HD_INIT;     //收包状态机的状态恢复为原始态，为收下一个包做准备                    
    pConn->precvbuf = pConn->dataHeadInfo;  //设置好收包的位置
    pConn->irecvlen = m_iLenPkgHeader;  //设置好要接收数据的大小
    return;
}

/**
 * @brief 发送数据并处理发送状态
 * @details 该函数用于向连接发送数据。它处理多种发送结果，包括成功发送、发送缓冲区满和对端断开连接等情况。如果发送缓冲区满，返回-1；如果对端断开，返回0；如果发生其他错误，返回-2。
 *
 * @param c 当前连接对象
 * @param buff 要发送的数据缓冲区
 * @param size 发送数据的大小
 * @return 返回成功发送的字节数，或表示错误的负值：
 *         > 0: 发送成功的字节数
 *         = 0: 对端已关闭连接
 *         -1: 发送缓冲区满（EAGAIN）
 *         -2: 发生其他错误
 */
ssize_t CSocket::sendproc(lpconnection_t c, char* buff, ssize_t size)  //ssize_t是有符号整型，在32位机器上等同与int，在64位机器上等同与long int，size_t就是无符号型的ssize_t
{
    //这里参考借鉴了官方nginx函数unix_send()的写法
    ssize_t   n;

    for (;; )
    {
        n = send(c->fd, buff, size, 0); //send()系统函数， 最后一个参数flag，一般为0； 
        if (n > 0) //成功发送了一些数据
        {
            //发送成功一些数据，但发送了多少，我们这里不关心，也不需要再次send
            //这里有两种情况
            //(1) n == size也就是想发送多少都发送成功了，这表示完全发完毕了
            //(2) n < size 没发送完毕，那肯定是发送缓冲区满了，所以也不必要重试发送，直接返回吧
            return n; //返回本次发送的字节数
        }

        if (n == 0)
        {
            //send()返回0？ 一般recv()返回0表示断开,send()返回0，我这里就直接返回0吧【让调用者处理】；我个人认为send()返回0，要么你发送的字节是0，要么对端可能断开。
            //网上找资料：send=0表示超时，对方主动关闭了连接过程
            //我们写代码要遵循一个原则，连接断开，我们并不在send动作里处理诸如关闭socket这种动作，集中到recv那里处理，否则send,recv都处理都处理连接断开关闭socket则会乱套
            //连接断开epoll会通知并且 recvproc()里会处理，不在这里处理
            return 0;
        }

        if (errno == EAGAIN)  //这东西应该等于EWOULDBLOCK
        {
            //内核缓冲区满，这个不算错误
            return -1;  //表示发送缓冲区满了
        }

        if (errno == EINTR)
        {
            //这个应该也不算错误 ，收到某个信号导致send产生这个错误？
            //参考官方的写法，打印个日志，其他啥也没干，那就是等下次for循环重新send试一次了
            globallogger->clog(LogLevel::ERROR, "CSocket::sendproc()中send()失败.");  //打印个日志看看啥时候出这个错误
            //其他不需要做什么，等下次for循环吧            
        }
        else
        {
            //走到这里表示是其他错误码，都表示错误，错误我也不断开socket，我也依然等待recv()来统一处理断开，因为我是多线程，send()也处理断开，recv()也处理断开，很难处理好
            return -2;
        }
    } //end for
}

/**
 * @brief 处理数据发送请求
 * @details 当数据可写时，由epoll通知此函数。该函数会尝试发送数据，如果发送缓冲区满则返回-1，发送成功则继续处理剩余的数据。数据发送完毕后，会从epoll中移除写事件。
 *          如果发送过程中发生错误，会记录错误日志。如果数据完全发送完毕，则会通知发送线程继续执行。
 *
 * @param pConn 当前连接对象，包含发送数据的缓冲区和数据长度等信息
 */
void CSocket::write_request_handler(lpconnection_t pConn)
{
    CMemory* p_memory = CMemory::GetInstance();

    //这些代码的书写可以参照 void* CSocket::ServerSendQueueThread(void* threadData)
    ssize_t sendsize = sendproc(pConn, pConn->psendbuf, pConn->isendlen);

    if (sendsize > 0 && sendsize != pConn->isendlen)
    {
        //没有全部发送完毕，数据只发出去了一部分，那么发送到了哪里，剩余多少，继续记录，方便下次sendproc()时使用
        pConn->psendbuf = pConn->psendbuf + sendsize;
        pConn->isendlen = pConn->isendlen - sendsize;
        return;
    }
    else if (sendsize == -1)
    {
        //这不太可能，可以发送数据时通知我发送数据，我发送时你却通知我发送缓冲区满？
        globallogger->clog(LogLevel::ERROR, "CSocket::write_request_handler()时if(sendsize == -1)成立，这很怪异。"); //打印个日志，别的先不干啥
        return;
    }

    if (sendsize > 0 && sendsize == pConn->isendlen) //成功发送完毕，做个通知是可以的；
    {
        //如果是成功的发送完毕数据，则把写事件通知从epoll中干掉吧；其他情况，那就是断线了，等着系统内核把连接从红黑树中干掉即可；
        if (epoll_oper_event(
            pConn->fd,          //socket句柄
            EPOLL_CTL_MOD,      //事件类型，这里是修改【因为我们准备减去写通知】
            EPOLLOUT,           //标志，这里代表要减去的标志,EPOLLOUT：可写【可写的时候通知我】
            1,                  //对于事件类型为增加的，EPOLL_CTL_MOD需要这个参数, 0：增加   1：去掉 2：完全覆盖
            pConn               //连接池中的连接
        ) == -1)
        {
            //有这情况发生？这可比较麻烦，不过先do nothing
            globallogger->clog(LogLevel::ERROR, "CSocket::write_request_handler()中epoll_oper_event()失败。");
        }

        //log_stderr(0,"CSocket::write_request_handler()中数据发送完毕，很好。"); //做个提示吧，商用时可以干掉

    }

    //能走下来的，要么数据发送完毕了，要么对端断开了，那么执行收尾工作吧；

    /* 2019.4.2注释掉，调整下顺序，感觉这个顺序不太好
    //数据发送完毕，或者把需要发送的数据干掉，都说明发送缓冲区可能有地方了，让发送线程往下走判断能否发送新数据
    if(sem_post(&m_semEventSendQueue)==-1)
        log_stderr(0,"CSocket::write_request_handler()中sem_post(&m_semEventSendQueue)失败.");


    p_memory->FreeMemory(pConn->psendMemPointer);  //释放内存
    pConn->psendMemPointer = NULL;
    --pConn->iThrowsendCount;  //建议放在最后执行
    */
    //2019.4.2调整成新顺序
    p_memory->FreeMemory(pConn->psendMemPointer);  //释放内存
    pConn->psendMemPointer = NULL;
    --pConn->iThrowsendCount;//这个值恢复了，触发下面一行的信号量才有意义
    if (sem_post(&m_semEventSendQueue) == -1)
        globallogger->clog(LogLevel::ERROR, "CSocket::write_request_handler()中sem_post(&m_semEventSendQueue)失败.");

    return;
}

/**
 * @brief 处理接收到的TCP消息
 * @details 该函数专门用于处理接收到的TCP消息，消息格式自解释，通过包头可以计算整个包的长度。该函数目前是一个占位符，具体的消息处理逻辑未实现。
 *
 * @param pMsgBuf 接收到的消息缓冲区
 */
void CSocket::threadRecvProcFunc(char* pMsgBuf)
{
    return;
}


