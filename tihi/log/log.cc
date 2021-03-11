#include "log.h"

#include <functional>
#include <iostream>
#include <time.h>
#include <string.h>

namespace tihi {

LogEvent::LogEvent(uint64_t time, uint32_t elapse, const char* file,
                   uint32_t line, uint32_t threadId, uint32_t fiberId,
                   std::shared_ptr<Logger> logger, LogLevel::Level level)
    : time_(time),
      elapse_(elapse),
      kFile_(file),
      line_(line),
      threadId_(threadId),
      fiberId_(fiberId),
      logger_(logger),
      level_(level) {}

const char* LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name) \
    case name:   \
        return #name;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
    }
    return "UNKNOW";
}

void LogEvent::format(const char* fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    format(fmt, vl);
    va_end(vl);
}

void LogEvent::format(const char* fmt, va_list vl) {
    char* buf;
    int len = vasprintf(&buf, fmt, vl);
    if (len != -1) {
        content_ << std::string(buf, len);
    }
    free(buf);
}

LogEventWarp::LogEventWarp(LogEvent::ptr event) : event_(event) {

}

LogEventWarp::~LogEventWarp() {
    event_->logger()->log(event_->level(), event_);
}

std::stringstream& LogEventWarp::content() {
    return event_->content();
}

Logger::Logger(const std::string& name) : name_(name), level_(LogLevel::DEBUG) {
    /*
    time_         %d | elapse_      %r | kFile_      %f
    line_         %l | threadId_    %t | level       %p
    fiberId_      %F | content_     %m | enter       %n
    logger_name   %c
    */
    formatter_.reset(new LogFormatter(
        "[%d{%Y-%m-%d %H:%M:%S}] %t %F [%p] [%c] %f:%l %m %n"));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (level < level_) {
        return;
    }

    auto self = getptr();
    for (auto& i : appenders_) {
        i->log(self, level, event);
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(LogEvent::ptr event) { log(LogLevel::INFO, event); }

void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }


void Logger::addAppender(LogAppender::ptr appender) {
    if (!appender->formatter()) {
        appender->set_formatter(formatter_);
    }
    appenders_.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for (auto it = appenders_.begin(); it != appenders_.end(); ++it) {
        if (*it == appender) {
            appenders_.erase(it);
            break;
        }
    }
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : file_name_(filename) {}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event) {
    if (level_ > level) {
        return;
    }

    file_stream_ << formatter_->format(logger, level, event);
}

bool FileLogAppender::reopen() {
    if (file_stream_) {
        file_stream_.close();
    }

    file_stream_.open(file_name_);

    return !!file_stream_;
}

void StdOutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) {
    if (level_ > level) {
        return;
    }

    std::cout << formatter_->format(logger, level, event);
}

LogFormatter::LogFormatter(const std::string& pattern) : pattern_(pattern) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for (auto& i : items_) {
        i->format(ss, logger, level, event);
    }

    return ss.str();
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->kContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->elapse();
    }
};

class NameFormatIterm : public LogFormatter::FormatItem {
public:
    NameFormatIterm(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << logger->name();
    }
};

class ThreadIdFormatIterm : public LogFormatter::FormatItem {
public:
    ThreadIdFormatIterm(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->threadId();
    }
};

class FiberIdFormatIterm : public LogFormatter::FormatItem {
public:
    FiberIdFormatIterm(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->fiberId();
    }
};

class DateTimeFormatIterm : public LogFormatter::FormatItem {
public:
    DateTimeFormatIterm(const std::string& format = "%Y-%m-%d %H:%M:%S")
        : FormatItem(format), format_(format) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        if (format_.empty()) {
            format_ = "%Y-%m-%d %H:%M:%S";
        }
        time_t time = event->time();
        char buf[32];
        strftime(buf, sizeof(buf), format_.c_str(), localtime(&time));
        os << buf;
    }

private:
    std::string format_;
};

class FilenameFormatIterm : public LogFormatter::FormatItem {
public:
    FilenameFormatIterm(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->kFile();
    }
};

class LineFormatIterm : public LogFormatter::FormatItem {
public:
    LineFormatIterm(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->line();
    }
};

class EnterFormatIterm : public LogFormatter::FormatItem {
public:
    EnterFormatIterm(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class TabFormatIterm : public LogFormatter::FormatItem {
public:
    TabFormatIterm(const std::string& fmt = "") : FormatItem(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << '\t';
    }
};

class StringFormatIterm : public LogFormatter::FormatItem {
public:
    StringFormatIterm(const std::string& str = "")
        : FormatItem(str), string_(str) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << string_;
    }

private:
    std::string string_;
};

void LogFormatter::init() {
    std::vector<std::tuple<std::string, std::string, int>> vec;

    pattern_parser(vec);

    static std::map<std::string,
                    std::function<FormatItem::ptr(const std::string& fmt)>>
        s_para_item{
#define XX(para, C) \
    {(#para),       \
     ([](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); })}
            XX(m, MessageFormatItem),  XX(d, DateTimeFormatIterm),
            XX(r, ElapseFormatItem),   XX(f, FilenameFormatIterm),
            XX(l, LineFormatIterm),    XX(t, ThreadIdFormatIterm),
            XX(F, FiberIdFormatIterm), XX(n, EnterFormatIterm),
            XX(p, LevelFormatItem),    XX(c, NameFormatIterm),
            XX(T, TabFormatIterm),
#undef XX
        };

    /*
    time_         %d | elapse_      %r | kFile_      %f
    line_         %l | threadId_    %t | level       %p
    fiberId_      %F | content_     %m | enter       %n
    logger_name   %c | Table        %T |
    */
    for (auto& t : vec) {
        if (std::get<2>(t) == 0) {
            items_.push_back(
                FormatItem::ptr(new StringFormatIterm(std::get<0>(t))));
        } else {
            auto it = s_para_item.find(std::get<0>(t));
            if (it == s_para_item.end()) {
                items_.push_back(FormatItem::ptr(new StringFormatIterm(
                    "<<error_format: %" + std::get<0>(t) + " >>")));
            } else {
                items_.push_back(it->second(std::get<1>(t)));
            }
        }

        // std::cout << std::get<0>(t) << " - " << std::get<1>(t) << " - "
        //           << std::get<2>(t) << std::endl;
    }
}


/*
%% %d{xx-xx-xx xx-xx-xx} %m
*/
bool LogFormatter::pattern_parser(
    std::vector<std::tuple<std::string, std::string, int>>& vec) {
    size_t p_n = pattern_.size();

    std::string nstr;
    size_t i = 0;
    while (i < p_n) {
        if (pattern_[i] != '%') {
            nstr.append(1, pattern_[i]);
            ++i;
            continue;
        }

        if (i + 1 < p_n && pattern_[i + 1] == '%') {
            nstr.append(1, '%');
            ++i;
            continue;
        }

        vec.push_back(std::make_tuple(nstr, "", 0));
        nstr.clear();

        ++i;
        if (i >= p_n) {
            break;
        }

        std::string para = pattern_.substr(i, 1);

        ++i;

        if (i < p_n && pattern_[i] == '{') {
            ++i;
            size_t fmt_end = pattern_.find_first_of('}', i);
            if (fmt_end == pattern_.npos) {
                std::cout << "pattern parse error..." << pattern_ << " - "
                          << pattern_.substr(i) << std::endl;
                return false;
            }

            std::string fmt = pattern_.substr(i, fmt_end - i);
            vec.push_back(std::make_tuple(para, fmt, 1));
            i = fmt_end + 1;
        } else {
            vec.push_back(std::make_tuple(para, "", 1));
        }
    }
    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "string", 0));
    }

    return true;
}

LoggerManager::LoggerManager() {
    root_ = Logger::ptr(new Logger);
    root_->addAppender(StdOutLogAppender::ptr(new StdOutLogAppender));
}

Logger::ptr LoggerManager::logger(const std::string& name) {
    auto it = loggers_.find(name);
    return it == loggers_.end() ? root_ : it->second;
}

}  // namespace tihi
