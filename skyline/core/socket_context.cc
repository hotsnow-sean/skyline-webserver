#include "socket_context.h"

namespace skyline::core::detail {

SocketContext::SocketContext(EventLoop& loop, int fd, uint32_t events)
    : Channel(loop, fd), events(events) {}

bool SocketContext::HandleWriteEvent() {
    if (write_buffer_.size() > 0) {
        auto bytes_write =
            ::write(fd(), write_buffer_.data(), write_buffer_.size());
        if (bytes_write < 0) return false;
        write_buffer_.Read(bytes_write);
    }
    return true;
}

}  // namespace skyline::core::detail
