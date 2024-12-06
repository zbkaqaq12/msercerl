#include "MSignal.h"

/**
 * @brief ��ʼ���ź�ϵͳ�����β�ϣ�����յ��źŲ�Ϊ�ź����ô�������
 *
 * �÷������ʼ���ź������֣����β�ϣ�����յ��źţ��� `SIGCHLD`��`SIGINT` �ȣ���
 * Ȼ������Ϊ�ź��б��е��ź�ע����Ӧ�Ĵ�������
 *
 * @return int ���� 0 ��ʾ�ɹ������� -1 ��ʾʧ�ܡ�
 */
int MSignal::init_signal()
{
    sigemptyset(&set);

    // ���β�ϣ�����յ��ź�
    sigaddset(&set, SIGCHLD);     // �ӽ���״̬�ı�
    sigaddset(&set, SIGALRM);     // ��ʱ����ʱ
    sigaddset(&set, SIGIO);       // �첽I/O
    sigaddset(&set, SIGINT);      // �ն��жϷ�
    sigaddset(&set, SIGHUP);      // ���ӶϿ�
    sigaddset(&set, SIGUSR1);     // �û������ź�
    sigaddset(&set, SIGUSR2);     // �û������ź�
    sigaddset(&set, SIGWINCH);    // �ն˴��ڴ�С�ı�
    sigaddset(&set, SIGTERM);     // ��ֹ
    sigaddset(&set, SIGQUIT);     // �ն��˳���
    
    // �����ź�������
    if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
        globallogger->flog(LogLevel::ALERT, "sigprocmask()ʧ��!");
        return -1;
    }

    for (auto& sig : signals_) {
        
        
        
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(struct sigaction));

        if (sig.handler) {
            sa.sa_sigaction = sig.handler;
            sa.sa_flags = SA_SIGINFO;
        }
        else {
            sa.sa_handler = SIG_IGN;
        }

        if (sigaction(sig.signo, &sa, nullptr) == -1) {
            std::cerr << "Failed to register signal handler for " << sig.signame << std::endl;
            throw std::runtime_error("Signal registration failed");
        }
        else {
            globallogger->clog(LogLevel::NOTICE, "Signal handler for %s registered successfully", sig.signame);
            //std::cout << "Signal handler for " << sig.signame << " registered successfully" << std::endl;
        }
    }
}

/**
 * @brief ���ָ���źŲ�Ϊ�������µĴ�������
 *
 * �÷��������ָ�����źţ���������Ӧ�Ĵ�����������ź��ѱ����Σ����ᱻ��ⲢΪ�������µĴ�������
 *
 * @param signo �źű�š�
 * @param handler �źŴ�������
 */
void MSignal::unmask_and_set_handler(int signo, SignalHandlerFunction handler)
{
    // ����ź�
    sigset_t current_set;
    sigemptyset(&current_set);
    sigaddset(&current_set, signo);  // �����Ŀ���ź�
    if (sigprocmask(SIG_UNBLOCK, &current_set, nullptr) == -1) {
        globallogger->flog(LogLevel::ALERT, "sigprocmask()����ź�ʧ��!");
    }

    // ���ø��źŵĴ�����
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signo, &sa, nullptr) == -1) {
        globallogger->flog(LogLevel::ALERT, "sigaction()�����źŴ���ʧ��: %d", signo);
    }
}

/**
 * @brief Ĭ�ϵ��źŴ�������
 *
 * �ú������ڽ��յ��ź�ʱ�����á�����������յ����źű�ţ�����ʾ�����źŵĽ��� ID������У���
 *
 * @param signo �źű�š�
 * @param siginfo �ź���Ϣ��
 * @param ucontext ������ָ�롣
 */
void MSignal::defaultSignalHandler(int signo, siginfo_t* siginfo, void* ucontext)
{
    //delete globallogger;
    std::cout << "Received signal: " << signo << std::endl;
    if (siginfo) {
        std::cout << "Sent by PID: " << siginfo->si_pid << std::endl;
    }
}

/**
 * @brief �����ӽ����˳����źŴ�������`SIGCHLD`����
 *
 * �ú����ᴦ�� `SIGCHLD` �źţ��������˳����ӽ��̲���ʾ�ӽ��̵��˳�״̬��
 *
 * @param signo �źű�š�
 * @param siginfo �ź���Ϣ��
 * @param ucontext ������ָ�롣
 */
void MSignal::reapChildProcess(int signo, siginfo_t* siginfo, void* ucontext)
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
 * @brief ���ź��б������һ���źż��䴦������
 *
 * �÷������źű�š��ź����ƺ��źŴ�������Ϊ��������ӵ��ź��б� `signals_` �С�
 *
 * @param signo �źű�š�
 * @param signame �ź����ơ�
 * @param handler �źŴ�������
 */
void MSignal::addSignal(int signo, const std::string& signame, SignalHandlerFunction handler)
{
    signals_.emplace_back(SignalInfo{ signo, signame, handler });
}
