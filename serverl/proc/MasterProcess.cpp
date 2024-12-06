#include "MasterProcess.h"
#include <unistd.h>
#include<sys/types.h>

/**
* @brief 启动主进程，初始化信号处理器并启动工作进程。
*
* 1. 初始化信号处理器，设置信号处理函数；
* 2. 设置主进程标题；
* 3. 从配置文件加载工作进程数量并启动相应数量的工作进程；
* 4. 进入等待信号的状态。
*/
void MasterProcess::start()
{
    // 初始化信号处理器
    if (signalHandler_->init_signal() == -1) {
        globallogger->clog(LogLevel::ALERT, "信号初始化失败!");
        return;
    }

    // 解除并设置需要处理的信号
    //signalHandler_->unmask_and_set_handler(SIGTERM, handleSIGTERM);
    //signalHandler_->unmask_and_set_handler(SIGINT, handleSIGINT);
    // 解除并设置需要处理的信号
    signalHandler_->unmask_and_set_handler(SIGTERM, handleSIGTERM);
    signalHandler_->unmask_and_set_handler(SIGINT, handleSIGINT);
    signalHandler_->unmask_and_set_handler(SIGCHLD, handleSIGCHLD);
    signalHandler_->unmask_and_set_handler(SIGHUP, handleSIGHUP);

    // 设置主进程标题
    set_process_title();

    // 从配置文件中读取工作进程数量
    int worker_count = globalconfig->GetIntDefault("WorkerProcesses", 1);;
    start_worker_processes(worker_count);

    // 等待信号驱动
    wait_for_signal();
}

/**
* @brief 设置主进程标题。
*
* 设置主进程的进程标题，使其更具可读性和可管理性。标题包含进程名和命令行参数。
*/
void MasterProcess::set_process_title()
{
    // 设置主进程标题
    size_t size;
    int    i;
    size = sizeof(processName_);  //注意我这里用的是sizeof，所以字符串末尾的\0是被计算进来了的
    size += g_argvneedmem;          //argv参数长度加进来    
    if (size < 1000) //长度小于这个，我才设置标题
    {
        char title[1000] = { 0 };
        strcpy(title, processName_.c_str()); //"master process"
        strcat(title, " ");  //跟一个空格分开一些，清晰    //"master process "
        for (i = 0; i < g_os_argc; i++)         //"master process ./nginx"
        {
            strcat(title, g_os_argv[i]);
        }//end for
        setproctitle(title); //设置标题
    }
}

/**
* @brief 创建并启动一个新的工作进程。
*
* @param num 工作进程编号，用于区分不同的工作进程。
*/
void MasterProcess::spawn_worker_process(int num)
{
    pid_t pid = fork();
    if (pid == -1) {
        globallogger->flog(LogLevel::ALERT, "fork()产生子进程失败!");
        return;
    }

    if (pid == 0) {
        // 子进程代码
        try {
            // 创建 WorkerProcess 对象
            std::unique_ptr<WorkerProcess> worker = std::make_unique<WorkerProcess>();
            // 确保 worker 不为 nullptr
            if (worker) {
                worker->start();  // 启动工作进程
            }
            else {
                std::cerr << "Failed to create WorkerProcess instance." << std::endl;
                exit(1);
            }

        }
        catch (const std::exception& e) {
            std::cerr << "Exception in child process: " << e.what() << std::endl;
            exit(1);  // 出现异常时退出子进程
        }

        exit(0);  // 子进程退出
    }
    // 父进程继续执行
}

/**
 * @brief 处理 SIGTERM 信号。
 *
 * 当主进程收到 SIGTERM 信号时，执行清理操作，通常用于平滑终止进程。
 * 在该函数中，你可以停止所有工作进程，释放资源等。
 *
 * @param signo 信号编号，表示接收到的信号类型（此时是 SIGTERM）。
 * @param siginfo 信号信息，包含关于信号的更多信息。
 * @param ucontext 上下文信息，包含关于信号处理的上下文（如寄存器状态等）。
 */
void MasterProcess::handleSIGTERM(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "处理 SIGTERM 信号，进行清理操作");
    // 在这里执行清理操作，如停止工作进程等
    // 清理子进程、日志处理等资源
}

/**
 * @brief 处理 SIGINT 信号。
 *
 * 当主进程收到 SIGINT 信号时，停止日志处理并进行资源释放操作。通常这是通过键盘中断（Ctrl+C）触发的。
 * 在该函数中，可以停止所有工作进程、清理资源、保存日志等。
 *
 * @param signo 信号编号，表示接收到的信号类型（此时是 SIGINT）。
 * @param siginfo 信号信息，包含关于信号的更多信息。
 * @param ucontext 上下文信息，包含关于信号处理的上下文（如寄存器状态等）。
 */
void MasterProcess::handleSIGINT(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->stoplogprocess();
    globallogger->clog(LogLevel::INFO, "处理 SIGINT 信号，进行清理操作");
    // 中断信号后，执行资源释放和终止操作
    // 停止所有工作进程、清理资源等
}

/**
 * @brief 处理 SIGCHLD 信号。
 *
 * 当子进程结束时，父进程会接收到 SIGCHLD 信号。该函数会处理子进程的退出状态，防止出现僵尸进程。
 * 它会调用 `waitpid` 来回收子进程状态，并输出退出信息。
 *
 * @param signo 信号编号，表示接收到的信号类型（此时是 SIGCHLD）。
 * @param siginfo 信号信息，包含关于信号的更多信息。
 * @param ucontext 上下文信息，包含关于信号处理的上下文（如寄存器状态等）。
 */
void MasterProcess::handleSIGCHLD(int signo, siginfo_t* siginfo, void* ucontext)
{
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
        }
        else if (WIFSIGNALED(status)) {
            std::cout << "Child process " << pid << " killed by signal " << WTERMSIG(status) << std::endl;
        }
    }
}

/**
 * @brief 处理 SIGHUP 信号。
 *
 * 当进程接收到 SIGHUP 信号时，通常意味着需要重新加载配置文件或者执行其他恢复操作。
 * 可以在该函数中加入重新加载配置的逻辑，或者进行其他必要的恢复操作。
 *
 * @param signo 信号编号，表示接收到的信号类型（此时是 SIGHUP）。
 * @param siginfo 信号信息，包含关于信号的更多信息。
 * @param ucontext 上下文信息，包含关于信号处理的上下文（如寄存器状态等）。
 */
void MasterProcess::handleSIGHUP(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "接收到 SIGHUP 信号，重新加载配置");
    // 可以在这里重新加载配置文件或执行其他恢复操作
}

