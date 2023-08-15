#pragma once

#include "../logger/log.h"

#define SYSTEM_LOG_LEVEL(lv)                             \
    if (skyline::core::getSystemLogger().level <= lv)    \
    skyline::logger::LogEventWrap(                       \
        skyline::core::getSystemLogger(), lv,            \
        skyline::logger::LogEvent{                       \
            .file = __FILE__,                            \
            .line = __LINE__,                            \
            .thread_id = skyline::logger::getThreadID(), \
            .time = std::time(0),                        \
        })                                               \
        .ss

#define SYSTEM_LOG_DEBUG SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::Debug)
#define SYSTEM_LOG_INFO SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::Info)
#define SYSTEM_LOG_WARN SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::Warn)
#define SYSTEM_LOG_ERROR SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::Error)
#define SYSTEM_LOG_FATAL SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::Fatal)

#define SYSTEM_LOG_FMT_LEVEL(lv, fmt, ...)                            \
    if (skyline::core::getSystemLogger().level <= lv)                 \
    skyline::core::getSystemLogger().log(                             \
        lv, skyline::logger::LogEvent{                                \
                .file = __FILE__,                                     \
                .line = __LINE__,                                     \
                .thread_id = skyline::logger::getThreadID(),          \
                .time = std::time(0),                                 \
                .content = skyline::logger::format(fmt, __VA_ARGS__), \
            })

#define SYSTEM_LOG_FMT_DEBUG(fmt, ...) \
    SYSTEM_LOG_FMT_LEVEL(skyline::logger::LogLevel::Debug, fmt, __VA_ARGS__)
#define SYSTEM_LOG_FMT_INFO(fmt, ...) \
    SYSTEM_LOG_FMT_LEVEL(skyline::logger::LogLevel::Info, fmt, __VA_ARGS__)
#define SYSTEM_LOG_FMT_WARN(fmt, ...) \
    SYSTEM_LOG_FMT_LEVEL(skyline::logger::LogLevel::Warn, fmt, __VA_ARGS__)
#define SYSTEM_LOG_FMT_ERROR(fmt, ...) \
    SYSTEM_LOG_FMT_LEVEL(skyline::logger::LogLevel::Error, fmt, __VA_ARGS__)
#define SYSTEM_LOG_FMT_FATAL(fmt, ...) \
    SYSTEM_LOG_FMT_LEVEL(skyline::logger::LogLevel::Fatal, fmt, __VA_ARGS__)

namespace skyline::core {

logger::Logger& getSystemLogger();

}  // namespace skyline::core
