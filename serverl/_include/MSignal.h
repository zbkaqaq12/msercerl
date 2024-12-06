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
 * @brief 信号管理类，负责设置信号处理函数并解封指定的信号。
 *
 * 该类用于信号的初始化、解封和设置处理函数。可以添加信号处理函数来处理特定的信号，如 `SIGHUP`、`SIGINT`、`SIGTERM` 等。
 */
class MSignal
{
public:
	using SignalHandlerFunction = void (*)(int signo, siginfo_t* siginfo, void* ucontext);

	/**
	* @struct SignalInfo
	* @brief 存储信号信息的结构体。
	*
	* 用于存储信号的编号、名称以及对应的信号处理函数。
	*/
	struct SignalInfo {
		int signo;                /**< 信号编号 */
		std::string signame;      /**< 信号名称 */
		SignalHandlerFunction handler;  /**< 信号处理函数 */
	};

	MSignal()
	{ };

	int init_signal();

	// 解封指定的信号并设置其处理函数
	void unmask_and_set_handler(int signo, SignalHandlerFunction handler);

	static void defaultSignalHandler(int signo, siginfo_t* siginfo, void* ucontext);

	static void reapChildProcess(int signo, siginfo_t* siginfo, void* ucontext);

	void addSignal(int signo, const std::string& signame, SignalHandlerFunction handler);

private:
	sigset_t set;  // 存储已屏蔽的信号集合

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

