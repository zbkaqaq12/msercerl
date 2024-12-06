#pragma once
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include"global.h"
#include"func.h"
#include"MSignal.h"
#include <sys/prctl.h>

/**
 * @brief ���������̵��ࡣ
 *
 * �����ʾһ���������̣������ʼ���������źš�ִ������ȡ��������̻�̳и����̵��źŴ�����ƣ�����Ӧ�ض����źţ�����ֹ�źš��û������źŵȣ���
 */
class WorkerProcess
{
public:
    /**
     * @brief ���캯������ʼ�� WorkerProcess ����
     *
     * @param processName �������ƣ�Ĭ��Ϊ "workerserverl"��
     */
    WorkerProcess( std::string processName = "workerserverl")
        :processName_(processName), signalHandler_(std::make_unique<MSignal>())
    {
    }

    /**
     * @brief �����������̡�
     *
     * ��ʼ�����̲���ʼ���С��˺��������ʼ�����̻����������� `run()` �����¼�ѭ����
     */
    void start() {
        // �ӽ��̵ĳ�ʼ������
        init_process();
        globallogger->clog(LogLevel::INFO, "Worker process init complete...");
        run();
    }
    
private:
private:
    std::string processName_;  ///< ��������
    std::unique_ptr<MSignal> signalHandler_;  ///< �źŴ���������

    void init_process();

    void set_process_title();

    void run();

    // ��̬�źŴ���������
    static void handleSIGTERM(int signo, siginfo_t* siginfo, void* ucontext);
    static void handleSIGINT(int signo, siginfo_t* siginfo, void* ucontext);
    static void handleSIGHUP(int signo, siginfo_t* siginfo, void* ucontext);
    static void handleSIGUSR1(int signo, siginfo_t* siginfo, void* ucontext);
    static void handleSIGUSR2(int signo, siginfo_t* siginfo, void* ucontext);

    
};

