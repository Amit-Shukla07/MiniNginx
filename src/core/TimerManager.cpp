#include "core/TimerManager.hpp"

#include <algorithm>
#include <vector>

namespace mininginx::core {

TimerId TimerManager::add_timer(Milliseconds delay, TimerCallback cb, bool repeated) {
    Timestamp now = Clock::now();
    auto timer = std::make_shared<Timer>(Timer{
        .id = 0,
        .expiry = now + delay,
        .interval = delay,
        .repeated = repeated,
        .cancelled = false,
        .callback = std::move(cb)
    });

    std::lock_guard<std::mutex> lock(mutex_);
    TimerId id = next_id_++;
    timer->id = id;
    active_timers_[id] = timer;
    min_heap_.push(timer);
    return id;
}

void TimerManager::cancel_timer(TimerId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_timers_.find(id);
    if (it != active_timers_.end()) {
        it->second->cancelled = true;
        active_timers_.erase(it);
    }
}

void TimerManager::handle_expired_timers() {
    Timestamp now = Clock::now();
    std::vector<TimerCallback> callbacks_to_run;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!min_heap_.empty()) {
            auto top = min_heap_.top();
            if (top->cancelled) {
                min_heap_.pop();
                continue;
            }

            if (top->expiry > now) {
                break;
            }

            min_heap_.pop();

            if (top->callback) {
                callbacks_to_run.push_back(top->callback);
            }

            if (top->repeated && !top->cancelled) {
                top->expiry = now + top->interval;
                min_heap_.push(top);
            } else {
                active_timers_.erase(top->id);
            }
        }
    }

    // Execute callbacks outside lock to prevent deadlocks
    for (const auto& cb : callbacks_to_run) {
        cb();
    }
}

int TimerManager::get_next_timeout_ms() {
    Timestamp now = Clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    while (!min_heap_.empty()) {
        auto top = min_heap_.top();
        if (top->cancelled) {
            min_heap_.pop();
            continue;
        }

        if (top->expiry <= now) {
            return 0;
        }

        auto diff = std::chrono::duration_cast<Milliseconds>(top->expiry - now).count();
        return static_cast<int>(std::min(diff, static_cast<decltype(diff)>(10000)));
    }

    return 100;
}

size_t TimerManager::active_timers_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_timers_.size();
}

}  // namespace mininginx::core
