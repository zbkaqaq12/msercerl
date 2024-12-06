#include "MasterProcess.h"
#include <unistd.h>
#include<sys/types.h>

/**
* @brief ���������̣���ʼ���źŴ������������������̡�
*
* 1. ��ʼ���źŴ������������źŴ�������
* 2. ���������̱��⣻
* 3. �������ļ����ع�������������������Ӧ�����Ĺ������̣�
* 4. ����ȴ��źŵ�״̬��
*/
void MasterProcess::start()
{
    // ��ʼ���źŴ�����
    if (signalHandler_->init_signal() == -1) {
        globallogger->clog(LogLevel::ALERT, "�źų�ʼ��ʧ��!");
        return;
    }

    // �����������Ҫ������ź�
    //signalHandler_->unmask_and_set_handler(SIGTERM, handleSIGTERM);
    //signalHandler_->unmask_and_set_handler(SIGINT, handleSIGINT);
    // �����������Ҫ������ź�
    signalHandler_->unmask_and_set_handler(SIGTERM, handleSIGTERM);
    signalHandler_->unmask_and_set_handler(SIGINT, handleSIGINT);
    signalHandler_->unmask_and_set_handler(SIGCHLD, handleSIGCHLD);
    signalHandler_->unmask_and_set_handler(SIGHUP, handleSIGHUP);

    // ���������̱���
    set_process_title();

    // �������ļ��ж�ȡ������������
    int worker_count = globalconfig->GetIntDefault("WorkerProcesses", 1);;
    start_worker_processes(worker_count);

    // �ȴ��ź�����
    wait_for_signal();
}

/**
* @brief ���������̱��⡣
*
* ���������̵Ľ��̱��⣬ʹ����߿ɶ��ԺͿɹ����ԡ���������������������в�����
*/
void MasterProcess::set_process_title()
{
    // ���������̱���
    size_t size;
    int    i;
    size = sizeof(processName_);  //ע���������õ���sizeof�������ַ���ĩβ��\0�Ǳ���������˵�
    size += g_argvneedmem;          //argv�������ȼӽ���    
    if (size < 1000) //����С��������Ҳ����ñ���
    {
        char title[1000] = { 0 };
        strcpy(title, processName_.c_str()); //"master process"
        strcat(title, " ");  //��һ���ո�ֿ�һЩ������    //"master process "
        for (i = 0; i < g_os_argc; i++)         //"master process ./nginx"
        {
            strcat(title, g_os_argv[i]);
        }//end for
        setproctitle(title); //���ñ���
    }
}

/**
* @brief ����������һ���µĹ������̡�
*
* @param num �������̱�ţ��������ֲ�ͬ�Ĺ������̡�
*/
void MasterProcess::spawn_worker_process(int num)
{
    pid_t pid = fork();
    if (pid == -1) {
        globallogger->flog(LogLevel::ALERT, "fork()�����ӽ���ʧ��!");
        return;
    }

    if (pid == 0) {
        // �ӽ��̴���
        try {
            // ���� WorkerProcess ����
            std::unique_ptr<WorkerProcess> worker = std::make_unique<WorkerProcess>();
            // ȷ�� worker ��Ϊ nullptr
            if (worker) {
                worker->start();  // ������������
            }
            else {
                std::cerr << "Failed to create WorkerProcess instance." << std::endl;
                exit(1);
            }

        }
        catch (const std::exception& e) {
            std::cerr << "Exception in child process: " << e.what() << std::endl;
            exit(1);  // �����쳣ʱ�˳��ӽ���
        }

        exit(0);  // �ӽ����˳�
    }
    // �����̼���ִ��
}

/**
 * @brief ���� SIGTERM �źš�
 *
 * ���������յ� SIGTERM �ź�ʱ��ִ�����������ͨ������ƽ����ֹ���̡�
 * �ڸú����У������ֹͣ���й������̣��ͷ���Դ�ȡ�
 *
 * @param signo �źű�ţ���ʾ���յ����ź����ͣ���ʱ�� SIGTERM����
 * @param siginfo �ź���Ϣ�����������źŵĸ�����Ϣ��
 * @param ucontext ��������Ϣ�����������źŴ���������ģ���Ĵ���״̬�ȣ���
 */
void MasterProcess::handleSIGTERM(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "���� SIGTERM �źţ������������");
    // ������ִ�������������ֹͣ�������̵�
    // �����ӽ��̡���־�������Դ
}

/**
 * @brief ���� SIGINT �źš�
 *
 * ���������յ� SIGINT �ź�ʱ��ֹͣ��־����������Դ�ͷŲ�����ͨ������ͨ�������жϣ�Ctrl+C�������ġ�
 * �ڸú����У�����ֹͣ���й������̡�������Դ��������־�ȡ�
 *
 * @param signo �źű�ţ���ʾ���յ����ź����ͣ���ʱ�� SIGINT����
 * @param siginfo �ź���Ϣ�����������źŵĸ�����Ϣ��
 * @param ucontext ��������Ϣ�����������źŴ���������ģ���Ĵ���״̬�ȣ���
 */
void MasterProcess::handleSIGINT(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->stoplogprocess();
    globallogger->clog(LogLevel::INFO, "���� SIGINT �źţ������������");
    // �ж��źź�ִ����Դ�ͷź���ֹ����
    // ֹͣ���й������̡�������Դ��
}

/**
 * @brief ���� SIGCHLD �źš�
 *
 * ���ӽ��̽���ʱ�������̻���յ� SIGCHLD �źš��ú����ᴦ���ӽ��̵��˳�״̬����ֹ���ֽ�ʬ���̡�
 * ������� `waitpid` �������ӽ���״̬��������˳���Ϣ��
 *
 * @param signo �źű�ţ���ʾ���յ����ź����ͣ���ʱ�� SIGCHLD����
 * @param siginfo �ź���Ϣ�����������źŵĸ�����Ϣ��
 * @param ucontext ��������Ϣ�����������źŴ���������ģ���Ĵ���״̬�ȣ���
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
 * @brief ���� SIGHUP �źš�
 *
 * �����̽��յ� SIGHUP �ź�ʱ��ͨ����ζ����Ҫ���¼��������ļ�����ִ�������ָ�������
 * �����ڸú����м������¼������õ��߼������߽���������Ҫ�Ļָ�������
 *
 * @param signo �źű�ţ���ʾ���յ����ź����ͣ���ʱ�� SIGHUP����
 * @param siginfo �ź���Ϣ�����������źŵĸ�����Ϣ��
 * @param ucontext ��������Ϣ�����������źŴ���������ģ���Ĵ���״̬�ȣ���
 */
void MasterProcess::handleSIGHUP(int signo, siginfo_t* siginfo, void* ucontext)
{
    globallogger->clog(LogLevel::INFO, "���յ� SIGHUP �źţ����¼�������");
    // �������������¼��������ļ���ִ�������ָ�����
}

