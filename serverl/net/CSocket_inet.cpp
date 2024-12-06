#include"CSocket.h"
#include"global.h"
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>


/**
 * @brief ���׽��ֵ�ַת��Ϊ�ַ�����ʾ
 * @details �ú����������� `sockaddr` ��ַת��Ϊ�ַ�����ʽ��֧��IPv4��ַ������ѡ���Ƿ�����˿���Ϣ��
 *
 * @param sa �׽��ֵ�ַ
 * @param port �Ƿ�����˿���Ϣ�ı�־��1��ʾ�����˿ڣ�0��ʾ������
 * @param text ���ڴ洢ת������Ļ�����
 * @param len ����������󳤶�
 *
 * @return ����ת������ַ�������
 */
size_t CSocket::sock_ntop(sockaddr* sa, int port, u_char* text, size_t len)
{
	if (sa->sa_family == AF_INET)  // ����IPv4��ַ
	{
		struct sockaddr_in* sin = (struct sockaddr_in*)sa;
		u_char* p = (u_char*)&sin->sin_addr;

		// �����Ҫ�˿���Ϣ
		if (port) {
			// ��IPv4��ַ�Ͷ˿ںŸ�ʽ�����ַ���
			return snprintf((char*)text, len, "%u.%u.%u.%u:%d",
				(unsigned char)p[0], (unsigned char)p[1],
				(unsigned char)p[2], (unsigned char)p[3],
				ntohs(sin->sin_port));
		}
		else {
			// ����ʽ��IPv4��ַ
			return snprintf((char*)text, len, "%u.%u.%u.%u",
				(unsigned char)p[0], (unsigned char)p[1],
				(unsigned char)p[2], (unsigned char)p[3]);
		}

		return (p - text);  // ���ظ�ʽ������ַ�������
	}

	return 0;  // ��IPv4��ַʱ����0
}
