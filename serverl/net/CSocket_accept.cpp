#include"CSocket.h"
#include"global.h"
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

/**
 * @brief ���������ӵĽ���
 * @details �ú����������ӵ���ʱ�� `ngx_epoll_process_events()` ���á����ݱ�Ե������ET����ˮƽ������LT��ģʽ��ȷ��ֻ����һ�� `accept()`��������������
 *          ����ͨ�� `accept()` �� `accept4()` �����µ����Ӳ�����������ӳأ�ͬʱ������¼���ӵ� `epoll` �С�
 *
 * @param oldc �������Ӷ������ڻ�ȡ�����׽���
 */
void CSocket::event_accept(lpconnection_t oldc)
{
	//��Ϊlisten�׽������õĲ���ET����Ե������������LT��ˮƽ����������ζ�ſͻ������������Ҫ��������������ᱻ��ε��ã����ԣ�������������Բ��ض��accept()������ִֻ��һ��accept()
		 //��Ҳ���Ա��Ȿ��������̫�ã�ע�⣬������Ӧ�þ��췵�أ����������������У�
	struct sockaddr mysockaddr;	//Զ�˷�������ַ
	socklen_t socklen;
	int err;
	LogLevel level;
	int s;
	static int use_accept4 = 1;
	lpconnection_t newc;	//�������ӳ��е�һ������

	socklen = sizeof(mysockaddr);
	do
	{
		if (use_accept4)
		{
			//��Ϊlisten�׽����Ƿ������ģ����Լ�����������Ӷ���Ϊ�գ�accept4()Ҳ���Ῠ�����
			s = accept4(oldc->fd, &mysockaddr, &socklen, SOCK_NONBLOCK);
		}
		else
		{
			//��Ϊlisten�׽����Ƿ������ģ����Լ�����������Ӷ���Ϊ�գ�accept()Ҳ���Ῠ�����
			s = accept(oldc->fd, &mysockaddr, &socklen);
		}

		//��Ⱥ����ʱ��һ����ȫ��������4��worker���̣�����ֻ��������2���ȵȣ�����һ���ɹ������accept4()���᷵��-1������ (11: Resource temporarily unavailable����Դ��ʱ�����á�) 
	   //���Բο����ϣ�https://blog.csdn.net/russell_tao/article/details/7204260
	   //��ʵ����linux2.6�ں��ϣ�acceptϵͳ�����Ѿ������ھ�Ⱥ�ˣ���������2.6.18�ں˰汾���Ѿ������ڣ�����ҿ���д���򵥵ĳ������£��ڸ�������bind,listen��Ȼ��fork���ӽ��̣�
			  //���е��ӽ��̶�accept�������������������������ӹ���ʱ����һᷢ�֣�����һ���ӽ��̷����½������ӣ������ӽ��̼���������accept�����ϣ�û�б����ѡ�
		if (s == -1)
		{
			err = errno;

			//��accept��send��recv���ԣ��¼�δ����ʱerrnoͨ�������ó�EAGAIN����Ϊ������һ�Ρ�������EWOULDBLOCK����Ϊ���ڴ���������
			if (err == EAGAIN) //accept()û׼���ã����EAGAIN����EWOULDBLOCK��һ����
			{
				//��������һ��ѭ�����ϵ�accept()ȡ�����е����ӣ���Ȼһ�㲻�������������������ֻȡһ�����ӣ�Ҳ����accept()һ�Ρ�
				return;
			}
			level = LogLevel::ALERT;
			if (err == ECONNABORTED)//ECONNRESET���������ڶԷ�����ر��׽��ֺ����������е����������һ���ѽ���������--���ڳ�ʱ��������ʧ�ܶ���ֹ����(�û�������߾Ϳ���������������)��
			{
				//�ô�������Ϊ��software caused connection abort����������������������ֹ����ԭ�����ڵ�����Ϳͻ�������������� TCP ���ӵġ��������֡���
					//�ͻ� TCP ȴ������һ�� RST ����λ���ֽڣ��ڷ�����̿��������ڸ��������� TCP �Ŷӣ����ŷ�����̵��� accept ��ʱ�� RST ȴ�����ˡ�
					//POSIX �涨��ʱ�� errno ֵ���� ECONNABORTED��Դ�� Berkeley ��ʵ����ȫ���ں��д�����ֹ�����ӣ�������̽���Զ��֪������ֹ�ķ�����
						//����������һ����Ժ��Ըô���ֱ���ٴε���accept��
				level = LogLevel::ERROR;
			}
			else if (err == EMFILE || err == ENFILE) //EMFILE:���̵�fd���þ����Ѵﵽϵͳ������һ�������ܴ򿪵��ļ�/�׽������������ɲο���https://blog.csdn.net/sdn_prc/article/details/28661661   �Լ� https://bbs.csdn.net/topics/390592927
				//ulimit -n ,�����ļ�����������,�����1024�Ļ�����Ҫ�Ĵ�;  �򿪵��ļ���������� ,��ϵͳ��fd�����ƺ�Ӳ���ƶ�̧��.
			//ENFILE���errno�Ĵ��ڣ�����һ������system-wide��resource limits������������process-specific��resource limits�����ճ�ʶ��process-specific��resource limits��һ��������system-wide��resource limits��
			{
				level = LogLevel::CRIT;
			}
			globallogger->flog(level, "CSocekt::ngx_event_accept()��accept4()ʧ��!");

			if (use_accept4 && err == ENOSYS) //accept4()����ûʵ�֣��ӵ���
			{
				use_accept4 = 0;  //��ǲ�ʹ��accept4()����������accept()����
				continue;         //��ȥ������accept()������
			}

			if (err == ECONNABORTED)  //�Է��ر��׽���
			{
				//���������Ϊ���Ժ��ԣ����Բ��ø�ɶ
				//do nothing
			}

			if (err == EMFILE || err == ENFILE)
			{
				//do nothing������ٷ��������ȰѶ��¼���listen socket���Ƴ���Ȼ����Ū����ʱ������ʱ�����������ִ�иú��������Ƕ�ʱ�������и���ǣ���Ѷ��¼����ӵ�listen socket��ȥ��
				//������Ŀǰ�Ȳ�����ɡ���Ϊ�ϱ��Ѿ�д�����־�ˡ���
			}
			return;
		}

		//�ߵ�����ģ���ʾaccept4()/accept()�ɹ���  
		newc = get_connection(s); //��������������û������ӣ��ͼ����׽��� ����Ӧ��������������ͬ�Ķ�������Ҫ���
		if (newc == NULL)
		{
			//���ӳ������Ӳ����ã���ô�͵ð����socektֱ�ӹرղ������ˣ���Ϊ��ngx_get_connection()���Ѿ�д��־�ˣ��������ﲻ��Ҫд��־��
			if (close(s) == -1)
			{
				globallogger->flog(LogLevel::ALERT, "CSocekt::ngx_event_accept()��close(%d)ʧ��!", s);
			}
			return;
		}
		//...........����������ж��Ƿ����ӳ���������������������ڣ�������Բ�����

		//�ɹ����õ������ӳ��е�һ������
		memcpy(&newc->s_sockaddr, &mysockaddr, socklen);  //�����ͻ��˵�ַ�����Ӷ���Ҫת���ַ���ip��ַ�ο�����ngx_sock_ntop()��

		if (!use_accept4)
		{
			//���������accept4()ȡ�õ�socket����ô��Ҫ����Ϊ����������Ϊ��accept4()���Ѿ���accept4()����Ϊ�������ˡ�
			if (setnonblocking(s) == false)
			{
				//���÷�������Ȼʧ��
				close_connection(newc); //�������ӳ��е����ӣ�ǧ�������ǣ������ر�socket
				return; //ֱ�ӷ���
			}
		}

		newc->listening = oldc->listening;                    //���Ӷ��� �ͼ����������������ͨ�����Ӷ����Ҽ������󡾹����������˿ڡ�
		//newc->w_ready = 1;                                    //��ǿ���д��������д�¼��϶���ready�ģ��������ӳ��ó�һ������ʱ������ӵ����г�Ա����0��            

		newc->rhandler = &CSocket::read_request_handler;  //����������ʱ�Ķ�����������ʵ�ٷ�nginx����ngx_http_wait_request_handler()
		newc->whandler = &CSocket::write_request_handler; //�������ݷ���ʱ��д��������
		//�ͻ���Ӧ���������͵�һ�ε����ݣ����ｫ���¼�����epoll��أ��������ͻ��˷���������ʱ���ᴥ��ngx_wait_request_handler()��ngx_epoll_process_events()����        
		if (epoll_oper_event(
			s,                  //socekt���
			EPOLL_CTL_ADD,      //�¼����ͣ�����������
			EPOLLIN | EPOLLRDHUP, //��־���������Ҫ���ӵı�־,EPOLLIN���ɶ���EPOLLRDHUP��TCP���ӵ�Զ�˹رջ��߰�ر� �������Ե����ģʽ�������� EPOLLET
			0,                  //�����¼�����Ϊ���ӵģ�����Ҫ�������
			newc                //���ӳ��е�����
		) == -1)
		{
			//�����¼�ʧ�ܣ�ʧ����־��ngx_epoll_add_event��д���ˣ�������ﲻ��дɶ��
			close_connection(newc);//�ر�socket,���ֿ�����������������ӣ������ӳ٣���Ϊ���ϻ�û�������շ���̸����ҵ���߼���������ӳ٣�
			return; //ֱ�ӷ���
		}
		/*
		else
		{
			//��ӡ�·��ͻ�������С
			int           n;
			socklen_t     len;
			len = sizeof(int);
			getsockopt(s,SOL_SOCKET,SO_SNDBUF, &n, &len);
			ngx_log_stderr(0,"���ͻ������Ĵ�СΪ%d!",n); //87040

			n = 0;
			getsockopt(s,SOL_SOCKET,SO_RCVBUF, &n, &len);
			ngx_log_stderr(0,"���ջ������Ĵ�СΪ%d!",n); //374400

			int sendbuf = 2048;
			if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,(const void *) &sendbuf,n) == 0)
			{
				ngx_log_stderr(0,"���ͻ�������С�ɹ�����Ϊ%d!",sendbuf);
			}

			 getsockopt(s,SOL_SOCKET,SO_SNDBUF, &n, &len);
			ngx_log_stderr(0,"���ͻ������Ĵ�СΪ%d!",n); //87040
		}
		*/

		if (m_ifkickTimeCount == 1)
		{
			AddToTimerQueue(newc);
		}
		++m_onlineUserCount;  //�����û�����+1 
		break;  //һ�����ѭ��һ�ξ�����ȥ

	} while (1);

	return;
}
