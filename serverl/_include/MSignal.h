#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <memory>
#include<signal.h>
#include"global.h"

/**
 * @class MSignal
 * @brief �źŹ����࣬���������źŴ����������ָ�����źš�
 *
 * ���������źŵĳ�ʼ�����������ô���������������źŴ������������ض����źţ��� `SIGHUP`��`SIGINT`��`SIGTERM` �ȡ�
 */
class MSignal
{
public:
	using SignalHandlerFunction = void (*)(int signo, siginfo_t* siginfo, void* ucontext);

	/**
	* @struct SignalInfo
	* @brief �洢�ź���Ϣ�Ľṹ�塣
	*
	* ���ڴ洢�źŵı�š������Լ���Ӧ���źŴ�������
	*/
	struct SignalInfo {
		int signo;                /**< �źű�� */
		std::string signame;      /**< �ź����� */
		SignalHandlerFunction handler;  /**< �źŴ����� */
	};

	MSignal()
	{ };

	int init_signal();

	// ���ָ�����źŲ������䴦����
	void unmask_and_set_handler(int signo, SignalHandlerFunction handler);

	static void defaultSignalHandler(int signo, siginfo_t* siginfo, void* ucontext);

	static void reapChildProcess(int signo, siginfo_t* siginfo, void* ucontext);

	void addSignal(int signo, const std::string& signame, SignalHandlerFunction handler);

private:
	sigset_t set;  // �洢�����ε��źż���

	std::vector<SignalInfo> signals_{
		{ SIGHUP, "SIGHUP", defaultSignalHandler },
		{ SIGINT, "SIGINT", defaultSignalHandler },
		{ SIGTERM, "SIGTERM", defaultSignalHandler },
		{ SIGCHLD, "SIGCHLD", reapChildProcess },
		{ SIGQUIT, "SIGQUIT", defaultSignalHandler },
		{ SIGIO, "SIGIO", defaultSignalHandler },
		{ SIGSYS, "SIGSYS", nullptr } // SIGSYS is ignored
	};
};

