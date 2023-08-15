#pragma once

#include <condition_variable>
#include <thread>

#include "log.h"

namespace skyline::logger {

class FixedBuffer;

// 异步输出地：纯虚类，子类需要实现将日志添加以及持久化的函数
// 它将在缓冲区满或者满足超时条件时调用子类的方法持久化日志
// 除纯虚函数外，该基类内部操作保证线程安全
class AsyncAppender : virtual public LogAppender {
public:
    AsyncAppender();
    virtual ~AsyncAppender() = 0;

    void log(const Logger& logger, LogLevel level,
             const LogEvent& event) override;

    // 子类析构时必须调用此函数，避免线程内部访问子类方法错误
    void stop();

private:
    virtual void append(const char* data, size_t length) = 0;
    virtual void flush() = 0;

private:
    std::unique_ptr<FixedBuffer> _current_buffer;
    std::unique_ptr<FixedBuffer> _next_buffer;
    std::vector<std::unique_ptr<FixedBuffer>> _buffers;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic_bool _stop{false};
};

class AsyncFileAppender : virtual public AsyncAppender {
public:
    AsyncFileAppender(const std::string& filename, bool thread_safe);
    ~AsyncFileAppender();

    void append(const char* data, size_t length) override;
    void flush() override;

protected:
    std::ofstream _filestream;
    std::mutex* _mutex{nullptr};
};

/// @brief
/// 异步滚动日志，limit_size应大于4M（缓冲区影响）
/// 其次如果日志滚动速度过小于到1s则失效（文件名影响）
class AsyncRollFileAppender : virtual public AsyncAppender {
public:
    AsyncRollFileAppender(std::string basename, size_t limit_size,
                          bool thread_safe);
    ~AsyncRollFileAppender();

    void append(const char* data, size_t length) override;
    void flush() override;

private:
    void rollFile();

private:
    std::string _basename;
    std::string _filename;
    std::ofstream _filestream;
    size_t _file_size{0};
    std::mutex* _mutex{nullptr};
    size_t _limit_size{0};
};

}  // namespace skyline::logger
