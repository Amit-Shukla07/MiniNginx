#pragma once

#include <cstdint>
#include <functional>

namespace mininginx::core {

enum class EventType : uint32_t {
    None = 0,
    Read = 1 << 0,
    Write = 1 << 1,
    Error = 1 << 2,
    Hangup = 1 << 3,
    EdgeTriggered = 1 << 4
};

constexpr EventType operator|(EventType a, EventType b) noexcept {
    return static_cast<EventType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr EventType operator&(EventType a, EventType b) noexcept {
    return static_cast<EventType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr EventType operator~(EventType a) noexcept {
    return static_cast<EventType>(~static_cast<uint32_t>(a));
}

constexpr bool has_event(uint32_t mask, EventType type) noexcept {
    return (mask & static_cast<uint32_t>(type)) != 0;
}

constexpr bool has_event(EventType mask, EventType type) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(type)) != 0;
}

struct FiredEvent {
    int fd{-1};
    uint32_t events{0};
};

using EventCallback = std::function<void(int fd, uint32_t events)>;

}  // namespace mininginx::core
