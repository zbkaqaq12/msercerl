#include "Logger.h"
#include"global.h"

//静态成员赋值
Logger* Logger::m_instance = nullptr;

/**
 * @brief 初始化日志系统，配置文件、日志级别和日志文件等。
 *
 * 检查配置是否已初始化，设置日志文件路径和日志级别，创建控制台和文件日志处理器。
 *
 * @return true 如果初始化成功。
 * @return false 如果初始化失败（如无法打开日志文件）。
 */
bool Logger::log_init()
{
    if (globalconfig == nullptr) {
            throw std::runtime_error("Config is not initialized");
            return false;
        }
    
    logFile_ = globalconfig->GetString("LogFile");
    logLevel_ = static_cast<LogLevel>(globalconfig->GetIntDefault("LogLevel", -1));

    std::ofstream logFileStream(logFile_, std::ios::app);  // Open the log file in append mode
    if (!logFileStream.is_open()) {
        // If the file cannot be opened, return false
        return false;
    }
    logFileStream.close();  // Close the file immediately after check
    consoleHandler_ = std::make_shared<ConsoleLogHandler>();
    fileHandler_ = std::make_shared<FileLogHandler>(logFile_);
    if (fileHandler_ == nullptr)
    {
        return false;
    }
    //logThread_ = std::thread([this] { this->processLogQueue(); });
    return true;
}

/**
 * @brief 记录日志到日志文件。
 *
 * 根据日志级别和格式化的日志信息，将日志消息写入文件。
 *
 * @param level 日志级别。
 * @param fmt 格式化字符串。
 * @param ... 可变参数，日志内容。
 */
void Logger::flog(LogLevel level, const char* fmt, ...)
{
    if (level > logLevel_) return;
    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);

    if (fileHandler_) {
        fileHandler_->handleLog(level, message);  // 发送到文件处理器
    }
}

/**
 * @brief 记录日志到控制台。
 *
 * 根据日志级别和格式化的日志信息，将日志消息输出到控制台。
 *
 * @param level 日志级别。
 * @param fmt 格式化字符串。
 * @param ... 可变参数，日志内容。
 */
void Logger::clog(LogLevel level, const char* fmt, ...)
{
    if (level > logLevel_) return;
    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);

    if (consoleHandler_) {
        consoleHandler_->handleLog(level, message);  // 发送到控制台处理器
    }
}

//void Logger::log(LogLevel level, const char* fmt, ...)
//{
//    if (level < logLevel_) return;
//
//    // 获取当前时间
//    auto now = std::chrono::system_clock::now();
//    auto time = std::chrono::system_clock::to_time_t(now);
//    std::tm tm;
//    localtime_r(&time, &tm);
//
//    // 格式化时间
//    std::ostringstream oss;
//    oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");
//
//    // 构造日志消息
//    std::ostringstream logMessage;
//    logMessage << oss.str() << " [" << logLevelToString(level) << "] ";
//
//    // 处理可变参数
//    va_list args;
//    va_start(args, fmt);
//    logMessage << format(fmt, args);
//    va_end(args);
//
//    // 放入日志队列等待异步处理
//    std::lock_guard<std::mutex> lock(queueMutex_);
//    logQueue_.push(logMessage.str());
//    cv_.notify_all();
//}
//
//void Logger::stoplogprocess()
//{
//    
//    stopLogging_ = true;
//    cv_.notify_all();
//    if (logThread_.joinable()) {
//        logThread_.join();
//    }
//    
//}
//
//void Logger::processLogQueue()
//{
//    while (!stopLogging_) {
//        std::unique_lock<std::mutex> lock(queueMutex_);
//        cv_.wait(lock, [this] { return !logQueue_.empty() || stopLogging_; });
//
//        while (!logQueue_.empty()) {
//            std::string message = logQueue_.front();
//            logQueue_.pop();
//            lock.unlock();
//
//            // 写入文件
//            std::ofstream logFile(logFile_, std::ios::app);
//            if (logFile.is_open()) {
//                logFile << message << std::endl;
//            }
//
//            lock.lock();
//        }
//    }
//}
