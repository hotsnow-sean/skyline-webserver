#pragma once

#include <sys/epoll.h>

#include <map>
#include <memory>
#include <thread>

#include "timer.h"

namespace skyline::core {

namespace detail {

class SocketContext;

}

// EventLoop 管理一个 epoll
// 设计用于在一个线程内使用 Loop 方法循环等待事件
// 同时提供线程唤醒、定时器管理的功能
class EventLoop {
public:
    EventLoop();

    // Wait events then handle events.
    void Loop();

    void Stop();

    // wakeup `epoll_wait` to break loop.
    void Wakeup();

    void AddSocketContext(std::shared_ptr<detail::SocketContext> ctx);

    void UpdateSocketContext(int fd, uint32_t events);
    void RemoveSocketContext(int fd);

    Timer::timer_id_t AddTimer(std::time_t msec,
                               detail::TimerNode::Callback&& func);

    void RemoveTimer(Timer::timer_id_t id);

    void RunInLoop(std::function<void()> func);

    bool isQuit() { return quit_; }

private:
    void DoPendingFuncs();

private:
    int epfd_{-1};
    std::atomic_bool quit_{false};
    int wakeup_fd_{-1};
    ::epoll_event* events_{nullptr};
    Timer timer_;

    std::mutex pending_mtx_;
    std::vector<std::function<void()>> pending_funcs_;

    std::thread::id tid_;  // 用于记录 Loop 函数运行所在的线程 id

    // fd -> ctx 一个文件描述符对应一个上下文
    std::map<int, std::shared_ptr<detail::SocketContext>> socket_ctxs_;
};

}  // namespace skyline::core
