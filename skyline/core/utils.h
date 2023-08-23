#pragma once

#include "../logger/log.h"

#define SYSTEM_LOG_LEVEL(lv)                          \
    if (skyline::core::getSystemLogger().level <= lv) \
    skyline::logger::LogEventWrap(skyline::core::getSystemLogger(), lv)

#define SYSTEM_LOG_DEBUG SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::DEBUG)
#define SYSTEM_LOG_INFO SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::INFO)
#define SYSTEM_LOG_WARN SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::WARN)
#define SYSTEM_LOG_ERROR SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::ERROR)
#define SYSTEM_LOG_FATAL SYSTEM_LOG_LEVEL(skyline::logger::LogLevel::FATAL)

namespace skyline::core {

logger::Logger& getSystemLogger();

template <typename... Args>
void SYSTEM_LOG_FMT(skyline::logger::Logger& logger,
                    skyline::logger::LogLevel level, const char* fmt,
                    Args&&... args) {
    skyline::logger::LOG_FMT(logger, level, fmt, args...);
}

#define _FUNCTION(name)                                                        \
    template <typename... Args>                                                \
    void SYSTEM_LOG_FMT_##name(skyline::logger::Logger& logger,                \
                               const char* fmt, Args&&... args) {              \
        SYSTEM_LOG_FMT(logger, skyline::logger::LogLevel::name, fmt, args...); \
    }
FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION

}  // namespace skyline::core
