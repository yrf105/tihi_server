#ifndef TIHI_LOG_H_
#define TIHI_LOG_H_

#include <stdarg.h>
#include <stdint.h>

#include <fstream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <map>

#include "utils/singleton.h"
#include "utils/utils.h"

#define TIHI_LOG_LEVEL(logger, grade)                                        \
    if (logger->level() <= grade)                                            \
    tihi::LogEventWarp(tihi::LogEvent::ptr(new tihi::LogEvent(               \
                           time(0), 0, __FILE__, __LINE__, tihi::ThreadId(), \
                           tihi::FiberId(), logger, grade)))                 \
        .content()

#define TIHI_LOG_DEBUG(logger) TIHI_LOG_LEVEL(logger, tihi::LogLevel::DEBUG)
#define TIHI_LOG_INFO(logger) TIHI_LOG_LEVEL(logger, tihi::LogLevel::INFO)
#define TIHI_LOG_WARN(logger) TIHI_LOG_LEVEL(logger, tihi::LogLevel::WARN)
#define TIHI_LOG_ERROR(logger) TIHI_LOG_LEVEL(logger, tihi::LogLevel::ERROR)
#define TIHI_LOG_FATAL(logger) TIHI_LOG_LEVEL(logger, tihi::LogLevel::FATAL)

#define TIHI_LOG_FMT_LEVEL(logger, grade, fmt, ...)                          \
    if (logger->level() <= grade)                                            \
    tihi::LogEventWarp(tihi::LogEvent::ptr(new tihi::LogEvent(               \
                           time(0), 0, __FILE__, __LINE__, tihi::ThreadId(), \
                           tihi::FiberId(), logger, grade)))                 \
        .event()                                                             \
        ->format(fmt, __VA_ARGS__)

#define TIHI_LOG_FMT_DEBUG(logger, fmt, ...) \
    TIHI_LOG_FMT_LEVEL(logger, tihi::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define TIHI_LOG_FMT_INFO(logger, fmt, ...) \
    TIHI_LOG_FMT_LEVEL(logger, tihi::LogLevel::INFO, fmt, __VA_ARGS__)
#define TIHI_LOG_FMT_WARN(logger, fmt, ...) \
    TIHI_LOG_FMT_LEVEL(logger, tihi::LogLevel::WARN, fmt, __VA_ARGS__)
#define TIHI_LOG_FMT_ERROR(logger, fmt, ...) \
    TIHI_LOG_FMT_LEVEL(logger, tihi::LogLevel::ERROR, fmt, __VA_ARGS__)
#define TIHI_LOG_FMT_FATAL(logger, fmt, ...) \
    TIHI_LOG_FMT_LEVEL(logger, tihi::LogLevel::FATAL, fmt, __VA_ARGS__)

#define TIHI_LOG_ROOT() tihi::LoggerMgr::GetInstance()->root()

namespace tihi {

class Logger;

class LogLevel {
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };

    static const char* ToString(LogLevel::Level level);
};

class LogEvent {
public:
    using ptr = std::shared_ptr<LogEvent>;
    LogEvent(uint64_t time, uint32_t elapse, const char* file, uint32_t line,
             uint32_t threadId, uint32_t fiberId,
             std::shared_ptr<Logger> logger, LogLevel::Level level);

    uint64_t time() const { return time_; }
    uint32_t elapse() const { return elapse_; }
    const char* kFile() const { return kFile_; }
    uint32_t line() const { return line_; }
    uint32_t threadId() const { return threadId_; }
    uint32_t fiberId() const { return fiberId_; }
    const std::string kContent() const { return content_.str(); }
    std::stringstream& content() { return content_; }
    std::shared_ptr<Logger> logger() const { return logger_; }
    LogLevel::Level level() const { return level_; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list vl);

private:
    uint64_t time_ = 0;
    uint32_t elapse_ = 0;
    const char* kFile_ = nullptr;
    uint32_t line_ = 0;
    uint32_t threadId_ = 0;
    uint32_t fiberId_ = 0;
    std::stringstream content_;

    std::shared_ptr<Logger> logger_;
    LogLevel::Level level_;
};

class LogEventWarp {
public:
    LogEventWarp(LogEvent::ptr event);
    ~LogEventWarp();
    LogEvent::ptr event() const { return event_; }
    std::stringstream& content();

private:
    LogEvent::ptr event_;
};

class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;

    LogFormatter(const std::string& pattern);

    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       LogEvent::ptr event);

public:
    class FormatItem {
    public:
        using ptr = std::shared_ptr<FormatItem>;

        FormatItem(const std::string& fmt = "") {}
        virtual ~FormatItem(){};

        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) = 0;

    private:
    };

    void init();
    bool pattern_parser(
        std::vector<std::tuple<std::string, std::string, int>>& vec);

private:
    std::string pattern_;
    std::vector<FormatItem::ptr> items_;
};

class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;

    virtual ~LogAppender(){};

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;

    LogFormatter::ptr formatter() const { return formatter_; }
    void set_formatter(LogFormatter::ptr val) { formatter_ = val; }
    LogLevel::Level level() const { return level_; }
    void set_level(LogLevel::Level level) { level_ = level; }

protected:
    LogLevel::Level level_ = LogLevel::Level::DEBUG;
    LogFormatter::ptr formatter_;
};

class Logger : public std::enable_shared_from_this<Logger> {
public:
    using ptr = std::shared_ptr<Logger>;

    Logger(const std::string& name = "root");

    void log(LogLevel::Level level, LogEvent::ptr event);
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    const std::string name() const { return name_; };
    LogLevel::Level level() const { return level_; }
    void set_level(LogLevel::Level level) { level_ = level; }
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    std::shared_ptr<Logger> getptr() { return shared_from_this(); }

private:
    std::string name_;
    LogLevel::Level level_;
    LogFormatter::ptr formatter_;
    std::list<LogAppender::ptr> appenders_;
};

class StdOutLogAppender : public LogAppender {
public:
    using ptr = std::shared_ptr<StdOutLogAppender>;

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) override;
};

class FileLogAppender : public LogAppender {
public:
    using ptr = std::shared_ptr<FileLogAppender>;

    FileLogAppender(const std::string& filename);

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) override;
    bool reopen();

private:
    std::string file_name_;
    std::ofstream file_stream_;
};

class LoggerManager {
public:
    LoggerManager();
    Logger::ptr logger(const std::string& name);

    Logger::ptr root() const { return root_; }

private:
    Logger::ptr root_;
    std::map<std::string, Logger::ptr> loggers_;
};

using LoggerMgr = SingletonPtr<LoggerManager>;

}  // namespace tihi

#endif  // TIHI_LOG_H_