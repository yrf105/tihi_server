#include "log.h"

#include <string.h>
#include <time.h>

#include <functional>
#include <iostream>

#include "config/config.h"

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

LogLevel::Level LogLevel::FormString(const std::string& str) {
#define XX(name, v) \
    if (str == #v) return LogLevel::name

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    return LogLevel::UNKNOW;

#undef XX
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

LogEventWarp::LogEventWarp(LogEvent::ptr event) : event_(event) {}

LogEventWarp::~LogEventWarp() {
    event_->logger()->log(event_->level(), event_);
}

std::stringstream& LogEventWarp::content() { return event_->content(); }

LogFormatter::ptr LogAppender::formatter() {
    mutex_type::read_lock lock(mutex_);

    return formatter_;
}

void LogAppender::set_formatter(LogFormatter::ptr val, bool from_logger) {
    mutex_type::write_lock lock(mutex_);

    if (!from_logger) {
        hasFormatter_ = true;
    }
    formatter_ = val;
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
    mutex_type::read_lock lock(mutex_);

    if (level < level_) {
        return;
    }

    auto self = getptr();
    if (!appenders_.empty()) {
        for (auto& i : appenders_) {
            i->log(self, level, event);
        }
    } else if (root_) {
        root_->log(level, event);
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(LogEvent::ptr event) { log(LogLevel::INFO, event); }

void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }

LogFormatter::ptr Logger::formatter() { return formatter_; }

void Logger::set_formatter(LogFormatter::ptr formatter) {
    mutex_type::write_lock lock(mutex_);

    if (formatter->isError()) {
        return;
    }
    formatter_ = formatter;
    for (auto& ap : appenders_) {
        if (ap->hasFormatter()) {
            continue ;
        }
        ap->set_formatter(formatter_, true);
    }
}

void Logger::set_formatter(const std::string& formatter) {
    LogFormatter::ptr new_formatter =
        LogFormatter::ptr(new LogFormatter(formatter));
    if (new_formatter->isError()) {
        return;
    }

    formatter_ = new_formatter;
    set_formatter(new_formatter);
}

void Logger::addAppender(LogAppender::ptr appender) {
    mutex_type::write_lock lock(mutex_);

    if (!appender->hasFormatter() && !appender->formatter()) {
        appender->set_formatter(formatter_, true);
    }
    appenders_.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    mutex_type::write_lock lock(mutex_);

    for (auto it = appenders_.begin(); it != appenders_.end(); ++it) {
        if (*it == appender) {
            appenders_.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    mutex_type::write_lock lock(mutex_);
    
    appenders_.clear();
}

const std::string Logger::toYAMLString() {
    mutex_type::read_lock lock(mutex_);

    YAML::Node node;
    node["name"] = name_;
    if (level_ != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(level_);
    }
    if (formatter_ && !formatter_->pattern().empty() &&
        !formatter_->isError()) {
        node["formatter"] = formatter_->pattern();
    }
    for (auto& appender : appenders_) {
        node["appenders"].push_back(YAML::Load(appender->toYAMLString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : file_name_(filename) {
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event) {
    mutex_type::write_lock lock(mutex_);

    if (level_ > level) {
        return;
    }

    if (last_time_ != time(0)) {
        reopen();
        last_time_ = time(0);
    }

    file_stream_ << formatter_->format(logger, level, event);
}

bool FileLogAppender::reopen() {
    mutex_type::write_lock lock(mutex_);

    if (file_stream_) {
        file_stream_.close();
    }

    file_stream_.open(file_name_);

    return !!file_stream_;
}

const std::string FileLogAppender::toYAMLString() {
    mutex_type::read_lock lock(mutex_);

    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = file_name_;
    if (level_ != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(level_);
    }
    if (formatter_ && !formatter_->pattern().empty() &&
        !formatter_->isError() && hasFormatter()) {
        node["formatter"] = formatter_->pattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void StdOutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) {
    mutex_type::write_lock lock(mutex_);
    
    if (level_ > level) {
        return;
    }

    std::cout << formatter_->format(logger, level, event);
}

const std::string StdOutLogAppender::toYAMLString() {
    mutex_type::read_lock lock(mutex_);

    YAML::Node node;
    node["type"] = "StdOutLogAppender";
    if (level_ != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(level_);
    }
    if (formatter_ && !formatter_->pattern().empty() &&
        !formatter_->isError() && hasFormatter()) {
        node["formatter"] = formatter_->pattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern) : pattern_(pattern) {
    init();
    if (error_) {
        std::cout << "\n invalid pattern!!! pattern" << pattern << std::endl;
    }
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
        os << event->logger()->name();
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

    error_ = !pattern_parser(vec);

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
解析成功返回 true
解析失败返回 false
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

    loggers_[root_->name()] = root_;
}

Logger::ptr LoggerManager::logger(const std::string& name) {
    {
        mutex_type::read_lock lock(mutex_);
        
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            return it->second;
        }
    }
    
    mutex_type::write_lock lock(mutex_);

    Logger::ptr logger(new Logger(name));
    logger->addAppender(StdOutLogAppender::ptr(new StdOutLogAppender));
    loggers_[logger->name()] = logger;
    return logger;
}

const std::string LoggerManager::toYAMLString() {
    mutex_type::read_lock lock(mutex_);

    YAML::Node node;
    for (auto& l : loggers_) {
        if (!l.second) {
            continue ;
        }
        node.push_back(YAML::Load(l.second->toYAMLString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}


struct ConfigLogAppender {
    int type_ = 0;  // 1 FileOut, 2 StdOut
    LogLevel::Level level_ = LogLevel::UNKNOW;
    std::string formatter_;
    std::string file_;

    bool operator==(const ConfigLogAppender& oth) const {
        return (type_ == oth.type_ && level_ == oth.level_ &&
                formatter_ == oth.formatter_ && file_ == oth.file_);
    }
};

struct ConfigLogger {
    std::string name_;
    LogLevel::Level level_;
    std::string formatter_;
    std::vector<ConfigLogAppender> appenders_;

    bool operator==(const ConfigLogger& oth) const {
        return (name_ == oth.name_ && level_ == oth.level_ &&
                formatter_ == oth.formatter_ && appenders_ == oth.appenders_);
    }

    bool operator<(const ConfigLogger& oth) const { return name_ < oth.name_; }
};

template <>
class LexicalCast<std::string, std::set<ConfigLogger>> {
public:
    std::set<ConfigLogger> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        std::set<ConfigLogger> ret;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            auto n = node[i];
            if (!n["name"].IsDefined()) {
                std::cout << "log config name is null.\n";
                continue;
            }

            ConfigLogger cl;
            cl.name_ = n["name"].as<std::string>();
            cl.level_ = LogLevel::FormString(
                n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if (n["formatter"].IsDefined()) {
                cl.formatter_ = n["formatter"].as<std::string>();
            }
            if (n["appenders"].IsDefined()) {
                for (size_t j = 0; j < n["appenders"].size(); ++j) {
                    auto a = n["appenders"][j];
                    if (!a["type"].IsDefined()) {
                        std::cout << "log appender config type is null.\n";
                        continue;
                    }

                    ConfigLogAppender cla;
                    std::string tmp_type = a["type"].as<std::string>();
                    if (tmp_type == "FileLogAppender") {
                        cla.type_ = 1;
                        if (!a["file"].IsDefined()) {
                            std::cout << "log appender config file is null.\n";
                            continue;
                        }
                        cla.file_ = a["file"].as<std::string>();
                    } else if (tmp_type == "StdOutLogAppender") {
                        cla.type_ = 2;
                    } else {
                        std::cout << "log appender config type is error.\n";
                        continue;
                    }

                    if (a["level"].IsDefined()) {
                        cla.level_ =
                            LogLevel::FormString(a["level"].as<std::string>());
                    }
                    if (a["formatter"].IsDefined()) {
                        cla.formatter_ = a["formatter"].as<std::string>();
                    }

                    cl.appenders_.push_back(cla);
                }
            }

            ret.insert(cl);
        }

        return ret;
    }
};

template <>
class LexicalCast<std::set<ConfigLogger>, std::string> {
public:
    std::string operator()(const std::set<ConfigLogger>& v) {
        YAML::Node node;
        for (auto& i : v) {
            YAML::Node n;
            n["name"] = i.name_;
            if (i.level_ != LogLevel::UNKNOW) {
                n["level"] = LogLevel::ToString(i.level_);
            }
            if (!i.formatter_.empty()) {
                n["formatter"] = i.formatter_;
            }

            for (auto& a : i.appenders_) {
                YAML::Node appender;
                if (a.type_ == 1) {
                    appender["type"] = "FileLogAppender";
                    appender["file"] = a.file_;
                } else if (a.type_ == 2) {
                    appender["type"] = "StdOutLogAppender";
                }
                if (a.level_ != LogLevel::UNKNOW) {
                    appender["level"] = LogLevel::ToString(a.level_);
                }
                if (!a.formatter_.empty()) {
                    appender["formatter"] = a.formatter_;
                }
                n["appenders"].push_back(appender);
            }
            node.push_back(n);
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

ConfigVar<std::set<ConfigLogger>>::ptr g_logger_config =
    Config::Lookup<std::set<ConfigLogger>>("logs", std::set<ConfigLogger>(),
                                           "logs config");

struct LogInit {
    LogInit() {
        g_logger_config->addListener(
            0, [](const std::set<ConfigLogger>& old_val,
                  const std::set<ConfigLogger>& new_val) {
                std::cout << "$ $ $ $ $ $ $ $ $ $ config logger!\n";
                // 新的里面有，旧的里面没有，新增
                // 或者
                // 新的旧的里面都有，修改
                for (auto& i : new_val) {
                    auto logger = TIHI_LOG_LOGGER(i.name_);
                    logger->set_level(i.level_);
                    logger->clearAppenders();
                    if (!i.formatter_.empty()) {
                        logger->set_formatter(i.formatter_);
                    }
                    for (auto& a : i.appenders_) {
                        LogAppender::ptr appender;
                        if (a.type_ == 1) {
                            appender.reset(new FileLogAppender(a.file_));
                        } else if (a.type_ == 2) {
                            appender.reset(new StdOutLogAppender);
                        }
                        appender->set_level(a.level_);
                        if (!a.formatter_.empty()) {
                            appender->set_formatter(
                                LogFormatter::ptr(new LogFormatter(a.formatter_)), false);
                        }
                        logger->addAppender(LogAppender::ptr(appender));
                    }
                }

                // 旧的里面有，新的里面没有，删除
                for (auto& i : old_val) {
                    auto it = new_val.find(i);
                    if (it == new_val.end()) {
                        auto logger = TIHI_LOG_LOGGER(i.name_);
                        logger->set_level(LogLevel::Level(INT_MAX));
                        logger->clearAppenders();
                    }
                }
            });
    }
};

static LogInit __log_init;

void LoggerManager::init() {}

}  // namespace tihi
