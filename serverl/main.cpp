#include"MSignal.h"
#include"Daemon.h"
#include <sys/prctl.h>
#include <cstring>
#include<unistd.h>
#include <iostream>
#include <MasterProcess.h>
#include"func.h"


static void freeresource();

//和设置标题有关的全局量
size_t  g_argvneedmem = 0;        //保存下这些argv参数所需要的内存大小
size_t  g_envneedmem = 0;         //环境变量所占内存大小
int     g_os_argc;              //参数个数 
char** g_os_argv;            //原始命令行参数数组,在main中会被赋值
char* gp_envmem = NULL;        //指向自己分配的env环境变量的内存，在ngx_init_setproctitle()函数中会被分配内存
int     g_daemonized = 0;         //守护进程标记，标记是否启用了守护进程模式，0：未启用，1：启用了


//socket相关
CLogicSocket g_socket;               //socket全局对象
CThreadPool   g_threadpool;

//和进程本身有关的全局量
pid_t   severl_pid;                //当前进程的pid
pid_t   severl_parent;             //父进程的pid
int     severl_process;            //进程类型，比如master,worker进程等
int     g_stopEvent;            //标志程序退出,0不退出1，退出


Logger* globallogger = Logger::GetInstance();
Config* globalconfig = Config::GetInstance();


int main(int argc, char* const* argv) {
    
    //
    int exitcode = 0;  // 退出代码，先给0表示正常退出
    int i;              //临时用

    //(0)先初始化的变量
    g_stopEvent = 0;            //标记程序是否退出，0不退出          

    //(1)无伤大雅也不需要释放的放最上边 
    severl_pid = getpid();
    severl_parent = getppid();

    //统计argv所占的内存
    g_argvneedmem = 0;
    for (i = 0; i < argc; i++)  //argv =  ./nginx -a -b -c asdfas
    {
        g_argvneedmem += strlen(argv[i]) + 1; //+1是给\0留空间。
    }
    //统计环境变量所占的内存。注意判断方法是environ[i]是否为空作为环境变量结束标记
    for (i = 0; environ[i]; i++)
    {
        g_envneedmem += strlen(environ[i]) + 1; //+1是因为末尾有\0,是占实际内存位置的，要算进来
    } //end for

    g_os_argc = argc;           //保存参数个数
    g_os_argv = (char**)argv; //保存参数指针

    //(2)初始化失败，就要直接退出的
    
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
    



    //(3)一些必须事先准备好的资源，先初始化
    //(4)一些初始化函数，准备放这里
    if (g_socket.Initialize() == false)//初始化socket
    {
        globallogger->flog(LogLevel::ERROR, "the Socket initialize fail!");
        return -1;
        //goto lblexit;
    }

    //(5)一些不好归类的其他类别的代码，准备放这里
    init_setproctitle();    //把环境变量搬家
    
    //(6)创建守护进程
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
    
    //(7)开始正式的主工作流程，主流程一致在下边这个函数里循环，暂时不会走下来，资源释放啥的日后再慢慢完善和考虑
    MasterProcess master;
    master.start();
   
    
    

lblexit:
        //(5)该释放的资源要释放掉
        globallogger->clog(LogLevel::NOTICE, "程序退出，再见了!");
        freeresource();  //一系列的main返回前的释放动作函数    
        return exitcode;

}

//专门在程序执行末尾释放资源的函数【一系列的main返回前的释放动作函数】
void freeresource()
{
    //(1)对于因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if (gp_envmem)
    {
        delete[]gp_envmem;
        gp_envmem = NULL;
    }

    //(2)关闭日志文件
    globallogger->stoplogprocess();
}
