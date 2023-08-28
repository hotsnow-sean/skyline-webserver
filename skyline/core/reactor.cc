#include "reactor.h"

static const unsigned int kThreadNum = std::thread::hardware_concurrency();

namespace skyline::core {

Reactor::Reactor(unsigned int sub_reactor_num)
    : sub_reactors_(std::min(kThreadNum, sub_reactor_num)) {}

Reactor::~Reactor() { Stop(); }

void Reactor::Start() {
    // start sub reactor loop
    for (auto& reactor : sub_reactors_) {
        sub_threads_.emplace_back([&reactor]() { reactor.Loop(); });
    }
    main_reactor.Loop();
}

void Reactor::Stop() {
    if (main_reactor.isQuit()) return;
    main_reactor.Stop();
    for (auto& reactor : sub_reactors_) {
        reactor.Stop();
    }
    for (auto& t : sub_threads_) {
        t.join();
    }
}

EventLoop& Reactor::NextLoop() noexcept {
    return this->sub_reactors_.empty()
               ? this->main_reactor
               : this->sub_reactors_[this->cur_reactor_id_++ %
                                     this->sub_reactors_.size()];
}

}  // namespace skyline::core
