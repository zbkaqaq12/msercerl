#include"MSignal.h"
#include"Daemon.h"
#include <sys/prctl.h>
#include <cstring>
#include<unistd.h>
#include <iostream>
#include <MasterProcess.h>
#include"func.h"


static void freeresource();

//�����ñ����йص�ȫ����
size_t  g_argvneedmem = 0;        //��������Щargv��������Ҫ���ڴ��С
size_t  g_envneedmem = 0;         //����������ռ�ڴ��С
int     g_os_argc;              //�������� 
char** g_os_argv;            //ԭʼ�����в�������,��main�лᱻ��ֵ
char* gp_envmem = NULL;        //ָ���Լ������env�����������ڴ棬��ngx_init_setproctitle()�����лᱻ�����ڴ�
int     g_daemonized = 0;         //�ػ����̱�ǣ�����Ƿ��������ػ�����ģʽ��0��δ���ã�1��������


//socket���
CLogicSocket g_socket;               //socketȫ�ֶ���
CThreadPool   g_threadpool;

//�ͽ��̱����йص�ȫ����
pid_t   severl_pid;                //��ǰ���̵�pid
pid_t   severl_parent;             //�����̵�pid
int     severl_process;            //�������ͣ�����master,worker���̵�
int     g_stopEvent;            //��־�����˳�,0���˳�1���˳�


Logger* globallogger = Logger::GetInstance();
Config* globalconfig = Config::GetInstance();


int main(int argc, char* const* argv) {
    
    //
    int exitcode = 0;  // �˳����룬�ȸ�0��ʾ�����˳�
    int i;              //��ʱ��

    //(0)�ȳ�ʼ���ı���
    g_stopEvent = 0;            //��ǳ����Ƿ��˳���0���˳�          

    //(1)���˴���Ҳ����Ҫ�ͷŵķ����ϱ� 
    severl_pid = getpid();
    severl_parent = getppid();

    //ͳ��argv��ռ���ڴ�
    g_argvneedmem = 0;
    for (i = 0; i < argc; i++)  //argv =  ./nginx -a -b -c asdfas
    {
        g_argvneedmem += strlen(argv[i]) + 1; //+1�Ǹ�\0���ռ䡣
    }
    //ͳ�ƻ���������ռ���ڴ档ע���жϷ�����environ[i]�Ƿ�Ϊ����Ϊ���������������
    for (i = 0; environ[i]; i++)
    {
        g_envneedmem += strlen(environ[i]) + 1; //+1����Ϊĩβ��\0,��ռʵ���ڴ�λ�õģ�Ҫ�����
    } //end for

    g_os_argc = argc;           //�����������
    g_os_argv = (char**)argv; //�������ָ��

    //(2)��ʼ��ʧ�ܣ���Ҫֱ���˳���
    
    //globalconfig = Config::GetInstance();
    if (globalconfig->Load("Config.xml") == false)
    {
        std::cerr << "Failed to open Config" << std::endl;
        throw std::runtime_error("Config open failed");
    }
    //globallogger = Logger::GetInstance();
    if (globallogger->log_init() == false)
    {
        std::cerr << "Failed to open Logger" << std::endl;
        throw std::runtime_error("Logger open failed");
    }
    



    //(3)һЩ��������׼���õ���Դ���ȳ�ʼ��
    //(4)һЩ��ʼ��������׼��������
    if (g_socket.Initialize() == false)//��ʼ��socket
    {
        globallogger->flog(LogLevel::ERROR, "the Socket initialize fail!");
        return -1;
        //goto lblexit;
    }

    //(5)һЩ���ù�����������Ĵ��룬׼��������
    init_setproctitle();    //�ѻ����������
    
    //(6)�����ػ�����
    if (globalconfig->GetIntDefault("Daemon", -1) == 1)
    { 
        Daemon daemon( severl_pid, severl_parent);
        int result = daemon.init();
        if (result == -1) {
            globallogger->flog(LogLevel::EMERG, "Daemon build fail!");
        }
        else if (result == 1) {
            globallogger->flog(LogLevel::EMERG, "Daemon parent process tui!");
        }
    }
    
    //(7)��ʼ��ʽ�����������̣�������һ�����±����������ѭ������ʱ��������������Դ�ͷ�ɶ���պ����������ƺͿ���
    MasterProcess master;
    master.start();
   
    
    

lblexit:
        //(5)���ͷŵ���ԴҪ�ͷŵ�
        globallogger->clog(LogLevel::NOTICE, "�����˳����ټ���!");
        freeresource();  //һϵ�е�main����ǰ���ͷŶ�������    
        return exitcode;

}

//ר���ڳ���ִ��ĩβ�ͷ���Դ�ĺ�����һϵ�е�main����ǰ���ͷŶ���������
void freeresource()
{
    //(1)������Ϊ���ÿ�ִ�г�����⵼�µĻ�������������ڴ棬����Ӧ���ͷ�
    if (gp_envmem)
    {
        delete[]gp_envmem;
        gp_envmem = NULL;
    }

    //(2)�ر���־�ļ�
    globallogger->stoplogprocess();
}
