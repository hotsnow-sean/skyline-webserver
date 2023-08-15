#include "logger/async_log.h"

using namespace skyline::logger;

int main() {
    // root 日志器默认包含一个控制台输出地
    // 输出格式为 时间 线程号 等级 文件名:行号 信息
    auto& logger = getRootLogger();

    // 测试控制台日志输出
    SKYLINE_LOG_DEBUG(logger) << "a debug log";
    SKYLINE_LOG_INFO(logger) << "a info log";
    SKYLINE_LOG_FMT_DEBUG(logger, "format debug %d", 123);
    SKYLINE_LOG_FMT_INFO(logger, "format info %d", 456);

    // 测试等级控制
    logger.level = LogLevel::Info;
    SKYLINE_LOG_DEBUG(logger) << "should not output debug log";
    SKYLINE_LOG_INFO(logger) << "should output info log";

    // 测试格式化器
    auto simple_formatter = std::make_shared<LogFormatter>("%c%T[%p]%T%m%n");
    auto stdout_appender =
        std::make_shared<StdoutLogAppender>(simple_formatter);
    auto& mini_logger = getLogger("custom");  // 后续测试的输出地都加入此日志器
    mini_logger.addAppender(stdout_appender);

    SKYLINE_LOG_INFO(mini_logger) << "should output like: `custom [INFO] ...`";

    // 测试简单版文件日志输出
    auto file_appender = std::make_shared<FileLogAppender>("logger_test.log");
    file_appender->setFormatter(simple_formatter);  // 可以设置新的格式化器
    mini_logger.addAppender(file_appender);

    SKYLINE_LOG_INFO(mini_logger)
        << "should output into console and file `logger_test.log`";

    // 测试基本异步文件日志
    auto async_appender =
        std::make_shared<AsyncFileAppender>("logger_async_test.log", false);
    // 这里使用默认格式化器
    mini_logger.addAppender(async_appender);

    // 下面的这条日志将出现在3个地方，且异步日志中的输出格式与其它两个不同
    SKYLINE_LOG_INFO(mini_logger)
        << "should output into console and file `logger_async_test.log` and "
           "`logger_test.log`";

    // 异步滚动日志不测了，使用方法基本相同（要达到滚动的效果需要不少的日志，且大于1秒的输出时间才能看到效果）

    return 0;
}
