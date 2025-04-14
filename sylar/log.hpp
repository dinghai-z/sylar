#ifndef _SYLAR_LOG_H_
#define _SYLAR_LOG_H_

#include <sstream>
#include <fstream>
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <iostream>
#include <ctime>
#include <functional>
#include <unordered_map>
#include <map>
#include "util.hpp"
#include "singleton.hpp"
#include "mutex.hpp"
#include "thread.hpp"

#define SYLAR_LOG_LEVEL(logger, level) \
    if(level >= logger->getLevel()) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, 0, time(0), __FILE__, __LINE__, \
                            sylar::GetThreadId(), sylar::Thread::GetName(), sylar::GetFiberId()))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar{

class Logger;
class LogFormatter;
class LoggerManager;

/**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details 
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */

class LogLevel{

public:
    enum Level{
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };

static std::string toString(LogLevel::Level level);

};

class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    /**
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %t 线程id
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %F 协程id
     *  %N 线程名称
     */
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, uint32_t elapse
            ,uint64_t time, const std::string &fileName, uint32_t line
            ,uint32_t threadId, const std::string &threadName, uint32_t fiberId);
    std::shared_ptr<Logger> getLogger(){return m_logger;};
    LogLevel::Level getLevel(){return m_level;}
    uint32_t getElapse(){return m_elapse;}
    uint32_t getThreadId(){return m_threadId;}
    uint32_t getFiberId(){return m_fiberId;}
    uint64_t getTime(){return m_time;}
    std::string getFileName(){return m_fileName;}
    uint32_t getLine(){return m_line;}
    std::string getThreadName(){return m_threadName;}
    std::string getContent(){return m_ss.str();}
    std::stringstream& getSS(){return m_ss;}
private:
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
    uint32_t m_elapse;
    uint64_t m_time;
    std::string m_fileName;
    uint32_t m_line;
    uint32_t m_threadId;
    std::string m_threadName;
    uint32_t m_fiberId;
    std::stringstream m_ss;
};

class LogEventWrap{
public:
    LogEventWrap(LogEvent::ptr);
    ~LogEventWrap();
    std::stringstream& getSS();
private:
    LogEvent::ptr m_event;
};

class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string &pattern);
    void setPattern(const std::string &pattern);
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };
    
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    void init();
};

class LogAppender{
public:
    typedef SpinLock MutexType;
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {}
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    void setLevel(LogLevel::Level level);
    LogLevel::Level getLevel(){return m_level;}
    void setFormatter(LogFormatter::ptr formatter);
    LogFormatter::ptr getFormatter(){return m_formatter;}
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
    MutexType m_mutex;
};

class Logger : public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    Logger(const std::string &name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr );
    void clearAppenders();
    void setName(const std::string &name);
    std::string getName();
    void setLevel(LogLevel::Level level);
    LogLevel::Level getLevel(){return m_level;}

private:
    std::string m_name;
    LogLevel::Level m_level;
    std::list<LogAppender::ptr> m_appenders;
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

class StdOutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdOutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
private:
    static MutexType m_mutex;
};

class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string &fileName);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
private:
    std::string m_ofileName;
    std::ofstream m_ofileStream;
};

class LoggerManager{
public:
    LoggerManager();
    Logger::ptr getRoot(){return m_root;};
    Logger::ptr getLogger(const std::string &name);
private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef Singleton<LoggerManager> LoggerMgr;

}

#endif