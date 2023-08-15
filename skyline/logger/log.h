/**
 * 该文件提供基本日志功能：包括
 *      日志器Logger（可包含多个Appender）
 *      日志输出地LogAppender（可同时被多个Logger使用，但只能有一个Formatter）
 *      日志格式化器LogFormatter（可同时被多个Appender使用）
 *      日志等级LogLevel
 * 除此之外，提供两个简单的Appender：控制台、文件（非线程安全）
 * 向外提供便携的宏定义，方便使用（提供流和格式化两种使用方式）
 *
 * example:
 *      auto& logger = skyline::logger::getRootLogger();
 *      SKYLINE_LOG_DEBUG(logger) << "test log";
 *      SKYLINE_LOG_FMT_DEBUG(logger, "%s", "test log");
 *
 * tip:
 * 由于Appender可以被多个Logger同时使用，因此它的线程安全由其子类实现处理
 * LogAppender基类本身不做线程安全保证
 **/

#pragma once

#include <sys/syscall.h>

#include <ctime>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <unordered_set>

#define SKYLINE_LOG_LEVEL(log, lv)                       \
    if (log.level <= lv)                                 \
    skyline::logger::LogEventWrap(                       \
        log, lv,                                         \
        skyline::logger::LogEvent{                       \
            .file = __FILE__,                            \
            .line = __LINE__,                            \
            .thread_id = skyline::logger::getThreadID(), \
            .time = std::time(0),                        \
        })                                               \
        .ss

#define SKYLINE_LOG_DEBUG(log) \
    SKYLINE_LOG_LEVEL(log, skyline::logger::LogLevel::Debug)
#define SKYLINE_LOG_INFO(log) \
    SKYLINE_LOG_LEVEL(log, skyline::logger::LogLevel::Info)
#define SKYLINE_LOG_WARN(log) \
    SKYLINE_LOG_LEVEL(log, skyline::logger::LogLevel::Warn)
#define SKYLINE_LOG_ERROR(log) \
    SKYLINE_LOG_LEVEL(log, skyline::logger::LogLevel::Error)
#define SKYLINE_LOG_FATAL(log) \
    SKYLINE_LOG_LEVEL(log, skyline::logger::LogLevel::Fatal)

#define SKYLINE_LOG_FMT_LEVEL(logg, lv, fmt, ...)                          \
    if (logg.level <= lv)                                                  \
    logg.log(lv, skyline::logger::LogEvent{                                \
                     .file = __FILE__,                                     \
                     .line = __LINE__,                                     \
                     .thread_id = skyline::logger::getThreadID(),          \
                     .time = std::time(0),                                 \
                     .content = skyline::logger::format(fmt, __VA_ARGS__), \
                 })

#define SKYLINE_LOG_FMT_DEBUG(log, fmt, ...)                          \
    SKYLINE_LOG_FMT_LEVEL(log, skyline::logger::LogLevel::Debug, fmt, \
                          __VA_ARGS__)
#define SKYLINE_LOG_FMT_INFO(log, fmt, ...)                          \
    SKYLINE_LOG_FMT_LEVEL(log, skyline::logger::LogLevel::Info, fmt, \
                          __VA_ARGS__)
#define SKYLINE_LOG_FMT_WARN(log, fmt, ...)                          \
    SKYLINE_LOG_FMT_LEVEL(log, skyline::logger::LogLevel::Warn, fmt, \
                          __VA_ARGS__)
#define SKYLINE_LOG_FMT_ERROR(log, fmt, ...)                          \
    SKYLINE_LOG_FMT_LEVEL(log, skyline::logger::LogLevel::Error, fmt, \
                          __VA_ARGS__)
#define SKYLINE_LOG_FMT_FATAL(log, fmt, ...)                          \
    SKYLINE_LOG_FMT_LEVEL(log, skyline::logger::LogLevel::Fatal, fmt, \
                          __VA_ARGS__)

namespace skyline::logger {

class Logger;

// 日志级别
enum class LogLevel {
    Debug = 1,
    Info,
    Warn,
    Error,
    Fatal,
};
std::ostream& operator<<(std::ostream& os, LogLevel level);

// 日志事件
struct LogEvent {
    const char* file{nullptr};  // 文件名
    int32_t line{0};            // 行号
    uint32_t elapse{0};         // 从程序启动的毫秒数
    uint32_t thread_id{0};      // 线程 id
    time_t time{0};             // 时间戳
    std::string content;
};

// 只为简化格式化输出
std::string format(const char* fmt, ...);

// 日志事件包装器，只为简化流式输出
// 注意：将忽略 event 本身的 content
class LogEventWrap final {
public:
    LogEventWrap(Logger& logger, LogLevel level,
                 const LogEvent& event) noexcept;
    ~LogEventWrap();

public:
    std::stringstream ss;

private:
    Logger& _logger;
    LogLevel _level;
    LogEvent _event;
};

/**
 * 日志格式器
 *
 * 格式操作符：
 * %m -> message
 * %p -> level
 * %r -> elapse
 * %c -> logger name
 * %t -> thread ID
 * %T -> tab
 * %n -> new line
 * %d{xx-xx} -> date
 * %f -> file
 * %l -> line
 **/
class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;

    LogFormatter(const std::string_view& pattern);

    void format(std::ostream& os, const Logger& logger, LogLevel level,
                const LogEvent& event) const;

private:
    using FormatItemFunc =
        std::function<void(std::ostream& os, const Logger& logger,
                           LogLevel level, const LogEvent& event)>;

private:
    std::vector<FormatItemFunc> _items;
};

// 日志输出地
class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;

    LogAppender();  // 将使用默认的 formatter
    LogAppender(LogFormatter::ptr formatter);
    virtual ~LogAppender() = default;

    virtual void log(const Logger& logger, LogLevel level,
                     const LogEvent& event) = 0;

    void setFormatter(LogFormatter::ptr formatter);

public:
    LogLevel level{LogLevel::Debug};

protected:
    LogFormatter::ptr _formatter;  // 保证有一个可用的 formatter
};

// 日志器
class Logger {
public:
    Logger(std::string name);

    void log(LogLevel level, const LogEvent& event) const;

    void debug(const LogEvent& event) const;
    void info(const LogEvent& event) const;
    void warn(const LogEvent& event) const;
    void error(const LogEvent& event) const;
    void fatal(const LogEvent& event) const;

    void addAppender(LogAppender::ptr appender);
    void delAppender(const LogAppender::ptr& appender);

    const std::string& getName() const;

public:
    LogLevel level{LogLevel::Debug};

private:
    std::string _name;
    std::unordered_set<LogAppender::ptr> _appenders;
};

// 控制台
class StdoutLogAppender : virtual public LogAppender {
public:
    using LogAppender::LogAppender;

    void log(const Logger& logger, LogLevel level,
             const LogEvent& event) override;
};

// 文件日志输出
// 创建时打开文件，销毁时关闭文件
class FileLogAppender : virtual public LogAppender {
public:
    FileLogAppender(std::string filename);

    void log(const Logger& logger, LogLevel level,
             const LogEvent& event) override;

private:
    std::string _filename;
    std::ofstream _filestream;
};

// Root日志器，默认包含一个控制台输出
// 请使用引用接收返回值，避免发生复制（除非明确需要一个复制的Logger）
Logger& getRootLogger() noexcept;
// 请使用引用接收返回值，避免发生复制，以便可以被管理器管理（除非明确需要一个复制的Logger）
Logger& getLogger(const std::string& name);

uint32_t getThreadID();

}  // namespace skyline::logger
