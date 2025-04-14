#include "log.hpp"

namespace sylar{

LogEventWrap::LogEventWrap(LogEvent::ptr event)
    :m_event(event){
}

LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS(){
    return m_event->getSS();
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, uint32_t elapse
            ,uint64_t time, const std::string &fileName, uint32_t line
            ,uint32_t threadId, const std::string &threadName, uint32_t fiberId)
    :m_logger(logger)
    ,m_level(level)
    ,m_elapse(elapse)
    ,m_time(time)
    ,m_fileName(fileName)
    ,m_line(line)
    ,m_threadId(threadId)
    ,m_threadName(threadName)
    ,m_fiberId(fiberId){

}

LogFormatter::LogFormatter(const std::string &pattern)
    :m_pattern(pattern){
    init();
}

void LogFormatter::setPattern(const std::string &pattern){
    m_pattern = pattern;
    m_items.clear();
    init();
}

std::string LogFormatter::format(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event){
    std::stringstream ss;
    // for(auto &i : m_items){
    //     i->format(ss, logger, level, event);
    // }
    format(ss, logger, level, event);
    return ss.str();
}

void LogFormatter::format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event){
    for(auto &i : m_items){
        i->format(os, logger, level, event);
    }
}

std::string LogLevel::toString(LogLevel::Level level){
    switch(level){
        // case LogLevel::DEBUG:
        //     return "DEBUG";
        //     break;
#define XX(str)\
        case LogLevel::str:\
            return #str;\
            break;
        XX(DEBUG)
        XX(INFO)
        XX(WARN)
        XX(ERROR)
        XX(FATAL)
#undef XX
        default:
            return "UNKNOW";
    }
}

class MessageFormatItem: public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getContent();
    }
};

class LevelFormatItem: public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << LogLevel::toString(level);
    }
};

class ElapseFormatItem: public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getElapse();
    }
};

class LogNameFormatItem: public LogFormatter::FormatItem{
public:
    LogNameFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << logger->getName();
    }
};

class ThreadIdFormatItem: public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getThreadId();
    }
};

class NewLineFormatItem: public LogFormatter::FormatItem{
public:
    NewLineFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << "\n";
    }
};

class TimeFormatItem: public LogFormatter::FormatItem{
public:
    TimeFormatItem(const std::string &fmt = "%Y-%m-%d %H:%M:%S") 
        :m_fmt(fmt) {
        if(m_fmt.empty()){
            m_fmt = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_fmt.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_fmt;
};

class FileNameFormatItem: public LogFormatter::FormatItem{
public:
    FileNameFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getFileName();
    }
};

class LineFormatItem: public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getLine();
    }
};

class TableFormatItem: public LogFormatter::FormatItem{
public:
    TableFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << "\t";
    }
};

class FiberIdFormatItem: public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem: public LogFormatter::FormatItem{
public:
    ThreadNameFormatItem(const std::string &fmt = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << event->getThreadName();
    }
};

class StringFormatItem: public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string &str = "") : m_str(str) {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << m_str;
    }
private:
    std::string m_str;
};

void LogFormatter::init(){
    //m_items.push_back(std::make_shared<MessageFormatItem>());
    std::vector<std::tuple<std::string, std::string, int>> vec; //<str(%m...), fmt({%Y-%m-%d %H:%M:%S}), 0:stringItem/1:others>
    std::string str;
    for(std::size_t i = 0; i != m_pattern.size(); i++){
        if(m_pattern[i] != '%'){
            while(1){
                str.append(1, m_pattern[i++]);
                if(i == m_pattern.size() || m_pattern[i] == '%'){
                    vec.push_back({"", str, 0}); //stringItem
                    str.clear();
                    i--;
                    break;
                }
            }
        } else { //str[i]==%
            if(i + 1 != m_pattern.size()){
                i++;
                if(m_pattern[i] == '%') //%%
                    vec.push_back({"%", "", 0});
                else if(!isalpha(m_pattern[i])){ //%?
                    vec.push_back({"", std::string(m_pattern.begin() + i, m_pattern.begin() + i + 2), 0}); 
                } else {
                    if(m_pattern[i + 1] != '{'){
                        vec.push_back({std::string(1, m_pattern[i]), "", 1});
                    } else { //%d{%Y-%m-%d %H:%M:%S}
                        std::size_t orig = i;
                        i += 2;
                        while(1){
                            str.append(1, m_pattern[i++]);
                            if(m_pattern[i] == '}'){
                                vec.push_back({std::string(1, m_pattern[orig]), std::string(m_pattern.begin() + orig + 2, m_pattern.begin() + i), 1});
                                str.clear();
                                break;
                            } else if(i == m_pattern.size()){
                                vec.push_back({std::string(1, m_pattern[orig]), "", 1});
                                vec.push_back({"", std::string(m_pattern.begin() + orig + 1, m_pattern.begin() + i), 0});
                                str.clear();
                                i--;
                                break;
                            }
                        }
                    }
                }
            } else {
                vec.push_back({"%", "", 0}); //stringItem
            }
        }
    }
    /**
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
    static std::unordered_map<std::string, std::function<FormatItem::ptr(const std::string &)> > s_format_map = {
        {"m", [](const std::string &fmt){return FormatItem::ptr(new MessageFormatItem(fmt));}},
        {"p", [](const std::string &fmt){return FormatItem::ptr(new LevelFormatItem(fmt));}},
        {"r", [](const std::string &fmt){return FormatItem::ptr(new ElapseFormatItem(fmt));}},
        {"c", [](const std::string &fmt){return FormatItem::ptr(new LogNameFormatItem(fmt));}},
        {"t", [](const std::string &fmt){return FormatItem::ptr(new ThreadIdFormatItem(fmt));}},
        {"n", [](const std::string &fmt){return FormatItem::ptr(new NewLineFormatItem(fmt));}},
        {"d", [](const std::string &fmt){return FormatItem::ptr(new TimeFormatItem(fmt));}},
        {"f", [](const std::string &fmt){return FormatItem::ptr(new FileNameFormatItem(fmt));}},
        {"l", [](const std::string &fmt){return FormatItem::ptr(new LineFormatItem(fmt));}},
        {"T", [](const std::string &fmt){return FormatItem::ptr(new TableFormatItem(fmt));}},
        {"F", [](const std::string &fmt){return FormatItem::ptr(new FiberIdFormatItem(fmt));}},
        {"N", [](const std::string &fmt){return FormatItem::ptr(new ThreadNameFormatItem(fmt));}},
    };

    for(auto &i : vec){
        if(std::get<2>(i) == 0){
            m_items.push_back(std::make_shared<StringFormatItem>(std::get<1>(i)));
        } else {
            auto it = s_format_map.find(std::get<0>(i));
            if(it != s_format_map.end()){
                m_items.push_back(it->second(std::get<1>(i)));
            } else {
                m_items.push_back(std::make_shared<StringFormatItem>(" <error %" + std::string(std::get<0>(i)) + ">"));
            }
        }
    }
}

void LogAppender::setLevel(LogLevel::Level level){
    m_level = level;
}

void LogAppender::setFormatter(LogFormatter::ptr formatter){
    m_formatter = formatter;
}

Logger::Logger(const std::string &name)
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        auto self = shared_from_this();
        if(!m_appenders.empty()){
            for(auto &i : m_appenders){
                i->log(self, level, event);
            }  
        } else {
            m_root->log(level, event);
        }
    } 
}

void Logger::addAppender(LogAppender::ptr appender){
    if(appender->getFormatter() == nullptr){
        appender->setFormatter(m_formatter);
        m_appenders.push_back(appender);
    }
}

void Logger::delAppender(LogAppender::ptr appender){
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders(){
    m_appenders.clear();
}

void Logger::setName(const std::string &name){
    m_name = name;
}

void Logger::setLevel(LogLevel::Level level){
    m_level = level;
}

std::string Logger::getName(){
    return m_name;
}

LogAppender::MutexType StdOutLogAppender::m_mutex;
void StdOutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        MutexType::Lock lock(m_mutex);
        m_formatter->format(std::cout, logger, level, event);
    }
}

FileLogAppender::FileLogAppender(const std::string &fileName)
    :m_ofileName(fileName) {
    m_ofileStream.open(m_ofileName);
}

void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        MutexType::Lock lock(m_mutex);
        m_formatter->format(m_ofileStream, logger, level, event);
    }
}

LoggerManager::LoggerManager(){
    m_root.reset(new Logger("root"));
    m_root->addAppender(LogAppender::ptr(new StdOutLogAppender));
    m_loggers["root"] = m_root;
}

Logger::ptr LoggerManager::getLogger(const std::string &name){
    auto it = m_loggers.find(name);
    if(it == m_loggers.end()){
        Logger::ptr logger = Logger::ptr(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    } else {
        return it->second;
    }
}

class LogIniter{
public:
    LogIniter(){
        Logger::ptr logger = SYLAR_LOG_NAME("system");
        //logger->addAppender(LogAppender::ptr(new FileLogAppender("../tests/systemlog.txt")));
        logger->addAppender(LogAppender::ptr(new StdOutLogAppender));
    }
};

LogIniter _log_init;

}