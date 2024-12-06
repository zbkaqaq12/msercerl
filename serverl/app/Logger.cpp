#include "Logger.h"
#include"global.h"

//��̬��Ա��ֵ
Logger* Logger::m_instance = nullptr;

/**
 * @brief ��ʼ����־ϵͳ�������ļ�����־�������־�ļ��ȡ�
 *
 * ��������Ƿ��ѳ�ʼ����������־�ļ�·������־���𣬴�������̨���ļ���־��������
 *
 * @return true �����ʼ���ɹ���
 * @return false �����ʼ��ʧ�ܣ����޷�����־�ļ�����
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
 * @brief ��¼��־����־�ļ���
 *
 * ������־����͸�ʽ������־��Ϣ������־��Ϣд���ļ���
 *
 * @param level ��־����
 * @param fmt ��ʽ���ַ�����
 * @param ... �ɱ��������־���ݡ�
 */
void Logger::flog(LogLevel level, const char* fmt, ...)
{
    if (level > logLevel_) return;
    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);

    if (fileHandler_) {
        fileHandler_->handleLog(level, message);  // ���͵��ļ�������
    }
}

/**
 * @brief ��¼��־������̨��
 *
 * ������־����͸�ʽ������־��Ϣ������־��Ϣ���������̨��
 *
 * @param level ��־����
 * @param fmt ��ʽ���ַ�����
 * @param ... �ɱ��������־���ݡ�
 */
void Logger::clog(LogLevel level, const char* fmt, ...)
{
    if (level > logLevel_) return;
    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);

    if (consoleHandler_) {
        consoleHandler_->handleLog(level, message);  // ���͵�����̨������
    }
}

//void Logger::log(LogLevel level, const char* fmt, ...)
//{
//    if (level < logLevel_) return;
//
//    // ��ȡ��ǰʱ��
//    auto now = std::chrono::system_clock::now();
//    auto time = std::chrono::system_clock::to_time_t(now);
//    std::tm tm;
//    localtime_r(&time, &tm);
//
//    // ��ʽ��ʱ��
//    std::ostringstream oss;
//    oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");
//
//    // ������־��Ϣ
//    std::ostringstream logMessage;
//    logMessage << oss.str() << " [" << logLevelToString(level) << "] ";
//
//    // ����ɱ����
//    va_list args;
//    va_start(args, fmt);
//    logMessage << format(fmt, args);
//    va_end(args);
//
//    // ������־���еȴ��첽����
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
//            // д���ļ�
//            std::ofstream logFile(logFile_, std::ios::app);
//            if (logFile.is_open()) {
//                logFile << message << std::endl;
//            }
//
//            lock.lock();
//        }
//    }
//}
