# C++ Linux 服务器架构

## 项目目标
实现一个高效的多线程服务器，支持并发处理客户端请求。
使用C++11及更高版本的现代C++特性，以提高代码的可维护性和执行效率。
支持TCP/UDP协议，能够处理多种类型的客户端连接。
提供简单的API接口，方便与其他系统集成。
保障服务器的高可用性和稳定性，通过错误处理、日志记录、性能监控等机制避免服务中断。
## 关键技术
C++11/14/17特性：包括智能指针、线程管理、STL容器等现代C++特性，提高代码的安全性和效率。
多线程编程：使用标准库的<thread>、<mutex>、<condition_variable>等工具，处理多个客户端的并发请求。
网络编程：基于POSIX套接字接口（如<sys/socket.h>和<netinet/in.h>），支持TCP和UDP协议。
I/O多路复用：使用select或epoll等机制，高效地管理大量客户端连接。
日志系统：记录运行时信息、错误和性能数据。
## 主要功能
客户端连接管理：支持TCP/UDP协议的客户端连接，包括连接建立、数据接收和发送、连接关闭等操作。
数据处理：接收客户端数据，进行解析、处理，并返回相应的结果。
多线程处理：通过线程池管理并发连接，确保每个请求都能够得到及时处理，提升系统的吞吐量和响应速度。
动态配置：支持配置文件，方便调整服务器的行为和参数，如最大连接数、超时设置、日志级别等。

## 功能特性简介
### 配置文件读取
该项目配置文件读取采用了xml格式的文件，将tinyxml2库上相关功能封装为CConfig类来负责配置文件加载，读取功能，CConfig设计为单例类，保证整体只能创建一个类，以全局变量形式供各个模块使用
### 日志文件打印
日志文件以异步线程和控制台的方式来打印输出，格式为 时间+日志等级+内容，单例模式+全局变量的形式供其他模块使用
### 进程设计
该项目进程分为master进程和worker进程，master进程管理多个worker进程，worker进程负责主要网路通信已经业务逻辑模块，两个进程封装为了不同的两个类及MasterProcess类和WorkerProcess类
### 信号处理
将linux中信号处理封装为MSignal类，屏蔽不想接收的信号，制定信号处理函数，该类还提供unmask_and_set_handler()函数用来对单一的已屏蔽的信号进行解封，并指向一个处理函数，这个方法在master和worker分别处理他们想要的信号
### 网络模块
#### 初始化
在主进程中Initialize初始化绑定并打开监听端口。
在子进程中
1、Initialize_subproc()初始化线程（发数据，连接回收，连接监控）、信号量
2、epoll_init初始化epoll功能，创建initconnection()连接池，将每一个socket和一个内存相互绑定，方便记录socket的数据和状态，epoll_oper_event执行 epoll 操作（增加、修改或删除事件）
3、调用epoll_process_events处理epoll事件（读，写）
#### 读事件流程
新链接 epoll_process_events--->(this->* (p_Conn->rhandler))(p_Conn)--->event_accept
旧链接 epoll_process_events--->(this->* (p_Conn->rhandler))(p_Conn)--->read_request_handler--->wait_request_handler_proc_p1--->wait_request_handler_proc_plast-->inMsgRecvQueueAndSignal
#### 写事件流程
msgSend--->m_MsgSendQueue.push_back(psendbuf)--->sem_post(&m_semEventSendQueue)--->sendproc
### 业务逻辑处理模块
线程池ThreadPool在worker进程初始化，当有消息发送时会唤醒其中一个线程来处理。CLogicSocket类继承CSocket类，其中增加业务处理函数_HandleLogIn等。
### 心跳包
_HandlePing，客户端服务器约定每20秒发送一个心跳包，服务器会在这个约定时间3倍等待客户端发送，如果没有接收到则关闭socket连接，客户端如果没收到服务器的心跳包则尝试主动连接服务器




