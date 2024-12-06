#include "WorkerProcess.h"

/**
 * @brief ��ʼ���������̡�
 * 
 * �÷��������ʼ���ӽ��̣��������ý��̱��⡢��ʼ���źŴ��������̳߳ء�Socket��Epoll �ȡ�
 */
void WorkerProcess::init_process()
{
    // ���ý�����
    set_process_title();
    
    // ��ʼ���źŴ�����
    if (signalHandler_->init_signal() == -1) {
        globallogger->clog(LogLevel::ALERT, "�źų�ʼ��ʧ��!");
        return;
    }
    
    // �����źŴ�����
    signalHandler_->unmask_and_set_handler(SIGTERM, handleSIGTERM);
    signalHandler_->unmask_and_set_handler(SIGINT, handleSIGINT);
    signalHandler_->unmask_and_set_handler(SIGHUP, handleSIGHUP);
    signalHandler_->unmask_and_set_handler(SIGUSR1, handleSIGUSR1);
    signalHandler_->unmask_and_set_handler(SIGUSR2, handleSIGUSR2);
  
    // ��ʼ���̳߳أ�������յ�����Ϣ
    int tmpthreadnums = globalconfig->GetIntDefault("ProcMsgRecvWorkThreadCount", 5);
    if (g_threadpool.Create(tmpthreadnums) == false) {
        // ����̳߳ش���ʧ�ܣ��˳�
        exit(-2);
    }
    sleep(1); // ������Ϣ
    
    // ��ʼ���ӽ�������Ķ��߳�����
    if (g_socket.Initialize_subproc() == false) {
        // ��ʼ��ʧ�ܣ��˳�
        exit(-2);
    }
    
    // ��ʼ�� Epoll ��Ϊ�����˿���Ӽ����¼�
    g_socket.epoll_init();
}

/**
 * @brief �����ӽ��̱��⡣
 * 
 * �÷���ͨ�����ý��̱�������ʶ��ǰ���̣��Ա���ϵͳ�����б�����ʾ��
 */
void WorkerProcess::set_process_title()
{
    size_t size;
    int i;
    size = sizeof(processName_);
    size += g_argvneedmem;  // ���������в������ڴ�

    if (size < 1000) {
        char title[1000] = { 0 };
        strcpy(title, processName_.c_str());
        strcat(title, " ");  // �ڽ�������ӿո�
        for (i = 0; i < g_os_argc; i++) {
            strcat(title, g_os_argv[i]); // ƴ�������в���
        }
        setproctitle(title); // ���ý��̱���
    }
}

/**
 * @brief ���� SIGTERM �źš�
 * 
 * �÷����ڽ��յ� SIGTERM �ź�ʱִ���������������ֹ�������̡�
 * 
 * @param signo �źű�š�
 * @param siginfo �ź���Ϣ��
 * @param ucontext ��������Ϣ��
 */
void WorkerProcess::handleSIGTERM(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGTERM, performing cleanup...");
    // ִ������������˳�
    exit(0);
}

/**
 * @brief ���� SIGINT �źš�
 * 
 * �÷����ڽ��յ� SIGINT �ź�ʱִ���������������ֹ�������̡�
 * 
 * @param signo �źű�š�
 * @param siginfo �ź���Ϣ��
 * @param ucontext ��������Ϣ��
 */
void WorkerProcess::handleSIGINT(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGINT, performing cleanup...");
    // ִ������������˳�
    exit(0);
}

/**
 * @brief ���� SIGHUP �źš�
 * 
 * �÷����ڽ��յ� SIGHUP �ź�ʱִ���������ز�����
 * 
 * @param signo �źű�š�
 * @param siginfo �ź���Ϣ��
 * @param ucontext ��������Ϣ��
 */
void WorkerProcess::handleSIGHUP(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGHUP, reloading configuration...");
    // ���¼������õȲ���
}

/**
 * @brief ���� SIGUSR1 �źš�
 * 
 * �÷����ڽ��յ� SIGUSR1 �ź�ʱִ���û��Զ�������
 * 
 * @param signo �źű�š�
 * @param siginfo �ź���Ϣ��
 * @param ucontext ��������Ϣ��
 */
void WorkerProcess::handleSIGUSR1(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGUSR1, performing specific task...");
    // ִ���û��Զ�������
}

/**
 * @brief ���� SIGUSR2 �źš�
 * 
 * �÷����ڽ��յ� SIGUSR2 �ź�ʱִ����һ���û��Զ�������
 * 
 * @param signo �źű�š�
 * @param siginfo �ź���Ϣ��
 * @param ucontext ��������Ϣ��
 */
void WorkerProcess::handleSIGUSR2(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "Worker process received SIGUSR2, performing another task...");
    // ִ�������û��Զ�������
}

/**
 * @brief �����������̵��¼�ѭ����
 * 
 * �÷������빤�����̵��¼�ѭ�������������¼�����ʱ����ȡ�
 * 
 * ���¼�ѭ���У��������̻���� Epoll �������¼���
 */
void WorkerProcess::run()
{
    // �����ӽ��̵��¼�ѭ��
    for (;;) {
        // ngx_process_events_and_timers();  // ���������¼��Ͷ�ʱ���¼�
        // std::cout << "this is worker pid  " << getpid() << std::endl;
        // std::cout << "this is master pid  " << getppid() << std::endl;
        // sleep(1);
        g_socket.epoll_process_events(-1);
    }

    // �˳��¼�ѭ����ֹͣ�̳߳غ��ͷ���Դ
    g_threadpool.StopAll();      // ֹͣ�̳߳�
    g_socket.Shutdown_subproc(); // �ͷ��� socket ��ص���Դ
}
