#pragma once

#include "event_loop.h"

namespace skyline::core {

class Reactor {
public:
    // 默认子反应堆数目为0，即单反应堆模式
    explicit Reactor(unsigned int sub_reactor_num = 0);
    ~Reactor();

    void Start();
    void Stop();

    EventLoop& NextLoop() noexcept;

public:
    EventLoop main_reactor;

private:
    std::vector<EventLoop> sub_reactors_;
    std::vector<std::thread> sub_threads_;

    // 记录当前应使用的子反应堆号，用于平均分配 fd
    size_t cur_reactor_id_{0};
};

}  // namespace skyline::core
