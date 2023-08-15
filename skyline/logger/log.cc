#include "log.h"

#include <cstdarg>
#include <iomanip>
#include <iostream>

/**
 * features:
 * C++11:
 *  function(lambda), shared_ptr, unique_ptr, unordered_set, unordered_map
 *  enum class, move, put_time, emplace
 * C++17:
 *  string_view
 * C++20:
 *  unordered_map::contains
 **/

namespace skyline::logger {

std::ostream& operator<<(std::ostream& os, LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return os << "DEBUG";
        case LogLevel::Info:
            return os << "INFO";
        case LogLevel::Warn:
            return os << "WARN";
        case LogLevel::Error:
            return os << "ERROR";
        case LogLevel::Fatal:
            return os << "FATAL";
        default:
            return os << "UNKNOW";
    }
}

std::string format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if (len != -1) {
        auto res = std::string(buf, len);
        free(buf);
        return res;
    }
    va_end(al);
    return "";
}

// ---------------------------- LogEventWrap ----------------------------

LogEventWrap::LogEventWrap(Logger& logger, LogLevel level,
                           const LogEvent& event) noexcept
    : _logger(logger), _level(level) {
    _event.file = event.file;
    _event.line = event.line;
    _event.elapse = event.elapse;
    _event.thread_id = event.thread_id;
    _event.time = event.time;
}

LogEventWrap::~LogEventWrap() {
    _event.content = ss.str();
    _logger.log(_level, _event);
}

// ---------------------------- Logger ----------------------------

Logger::Logger(std::string name) : _name(std::move(name)) {
}

void Logger::log(LogLevel level, const LogEvent& event) const {
    if (level < this->level) return;
    for (auto& a : _appenders) {
        a->log(*this, level, event);
    }
}

void Logger::debug(const LogEvent& event) const {
    log(LogLevel::Debug, event);
}

void Logger::info(const LogEvent& event) const {
    log(LogLevel::Debug, event);
}

void Logger::warn(const LogEvent& event) const {
    log(LogLevel::Debug, event);
}

void Logger::error(const LogEvent& event) const {
    log(LogLevel::Debug, event);
}

void Logger::fatal(const LogEvent& event) const {
    log(LogLevel::Debug, event);
}

void Logger::addAppender(LogAppender::ptr appender) {
    _appenders.emplace(std::move(appender));
}

void Logger::delAppender(const LogAppender::ptr& appender) {
    _appenders.erase(appender);
}

const std::string& Logger::getName() const {
    return _name;
}

// ---------------------------- Appender ----------------------------

static LogFormatter::ptr kDefaultFormatter = std::make_shared<LogFormatter>(
    "%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n");
LogAppender::LogAppender() : LogAppender(kDefaultFormatter) {
}

LogAppender::LogAppender(LogFormatter::ptr formatter)
    : _formatter(std::move(formatter)) {
}

void LogAppender::setFormatter(LogFormatter::ptr formatter) {
    if (formatter) _formatter.swap(formatter);
}

void StdoutLogAppender::log(const Logger& logger, LogLevel level,
                            const LogEvent& event) {
    if (level < this->level) return;
    _formatter->format(std::cout, logger, level, event);
}

FileLogAppender::FileLogAppender(std::string filename)
    : _filename(std::move(filename)) {
    _filestream.open(_filename, std::ios::app);
    if (!_filestream) {
        auto& logger = getRootLogger();
        SKYLINE_LOG_FMT_DEBUG(logger, "log file: `%s` open failed",
                              _filename.c_str());
    }
}

void FileLogAppender::log(const Logger& logger, LogLevel level,
                          const LogEvent& event) {
    if (level < this->level) return;
    _formatter->format(_filestream, logger, level, event);
}

// ---------------------------- Formatter ----------------------------

LogFormatter::LogFormatter(const std::string_view& pattern) {
    for (size_t i = 0, start = 0; i < pattern.size(); ++i) {
        if (pattern[i] != '%') continue;
        if (i > start) {  // string item
            _items.push_back([str = pattern.substr(start, i - start)](
                                 std::ostream& os, const Logger& logger,
                                 LogLevel level,
                                 const LogEvent& event) { os << str; });
        }
        if (i + 1 >= pattern.size()) break;
        switch (pattern[++i]) {
            case 'm':  // content item
                _items.push_back(
                    [](std::ostream& os, const Logger& logger, LogLevel level,
                       const LogEvent& event) { os << event.content; });
                break;
            case 'p':  // level item
                _items.push_back([](std::ostream& os, const Logger& logger,
                                    LogLevel level,
                                    const LogEvent& event) { os << level; });
                break;
            case 'r':  // elapse item
                _items.push_back(
                    [](std::ostream& os, const Logger& logger, LogLevel level,
                       const LogEvent& event) { os << event.elapse; });
                break;
            case 'c':  // logger item
                _items.push_back(
                    [](std::ostream& os, const Logger& logger, LogLevel level,
                       const LogEvent& event) { os << logger.getName(); });
                break;
            case 't':  // thread item
                _items.push_back(
                    [](std::ostream& os, const Logger& logger, LogLevel level,
                       const LogEvent& event) { os << event.thread_id; });
                break;
            case 'T':  // tab item
                _items.push_back([](std::ostream& os, const Logger& logger,
                                    LogLevel level,
                                    const LogEvent& event) { os << '\t'; });
                break;
            case 'n':  // new line item
                _items.push_back([](std::ostream& os, const Logger& logger,
                                    LogLevel level,
                                    const LogEvent& event) { os << '\n'; });
                break;
            case 'd':  // date item
            {
                auto fmt = [&]() -> std::string {
                    auto default_fmt = "%Y-%m-%d %H:%M:%S";
                    // 至少有一对大括号，才表示日期的格式
                    if (i + 2 >= pattern.size() || pattern[i + 1] != '{')
                        return default_fmt;
                    auto it = pattern.find('}', i + 2);
                    if (it == pattern.npos) return default_fmt;
                    auto start = i + 2;
                    i = it;
                    return std::string(pattern, start, it - start);
                }();
                _items.push_back([fmt](std::ostream& os, const Logger& logger,
                                       LogLevel level, const LogEvent& event) {
                    os << std::put_time(std::localtime(&event.time),
                                        fmt.c_str());
                });
                break;
            }
            case 'f':  // file item
                _items.push_back(
                    [](std::ostream& os, const Logger& logger, LogLevel level,
                       const LogEvent& event) { os << event.file; });
                break;
            case 'l':  // line item
                _items.push_back(
                    [](std::ostream& os, const Logger& logger, LogLevel level,
                       const LogEvent& event) { os << event.line; });
                break;
            default:  // 忽略其它情况
                --i;
        }
        start = i + 1;
    }
}

void LogFormatter::format(std::ostream& os, const Logger& logger,
                          LogLevel level, const LogEvent& event) const {
    for (auto& i : _items) i(os, logger, level, event);
}

// ---------------------------- LoggerManager ----------------------------

Logger& getRootLogger() noexcept {
    static Logger root_logger = []() -> Logger {
        Logger logger("root");
        logger.addAppender(
            std::make_shared<skyline::logger::StdoutLogAppender>());
        return logger;
    }();
    return root_logger;
}

static std::unordered_map<std::string, Logger> kLoggers;
static std::mutex kLoggerMutex;
Logger& getLogger(const std::string& name) {
    kLoggerMutex.lock();
    if (!kLoggers.contains(name)) {
        kLoggers.emplace(name, Logger(name));
    }
    kLoggerMutex.unlock();
    return kLoggers.find(name)->second;
}

uint32_t getThreadID() {
    static thread_local auto id = syscall(__NR_gettid);
    return id;
}

}  // namespace skyline::logger
