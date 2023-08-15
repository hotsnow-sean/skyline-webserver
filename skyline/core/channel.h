#pragma once

#include <functional>
#include <memory>

namespace skyline::core {

class ReadBuffer;
class EventLoop;

// 管理一个 socket fd 的生命周期
// 提供 消息回调注册，关闭前回调注册，消息发送方法
class Channel {
public:
    Channel(EventLoop& loop, int fd);
    Channel(const Channel&) = delete;
    Channel(Channel&&) = delete;
    virtual ~Channel();

    virtual void SendMassage(const std::string_view& massage) = 0;

    // 子类如果重写，最好在函数的最后手动调用该Close方法
    // 否则真正的关闭将在析构时发生
    virtual void Close();

    int fd() const noexcept { return fd_; }
    EventLoop& loop() const noexcept { return loop_; }

protected:
    EventLoop& loop_;

private:
    int fd_{-1};  // -1 代表不合法
};

}  // namespace skyline::core
