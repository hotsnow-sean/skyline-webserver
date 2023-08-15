#include "event_loop.h"

#include <sys/eventfd.h>

#include <cstring>

#include "socket_context.h"
#include "utils.h"

static constexpr int kMaxEvents = 1000;

namespace skyline::core {

EventLoop::EventLoop()
    : epfd_(epoll_create1(0)),
      wakeup_fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      events_(new ::epoll_event[kMaxEvents]()) {
    if (epfd_ == -1) {
        SYSTEM_LOG_FATAL << "epoll create fail: " << strerror(errno);
        throw "epoll create fail";
    }
    if (wakeup_fd_ == -1) {
        SYSTEM_LOG_FATAL << "eventfd create fail: " << strerror(errno);
        throw "eventfd create fail";
    }
    ::epoll_event ev{.events = EPOLLIN | EPOLLPRI, .data = {.fd = wakeup_fd_}};
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, wakeup_fd_, &ev) == -1) {
        SYSTEM_LOG_FATAL << "eventfd add into epoll fail: " << strerror(errno);
        throw "eventfd add into epoll fail";
    }
}

void EventLoop::Loop() {
    tid_ = std::this_thread::get_id();
    while (!quit_) {
        int nfds = epoll_wait(epfd_, events_, kMaxEvents, timer_.timeToSleep());
        if (nfds == -1) {
            if (errno == EINTR) continue;
            SYSTEM_LOG_FATAL << "epoll wait error: " << strerror(errno);
            break;
        }
        for (int i = 0; i < nfds; ++i) {
            const ::epoll_event &cur_ev = events_[i];
            if (cur_ev.data.fd == wakeup_fd_) {
                ::eventfd_t tmp;
                ::eventfd_read(wakeup_fd_, &tmp);
                continue;
            }
            // 移除stream之前保证删除epoll监听，因此无需判断是否为空
            const auto cur_conn = socket_ctxs_[cur_ev.data.fd];
            if (cur_ev.events & EPOLLERR) {
                if (errno == EINTR) continue;
                SYSTEM_LOG_ERROR << "epoll error event: " << cur_ev.data.fd
                                 << " " << strerror(errno);
                RemoveSocketContext(cur_ev.data.fd);
                continue;
            }
            if (cur_ev.events & EPOLLOUT) {
                if (!cur_conn->HandleWriteEvent()) {
                    SYSTEM_LOG_ERROR << "epoll write fail: " << cur_ev.data.fd
                                     << " " << strerror(errno);
                    RemoveSocketContext(cur_ev.data.fd);
                } else if (!cur_conn->NeedWrite()) {
                    cur_conn->events &= ~EPOLLOUT;
                    UpdateSocketContext(cur_ev.data.fd, cur_conn->events);
                }
            }
            if (cur_ev.events & (EPOLLIN | EPOLLPRI)) {
                if (!cur_conn->HandleReadEvent()) {
                    RemoveSocketContext(cur_ev.data.fd);
                }
            }
        }
        DoPendingFuncs();
        timer_.checkTimer();
    }
}

void EventLoop::Stop() {
    if (quit_) return;
    quit_ = true;
    Wakeup();
}

void EventLoop::Wakeup() { ::eventfd_write(wakeup_fd_, 1); }

// 为确保线程安全，fd的添加应该放在loop中执行
void EventLoop::AddSocketContext(std::shared_ptr<detail::SocketContext> ctx) {
    RunInLoop([this, ctx = std::move(ctx)]() {
        if (!ctx || ctx->fd() < 0 || socket_ctxs_[ctx->fd()]) return;
        epoll_event ev{
            .events = ctx->events,
            .data = {.fd = ctx->fd()},
        };
        socket_ctxs_[ctx->fd()] = ctx;
        if (epoll_ctl(epfd_, EPOLL_CTL_ADD, ctx->fd(), &ev) == -1) {
            socket_ctxs_[ctx->fd()].reset();
            SYSTEM_LOG_ERROR << "epoll add fail: [" << ctx->fd() << "] "
                             << strerror(errno);
            return;
        }
        SYSTEM_LOG_DEBUG << "[" << ctx->fd() << "] added into epoll";
    });
}

void EventLoop::UpdateSocketContext(int fd, uint32_t events) {
    if (fd < 0 || !socket_ctxs_[fd]) return;
    epoll_event ev{
        .events = events,
        .data = {.fd = fd},
    };
    if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
        SYSTEM_LOG_ERROR << "epoll event mod fail: [" << fd << "] "
                         << strerror(errno);
        RemoveSocketContext(fd);
    }
}

// 为确保线程安全，fd的删除应该放在loop中执行
void EventLoop::RemoveSocketContext(int fd) {
    RunInLoop([this, fd]() {
        if (fd >= 0 && socket_ctxs_[fd]) {
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
            socket_ctxs_[fd].reset();
            SYSTEM_LOG_DEBUG << "[" << fd << "] del from epoll";
        }
    });
}

Timer::timer_id_t EventLoop::AddTimer(std::time_t msec,
                                      detail::TimerNode::Callback &&func) {
    return timer_.addTimer(msec, std::move(func));
}

void EventLoop::RemoveTimer(Timer::timer_id_t id) { timer_.delTimer(id); }

void EventLoop::RunInLoop(std::function<void()> func) {
    if (tid_ == std::this_thread::get_id()) {
        func();
    } else {
        std::lock_guard lock(pending_mtx_);
        pending_funcs_.push_back(std::move(func));
        Wakeup();
    }
}

void EventLoop::DoPendingFuncs() {
    std::vector<std::function<void()>> tmp;
    {
        std::lock_guard lock(pending_mtx_);
        pending_funcs_.swap(tmp);
    }
    for (auto &func : tmp) {
        func();
    }
}

}  // namespace skyline::core
