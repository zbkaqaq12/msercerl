#include"CSocket.h"
#include"global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO等
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno
//#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>

/**
 * @brief 处理新连接的接入
 * @details 该函数在新连接到来时由 `ngx_epoll_process_events()` 调用。根据边缘触发（ET）或水平触发（LT）模式，确保只调用一次 `accept()`，避免阻塞程序。
 *          函数通过 `accept()` 或 `accept4()` 接受新的连接并将其加入连接池，同时将相关事件添加到 `epoll` 中。
 *
 * @param oldc 现有连接对象，用于获取监听套接字
 */
void CSocket::event_accept(lpconnection_t oldc)
{
	//因为listen套接字上用的不是ET【边缘触发】，而是LT【水平触发】，意味着客户端连入如果我要不处理，这个函数会被多次调用，所以，我这里这里可以不必多次accept()，可以只执行一次accept()
		 //这也可以避免本函数被卡太久，注意，本函数应该尽快返回，以免阻塞程序运行；
	struct sockaddr mysockaddr;	//远端服务器地址
	socklen_t socklen;
	int err;
	LogLevel level;
	int s;
	static int use_accept4 = 1;
	lpconnection_t newc;	//代表连接池中的一个连接

	socklen = sizeof(mysockaddr);
	do
	{
		if (use_accept4)
		{
			//以为listen套接字是非阻塞的，所以即便已完成连接队列为空，accept4()也不会卡在这里；
			s = accept4(oldc->fd, &mysockaddr, &socklen, SOCK_NONBLOCK);
		}
		else
		{
			//以为listen套接字是非阻塞的，所以即便已完成连接队列为空，accept()也不会卡在这里；
			s = accept(oldc->fd, &mysockaddr, &socklen);
		}

		//惊群，有时候不一定完全惊动所有4个worker进程，可能只惊动其中2个等等，其中一个成功其余的accept4()都会返回-1；错误 (11: Resource temporarily unavailable【资源暂时不可用】) 
	   //所以参考资料：https://blog.csdn.net/russell_tao/article/details/7204260
	   //其实，在linux2.6内核上，accept系统调用已经不存在惊群了（至少我在2.6.18内核版本上已经不存在）。大家可以写个简单的程序试下，在父进程中bind,listen，然后fork出子进程，
			  //所有的子进程都accept这个监听句柄。这样，当新连接过来时，大家会发现，仅有一个子进程返回新建的连接，其他子进程继续休眠在accept调用上，没有被唤醒。
		if (s == -1)
		{
			err = errno;

			//对accept、send和recv而言，事件未发生时errno通常被设置成EAGAIN（意为“再来一次”）或者EWOULDBLOCK（意为“期待阻塞”）
			if (err == EAGAIN) //accept()没准备好，这个EAGAIN错误EWOULDBLOCK是一样的
			{
				//除非你用一个循环不断的accept()取走所有的连接，不然一般不会有这个错误【我们这里只取一个连接，也就是accept()一次】
				return;
			}
			level = LogLevel::ALERT;
			if (err == ECONNABORTED)//ECONNRESET错误则发生在对方意外关闭套接字后【您的主机中的软件放弃了一个已建立的连接--由于超时或者其它失败而中止接连(用户插拔网线就可能有这个错误出现)】
			{
				//该错误被描述为“software caused connection abort”，即“软件引起的连接中止”。原因在于当服务和客户进程在完成用于 TCP 连接的“三次握手”后，
					//客户 TCP 却发送了一个 RST （复位）分节，在服务进程看来，就在该连接已由 TCP 排队，等着服务进程调用 accept 的时候 RST 却到达了。
					//POSIX 规定此时的 errno 值必须 ECONNABORTED。源自 Berkeley 的实现完全在内核中处理中止的连接，服务进程将永远不知道该中止的发生。
						//服务器进程一般可以忽略该错误，直接再次调用accept。
				level = LogLevel::ERROR;
			}
			else if (err == EMFILE || err == ENFILE) //EMFILE:进程的fd已用尽【已达到系统所允许单一进程所能打开的文件/套接字总数】。可参考：https://blog.csdn.net/sdn_prc/article/details/28661661   以及 https://bbs.csdn.net/topics/390592927
				//ulimit -n ,看看文件描述符限制,如果是1024的话，需要改大;  打开的文件句柄数过多 ,把系统的fd软限制和硬限制都抬高.
			//ENFILE这个errno的存在，表明一定存在system-wide的resource limits，而不仅仅有process-specific的resource limits。按照常识，process-specific的resource limits，一定受限于system-wide的resource limits。
			{
				level = LogLevel::CRIT;
			}
			globallogger->flog(level, "CSocekt::ngx_event_accept()中accept4()失败!");

			if (use_accept4 && err == ENOSYS) //accept4()函数没实现，坑爹？
			{
				use_accept4 = 0;  //标记不使用accept4()函数，改用accept()函数
				continue;         //回去重新用accept()函数搞
			}

			if (err == ECONNABORTED)  //对方关闭套接字
			{
				//这个错误因为可以忽略，所以不用干啥
				//do nothing
			}

			if (err == EMFILE || err == ENFILE)
			{
				//do nothing，这个官方做法是先把读事件从listen socket上移除，然后再弄个定时器，定时器到了则继续执行该函数，但是定时器到了有个标记，会把读事件增加到listen socket上去；
				//我这里目前先不处理吧【因为上边已经写这个日志了】；
			}
			return;
		}

		//走到这里的，表示accept4()/accept()成功了  
		newc = get_connection(s); //这是针对新连入用户的连接，和监听套接字 所对应的连接是两个不同的东西，不要搞混
		if (newc == NULL)
		{
			//连接池中连接不够用，那么就得把这个socekt直接关闭并返回了，因为在ngx_get_connection()中已经写日志了，所以这里不需要写日志了
			if (close(s) == -1)
			{
				globallogger->flog(LogLevel::ALERT, "CSocekt::ngx_event_accept()中close(%d)失败!", s);
			}
			return;
		}
		//...........将来这里会判断是否连接超过最大允许连接数，现在，这里可以不处理

		//成功的拿到了连接池中的一个连接
		memcpy(&newc->s_sockaddr, &mysockaddr, socklen);  //拷贝客户端地址到连接对象【要转成字符串ip地址参考函数ngx_sock_ntop()】

		if (!use_accept4)
		{
			//如果不是用accept4()取得的socket，那么就要设置为非阻塞【因为用accept4()的已经被accept4()设置为非阻塞了】
			if (setnonblocking(s) == false)
			{
				//设置非阻塞居然失败
				close_connection(newc); //回收连接池中的连接（千万不能忘记），并关闭socket
				return; //直接返回
			}
		}

		newc->listening = oldc->listening;                    //连接对象 和监听对象关联，方便通过连接对象找监听对象【关联到监听端口】
		//newc->w_ready = 1;                                    //标记可以写，新连接写事件肯定是ready的；【从连接池拿出一个连接时这个连接的所有成员都是0】            

		newc->rhandler = &CSocket::read_request_handler;  //设置数据来时的读处理函数，其实官方nginx中是ngx_http_wait_request_handler()
		newc->whandler = &CSocket::write_request_handler; //设置数据发送时的写处理函数。
		//客户端应该主动发送第一次的数据，这里将读事件加入epoll监控，这样当客户端发送数据来时，会触发ngx_wait_request_handler()被ngx_epoll_process_events()调用        
		if (epoll_oper_event(
			s,                  //socekt句柄
			EPOLL_CTL_ADD,      //事件类型，这里是增加
			EPOLLIN | EPOLLRDHUP, //标志，这里代表要增加的标志,EPOLLIN：可读，EPOLLRDHUP：TCP连接的远端关闭或者半关闭 ，如果边缘触发模式可以增加 EPOLLET
			0,                  //对于事件类型为增加的，不需要这个参数
			newc                //连接池中的连接
		) == -1)
		{
			//增加事件失败，失败日志在ngx_epoll_add_event中写过了，因此这里不多写啥；
			close_connection(newc);//关闭socket,这种可以立即回收这个连接，无需延迟，因为其上还没有数据收发，谈不到业务逻辑因此无需延迟；
			return; //直接返回
		}
		/*
		else
		{
			//打印下发送缓冲区大小
			int           n;
			socklen_t     len;
			len = sizeof(int);
			getsockopt(s,SOL_SOCKET,SO_SNDBUF, &n, &len);
			ngx_log_stderr(0,"发送缓冲区的大小为%d!",n); //87040

			n = 0;
			getsockopt(s,SOL_SOCKET,SO_RCVBUF, &n, &len);
			ngx_log_stderr(0,"接收缓冲区的大小为%d!",n); //374400

			int sendbuf = 2048;
			if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,(const void *) &sendbuf,n) == 0)
			{
				ngx_log_stderr(0,"发送缓冲区大小成功设置为%d!",sendbuf);
			}

			 getsockopt(s,SOL_SOCKET,SO_SNDBUF, &n, &len);
			ngx_log_stderr(0,"发送缓冲区的大小为%d!",n); //87040
		}
		*/

		if (m_ifkickTimeCount == 1)
		{
			AddToTimerQueue(newc);
		}
		++m_onlineUserCount;  //连入用户数量+1 
		break;  //一般就是循环一次就跳出去

	} while (1);

	return;
}
