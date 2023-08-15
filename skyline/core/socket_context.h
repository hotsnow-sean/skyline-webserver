#pragma once

#include "buffer.h"
#include "channel.h"

namespace skyline::core::detail {

// socket 上下文，维护一个 socket fd 被监听的事件
class SocketContext : public Channel {
public:
    SocketContext(EventLoop& loop, int fd, uint32_t events);

    // 读取数据，返回当前socket是否继续可用
    // 如果对端关闭，则无法继续使用
    virtual bool HandleReadEvent() = 0;

    // 将剩余数据发送，返回当前socket是否继续可用
    // 如果对端关闭，则无法继续使用
    bool HandleWriteEvent();

    bool NeedWrite() { return write_buffer_.size() > 0; }

public:
    uint32_t events{0};

protected:
    Buffer write_buffer_;
};

}  // namespace skyline::core::detail
