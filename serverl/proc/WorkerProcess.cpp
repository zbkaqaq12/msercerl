#include "WorkerProcess.h"

/**
 * @brief 初始化工作进程。
 * 
 * 该方法负责初始化子进程，包括设置进程标题、初始化信号处理器、线程池、Socket、Epoll 等。
 */
void WorkerProcess::init_process()
{
    // 设置进程名
    set_process_title();
    
    // 初始化信号处理器
    if (signalHandler_->init_signal() == -1) {
        globallogger->clog(LogLevel::ALERT, "信号初始化失败!");
        return;
    }
    
    // 设置信号处理器
    signalHandler_->unmask_and_set_handler(SIGTERM, handleSIGTERM);
    signalHandler_->unmask_and_set_handler(SIGINT, handleSIGINT);
    signalHandler_->unmask_and_set_handler(SIGHUP, handleSIGHUP);
    signalHandler_->unmask_and_set_handler(SIGUSR1, handleSIGUSR1);
    signalHandler_->unmask_and_set_handler(SIGUSR2, handleSIGUSR2);
  
    // 初始化线程池，处理接收到的消息
    int tmpthreadnums = globalconfig->GetIntDefault("ProcMsgRecvWorkThreadCount", 5);
    if (g_threadpool.Create(tmpthreadnums) == false) {
        // 如果线程池创建失败，退出
        exit(-2);
    }
    sleep(1); // 稍作休息
    
    // 初始化子进程所需的多线程能力
    if (g_socket.Initialize_subproc() == false) {
        // 初始化失败，退出
        exit(-2);
    }
    
    // 初始化 Epoll 并为监听端口添加监听事件
    g_socket.epoll_init();
}

/**
 * @brief 设置子进程标题。
 * 
 * 该方法通过设置进程标题来标识当前进程，以便在系统进程列表中显示。
 */
void WorkerProcess::set_process_title()
{
    size_t size;
    int i;
    size = sizeof(processName_);
    size += g_argvneedmem;  // 包括命令行参数的内存

    if (size < 1000) {
        char title[1000] = { 0 };
        strcpy(title, processName_.c_str());
        strcat(title, " ");  // 在进程名后加空格
        for (i = 0; i < g_os_argc; i++) {
            strcat(title, g_os_argv[i]); // 拼接命令行参数
        }
        setproctitle(title); // 设置进程标题
    }
}

/**
 * @brief 处理 SIGTERM 信号。
 * 
 * 该方法在接收到 SIGTERM 信号时执行清理操作，并终止工作进程。
 * 
 * @param signo 信号编号。
 * @param siginfo 信号信息。
 * @param ucontext 上下文信息。
 */
void WorkerProcess::handleSIGTERM(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGTERM, performing cleanup...");
    // 执行清理操作后退出
    exit(0);
}

/**
 * @brief 处理 SIGINT 信号。
 * 
 * 该方法在接收到 SIGINT 信号时执行清理操作，并终止工作进程。
 * 
 * @param signo 信号编号。
 * @param siginfo 信号信息。
 * @param ucontext 上下文信息。
 */
void WorkerProcess::handleSIGINT(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGINT, performing cleanup...");
    // 执行清理操作后退出
    exit(0);
}

/**
 * @brief 处理 SIGHUP 信号。
 * 
 * 该方法在接收到 SIGHUP 信号时执行配置重载操作。
 * 
 * @param signo 信号编号。
 * @param siginfo 信号信息。
 * @param ucontext 上下文信息。
 */
void WorkerProcess::handleSIGHUP(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGHUP, reloading configuration...");
    // 重新加载配置等操作
}

/**
 * @brief 处理 SIGUSR1 信号。
 * 
 * 该方法在接收到 SIGUSR1 信号时执行用户自定义任务。
 * 
 * @param signo 信号编号。
 * @param siginfo 信号信息。
 * @param ucontext 上下文信息。
 */
void WorkerProcess::handleSIGUSR1(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGUSR1, performing specific task...");
    // 执行用户自定义任务
}

/**
 * @brief 处理 SIGUSR2 信号。
 * 
 * 该方法在接收到 SIGUSR2 信号时执行另一种用户自定义任务。
 * 
 * @param signo 信号编号。
 * @param siginfo 信号信息。
 * @param ucontext 上下文信息。
 */
void WorkerProcess::handleSIGUSR2(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGUSR2, performing another task...");
    // 执行其他用户自定义任务
}

/**
 * @brief 启动工作进程的事件循环。
 * 
 * 该方法进入工作进程的事件循环，处理网络事件、定时任务等。
 * 
 * 在事件循环中，工作进程会调用 Epoll 来处理事件。
 */
void WorkerProcess::run()
{
    // 进入子进程的事件循环
    for (;;) {
        // ngx_process_events_and_timers();  // 处理网络事件和定时器事件
        // std::cout << "this is worker pid  " << getpid() << std::endl;
        // std::cout << "this is master pid  " << getppid() << std::endl;
        // sleep(1);
        g_socket.epoll_process_events(-1);
    }

    // 退出事件循环后，停止线程池和释放资源
    g_threadpool.StopAll();      // 停止线程池
    g_socket.Shutdown_subproc(); // 释放与 socket 相关的资源
}
