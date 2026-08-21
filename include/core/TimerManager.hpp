#pragma once

#include "core/Timer.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

namespace mininginx::core {

class TimerManager {
public:
    TimerManager() = default;
    ~TimerManager() = default;

    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;
    TimerManager(TimerManager&&) noexcept = delete;
    TimerManager& operator=(TimerManager&&) noexcept = delete;

    TimerId add_timer(Milliseconds delay, TimerCallback cb, bool repeated = false);
    void cancel_timer(TimerId id);
    void handle_expired_timers();
    [[nodiscard]] int get_next_timeout_ms();
    [[nodiscard]] size_t active_timers_count() const noexcept;

private:
    struct TimerComparator {
        bool operator()(const std::shared_ptr<Timer>& a, const std::shared_ptr<Timer>& b) const {
            return *a > *b;
        }
    };

    TimerId next_id_{1};
    std::priority_queue<
        std::shared_ptr<Timer>,
        std::vector<std::shared_ptr<Timer>>,
        TimerComparator
    > min_heap_;
    std::unordered_map<TimerId, std::shared_ptr<Timer>> active_timers_;
    mutable std::mutex mutex_;
};

}  // namespace mininginx::core
