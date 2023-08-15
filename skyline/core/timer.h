#pragma once

#include <functional>
#include <set>
#include <shared_mutex>

namespace skyline::core {

namespace detail {

struct TimerID {
    std::time_t expire{};
    uint64_t id{};
};
bool operator<(const TimerID& lhd, const TimerID& rhd);

struct TimerNode : public TimerID {
    using Callback = std::function<void(uint64_t)>;

    TimerNode(std::time_t expire, uint64_t id, Callback func);

    Callback func;
};

}  // namespace detail

class Timer {
public:
    using timer_id_t = uint64_t;
    timer_id_t addTimer(std::time_t msec, detail::TimerNode::Callback&& func,
                        bool recurring = false);

    bool delTimer(timer_id_t id);

    void checkTimer();

    /// @brief 返回当前到最近的定时器触发时间的间隔
    /// @return 有定时器情况下返回距离触发的时间(最小为0)，否则返回-1
    std::time_t timeToSleep();

private:
    void recurringWrapper(std::time_t msec, uint64_t id,
                          detail::TimerNode::Callback func);

private:
    std::set<detail::TimerNode, std::less<>> _timers;
    std::unordered_map<uint64_t, detail::TimerID> _ids;
    std::shared_mutex _mtx;
};

}  // namespace skyline::core
