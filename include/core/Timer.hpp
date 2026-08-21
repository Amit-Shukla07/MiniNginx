#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

namespace mininginx::core {

using TimerId = uint64_t;
using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point;
using Milliseconds = std::chrono::milliseconds;
using TimerCallback = std::function<void()>;

struct Timer {
    TimerId id{0};
    Timestamp expiry;
    Milliseconds interval{0};
    bool repeated{false};
    bool cancelled{false};
    TimerCallback callback;

    bool operator>(const Timer& other) const noexcept {
        return expiry > other.expiry;
    }
};

}  // namespace mininginx::core
