#pragma once
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include"global.h"
#include <sys/prctl.h>
#include"WorkerProcess.h"
#include"func.h"
#include"MSignal.h"

/**
 * @class MasterProcess
 * @brief �������࣬���ڹ���Ϳ��ƽ����е��ӽ��̡�
 *
 * ���ฺ���ʼ���źŴ����������ý��̱��⡢�����������̣����ڽ��յ��ź�ʱ������ز�����
 */
class MasterProcess
{
public:
    /**
     * @brief ���캯������ʼ�������̶���
     *
     * @param processName ���������ƣ�Ĭ��Ϊ "masterserverl"��
     */
    MasterProcess(std::string processName = "masterserverl")
        : processName_(processName), signalHandler_(std::make_unique<MSignal>())
    {}

    void start();
private:
    void set_process_title();

    /**
     * @brief ����ָ�������Ĺ������̡�
     *
     * @param worker_count ��������������
     */
    void start_worker_processes(int worker_count) {
        for (int i = 0; i < worker_count; ++i) {
            spawn_worker_process(i);
        }
    }

    void spawn_worker_process(int num);

    /**
     * @brief �����̵ȴ��źţ���������״̬��ֱ�����յ��źš�
     */
    void wait_for_signal() {
        // �����̵ȴ��ź�
        while (true) {
            pause();
        }
    }

    // �Զ����źŴ�����
    static void handleSIGTERM(int signo, siginfo_t* siginfo, void* ucontext);
    static void handleSIGINT(int signo, siginfo_t* siginfo, void* ucontext);
    static void handleSIGCHLD(int signo, siginfo_t* siginfo, void* ucontext);      
    static void handleSIGHUP(int signo, siginfo_t* siginfo, void* ucontext);

private:
    std::string processName_;
    std::unique_ptr<MSignal> signalHandler_;
};

