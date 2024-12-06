#pragma once
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <memory>
#include "global.h"

/**
 * @class Daemon
 * @brief 守护进程类，用于将进程转为守护进程。
 * 
 * 该类通过 `fork()`、`setsid()` 和重定向标准输入输出到 `/dev/null` 来实现将进程转为守护进程。
 */
class Daemon {
public:
    /**
     * @brief 构造函数，初始化守护进程信息。
     * 
     * @param pid 子进程的 PID。
     * @param ppid 父进程的 PID。
     */
    Daemon(pid_t pid, pid_t ppid)
        : pid_(pid), ppid_(ppid), fd_(-1) {}

    /**
     * @brief 初始化守护进程，包括创建子进程、脱离控制终端和重定向流。
     * 
     * 创建子进程并脱离控制终端，成功后将标准输入、输出和错误重定向到 `/dev/null`。
     * 
     * @return 返回 1 表示父进程，返回 0 表示子进程，返回 -1 表示初始化失败。
     */
    int init() {
        // 第一次 fork() 创建子进程
        pid_t pid = fork();
        if (pid == -1) {
            logError("fork()失败");
            return -1;  // 如果 fork() 失败，返回错误
        }

        if (pid > 0) {
            return 1;  // 父进程直接退出
        }

        // 子进程
        if (setsid() == -1) {
            globallogger->flog(LogLevel::ERROR, "setsid()失败");
            return -1;  // 如果 setsid() 失败，返回错误
        }

        umask(0);  // 设置文件创建掩码为0，确保文件权限不受限制

        // 打开 /dev/null 并重定向标准流
        if (!openNullDevice()) {
            return -1;  // 如果打开 /dev/null 失败，返回错误
        }

        return 0;  // 子进程成功，继续执行
    }

private:
    int fd_;  ///< /dev/null 的文件描述符
    pid_t pid_;  ///< 子进程 PID
    pid_t ppid_;  ///< 父进程 PID

    /**
     * @brief 错误日志输出函数。
     * 
     * @param message 错误信息。
     */
    void logError(const std::string& message) const {
        globallogger->flog(LogLevel::EMERG, message.c_str());
    }

    /**
     * @brief 打开 `/dev/null` 设备并重定向标准输入、标准输出、标准错误流。
     * 
     * @return 如果成功返回 true，否则返回 false。
     */
    bool openNullDevice() {
        fd_ = open("/dev/null", O_RDWR);
        if (fd_ == -1) {
            globallogger->flog(LogLevel::ERROR, "open(\"/dev/null\")失败");
            return false;
        }

        // 将标准输入、标准输出和标准错误重定向到 /dev/null
        if (!redirectStream(STDIN_FILENO) || !redirectStream(STDOUT_FILENO) || !redirectStream(STDERR_FILENO)) {
            return false;
        }

        return true;
    }

    /**
     * @brief 将标准流重定向到 `/dev/null`。
     * 
     * @param stream 要重定向的流，可以是 `STDIN_FILENO`、`STDOUT_FILENO` 或 `STDERR_FILENO`。
     * 
     * @return 如果成功返回 true，否则返回 false。
     */
    bool redirectStream(int stream) {
        if (dup2(fd_, stream) == -1) {
            globallogger->flog(LogLevel::ERROR, "dup2()失败，流：%s", std::to_string(stream));
            return false;
        }
        return true;
    }
};
