#include "utils.h"

#include <iostream>

namespace skyline::core {

class SafeStdoutLogAppender : virtual public logger::LogAppender {
public:
    using logger::LogAppender::LogAppender;

    void log(const logger::Logger& logger, logger::LogLevel level,
             const logger::LogEvent& event) override {
        if (level < this->level) return;
        std::lock_guard lock(_mtx);
        _formatter->format(std::cout, logger, level, event);
    }

private:
    std::mutex _mtx;
};

logger::Logger& getSystemLogger() {
    static logger::Logger system_logger = []() -> logger::Logger {
        logger::Logger logger("system");
        logger.addAppender(std::make_shared<SafeStdoutLogAppender>());
        return logger;
    }();
    return system_logger;
}

}  // namespace skyline::core
