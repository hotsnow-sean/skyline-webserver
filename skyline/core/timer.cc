#include "timer.h"

namespace skyline::core {

static std::time_t getTick() {
    using namespace std::chrono;
    auto sc = time_point_cast<microseconds>(steady_clock::now());
    auto temp = duration_cast<milliseconds>(sc.time_since_epoch());
    return temp.count();
}

static Timer::timer_id_t getID() {
    static Timer::timer_id_t id = 0;
    return ++id;
}

namespace detail {

bool operator<(const TimerID& lhd, const TimerID& rhd) {
    if (lhd.expire == rhd.expire) return lhd.id < rhd.id;
    return lhd.expire < rhd.expire;
}

TimerNode::TimerNode(std::time_t expire, uint64_t id, Callback func)
    : TimerID{.expire = expire, .id = id}, func(std::move(func)) {}

}  // namespace detail

Timer::timer_id_t Timer::addTimer(std::time_t msec,
                                  detail::TimerNode::Callback&& func,
                                  bool recurring) {
    std::lock_guard lock(_mtx);
    auto id = getID();
    if (recurring) {
        detail::TimerNode::Callback wrap_func = std::bind(
            &Timer::recurringWrapper, this, msec, id, std::move(func));
        auto ok = _timers.emplace(getTick() + msec, id, std::move(wrap_func));
        _ids[id] = *ok.first;
        return id;
    } else {
        auto ok = _timers.emplace(getTick() + msec, id, std::move(func));
        _ids[id] = *ok.first;
        return id;
    }
}

bool Timer::delTimer(timer_id_t id) {
    std::lock_guard lock(_mtx);
    auto node_id = _ids.find(id);
    if (node_id == _ids.end()) return false;
    auto iter = _timers.find(node_id->second);
    if (iter == _timers.end()) return false;
    _timers.erase(iter);
    return true;
}

void Timer::checkTimer() {
    std::vector<std::function<void()>> funcs;
    {
        std::lock_guard lock(_mtx);
        for (auto it = _timers.begin(); it != _timers.end();) {
            if (it->expire > getTick()) break;
            funcs.push_back(std::bind(std::move(it->func), it->id));
            it = _timers.erase(it);
        }
    }
    for (auto& f : funcs) f();
}

std::time_t Timer::timeToSleep() {
    std::shared_lock lock(_mtx);
    if (_timers.empty()) return -1;
    auto dist = _timers.begin()->expire - getTick();
    return dist > 0 ? dist : 0;
}

void Timer::recurringWrapper(std::time_t msec, uint64_t id,
                             detail::TimerNode::Callback func) {
    func(id);
    std::lock_guard lock(_mtx);
    auto ok = this->_timers.emplace(
        getTick() + msec, id,
        std::bind(&Timer::recurringWrapper, this, msec, id, std::move(func)));
    this->_ids[id] = *ok.first;
}

}  // namespace skyline::core
