#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <cstdarg>
#include <functional>
#include <ctime>
#include <filesystem>

/**
 * @sa LogLevel
 * @brief ������־�Ĳ�ͬ����
 */
enum class LogLevel {
    EMERG = 0,   // ����
    ALERT,       // ����
    CRIT,        // ����
    ERROR,       // ����
    WARN,        // ����
    NOTICE,      // ע��
    INFO,        // ��Ϣ
    DEBUG        // ����
};

/**
 * @brief �� LogLevel ת��Ϊ��Ӧ���ַ�����ʾ��
 *
 * @param level ��־����
 * @return const char* ��Ӧ��־������ַ�����ʾ��
 */
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::EMERG: return "emerg";
    case LogLevel::ALERT: return "alert";
    case LogLevel::CRIT: return "crit";
    case LogLevel::ERROR: return "error";
    case LogLevel::WARN: return "warn";
    case LogLevel::NOTICE: return "notice";
    case LogLevel::INFO: return "info";
    case LogLevel::DEBUG: return "debug";
    default: return "unknown";
    }
}

/**
 * @sa LogHandler
 * @brief ��־�������Ļ��࣬���ڴ���ͬ���͵���־�����
 *
 * �ṩһ�������Ľӿڣ�����ͬ����־������ʵ�֡�
 */
class LogHandler{
public:
    virtual ~LogHandler() = default;
    virtual void handleLog(LogLevel, const std::string& message) = 0;
    virtual void stoplog() {};
};

/**
 * @sa ConsoleLogHandler
 * @brief ����̨��־�������������־����׼����̨��
 */
class ConsoleLogHandler :public LogHandler {
public:
    /**
     * @brief ������־��Ϣ�����������̨��
     *
     * @param level ��־����
     * @param message ��־��Ϣ��
     */
    void handleLog(LogLevel level, const std::string& message) override
    {
        // ��ȡ��ǰʱ��
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_r(&time, &tm);

        // ��ʽ��ʱ��
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");

        // �����־��Ϣ
        std::cout << oss.str() << " [" << logLevelToString(level) << "] " << message << std::endl;
    }

    void stoplog() override {}
};

/**
 * @sa FileLogHandler
 * @brief �ļ���־�������������־��ָ������־�ļ���
 */
class FileLogHandler :public LogHandler {
public:
    /**
     * @brief ���캯������ʼ���ļ���־�������������첽��־д���̡߳�
     *
     * @param fileName ��־�ļ����ļ�����
     */
    explicit FileLogHandler(const std::string& fileName)
        : logFile_(fileName, std::ios::app), stopLogging_(false)
    {
        // �����첽д���߳�
        logThread_ = std::thread(&FileLogHandler::processLogQueue, this);
    }

    /**
     * @brief ֹͣ��־���������첽��־д���̡߳�
     */
    void stoplog()
    {
        stopLogging_ = true;
        // Push a special stop message to the queue
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            logQueue_.push({ LogLevel::INFO, "" });  // Empty string as a stop signal
        }
        cv_.notify_all();
        if (logThread_.joinable()) {
            logThread_.join();
        }
    }

    ~FileLogHandler() {
        stoplog();
    }

    /**
     * @brief ������־��Ϣ���������־�ļ���
     *
     * @param level ��־����
     * @param message ��־��Ϣ��
     */
    void handleLog(LogLevel level, const std::string& message) override {
        // ����־��Ϣ������еȴ��첽����
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            logQueue_.push({ level, message });
        }
        cv_.notify_all();
    }

private:
    /**
    * @brief �첽������־���У�д����־�ļ���
    */
    void processLogQueue() {
        while (!stopLogging_) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this] { return !logQueue_.empty() || stopLogging_; });

            while (!logQueue_.empty()) {
                auto logItem = logQueue_.front();
                logQueue_.pop();
                lock.unlock();

                if (logItem.second.empty()) {
                    break;  // Stop signal
                }

                LogLevel level = logItem.first;
                const std::string& message = logItem.second;

                // ��ȡ��ǰʱ��
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                std::tm tm;
                localtime_r(&time, &tm);

                // ��ʽ��ʱ��
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");

                // �����־��Ϣ���ļ�
                if (logFile_.is_open()) {
                    logFile_ << oss.str() << " [" << logLevelToString(level) << "] " << message << std::endl;
                }

                lock.lock();
            }
        }

        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    std::ofstream logFile_; /**< ��־�ļ������ */
    std::queue<std::pair<LogLevel, std::string>> logQueue_; /**< ��־���� */
    std::mutex queueMutex_; /**< ����ͬ����־���еĻ����� */
    std::condition_variable cv_; /**< ����ͬ����־���е��������� */
    std::atomic<bool> stopLogging_; /**< ������־����ֹͣ�ı�־ */
    std::thread logThread_; /**< �첽��־�����߳� */
};


/**
 * @sa Logger
 * @brief ��־���������ṩ��־��¼�Ĺ��ܡ�
 *
 * �ṩ����̨��־���ļ���־�Ĵ����ܣ���֧�ֶ��߳��첽����
 */
class Logger {
private:
    Logger()
    {
        
    }

public:

    ~Logger() {
    }

    static Logger* GetInstance()
    {
        if (m_instance == nullptr)
        {
            if (m_instance == nullptr)
            {
                m_instance = new Logger();
                static Garhuishou c1;
            }
        }
        return m_instance;
    }

    // ɾ�����ƹ��캯���͸�ֵ������
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    class Garhuishou  //�������࣬�����ͷŶ���
    {
    public:
        ~Garhuishou()
        {
            if (Logger::m_instance)
            {
                delete Logger::m_instance;
                Logger::m_instance = nullptr;
            }
        }
    };

    bool log_init();

    // ��ȡ��־����
    /*LogLevel getLogLevel() const {
        return logLevel_;
    }*/
    
    // ��¼��־
    void flog(LogLevel level, const char* fmt, ...);  //��־�ļ�ģʽ
    void clog(LogLevel level, const char* fmt, ...);   //����̨ģʽ

    //ֹͣ�߳��첽��־����
    void stoplogprocess()
    {
        fileHandler_->stoplog();
    }
    

private:
    // �첽��־�����߳�
    //void processLogQueue();

    // ��ʽ���ɱ����
    std::string format(const char* fmt, va_list args) {
        int size = std::vsnprintf(nullptr, 0, fmt, args) + 1;
        std::unique_ptr<char[]> buffer(new char[size]);
        std::vsnprintf(buffer.get(), size, fmt, args);
        return std::string(buffer.get());
    }

    static Logger* m_instance; /**< Logger ʵ�� */
    std::string logFile_; /**< ��־�ļ��� */
    LogLevel logLevel_; /**< ��־���� */
    std::shared_ptr<LogHandler> consoleHandler_; /**< ����̨��־������ */
    std::shared_ptr<LogHandler> fileHandler_; /**< �ļ���־������ */
    
};


