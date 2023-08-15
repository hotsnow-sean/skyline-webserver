#include "channel.h"

#include "utils.h"

namespace skyline::core {

Channel::Channel(EventLoop& loop, int fd) : fd_(fd), loop_(loop) {}

Channel::~Channel() { Close(); }

void Channel::Close() {
    if (fd_ != -1) {
        SYSTEM_LOG_DEBUG << fd_ << " closed";
        ::close(fd_);
        fd_ = -1;
    }
}

}  // namespace skyline::core
