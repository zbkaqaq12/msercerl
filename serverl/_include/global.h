#pragma once
#include"Config.h"
#include"Logger.h"
#include"CLogicSocket.h"
#include"CThreadPool.h"
#include <signal.h> 
#include<string>
#include<vector>


//#include "ngx_c_slogic.h"
//#include "ngx_c_threadpool.h"

//一些比较通用的定义放在这里，比如typedef定义
//一些全局变量的外部声明也放在这里

//类型定义----------------


//外部全局量声明
extern size_t        g_argvneedmem;
extern size_t        g_envneedmem;
extern int           g_os_argc;
extern char** g_os_argv;
extern char* gp_envmem;
extern int           g_daemonized;
//extern CSocket		g_socket;
extern CLogicSocket  g_socket;
extern CThreadPool   g_threadpool;

extern pid_t         ngx_pid;
extern pid_t         ngx_parent;
extern int           ngx_process;
extern sig_atomic_t  ngx_reap;
extern int           g_stopEvent;

extern Logger* globallogger;
extern Config* globalconfig;

