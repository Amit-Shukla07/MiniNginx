#pragma once

#include "core/Event.hpp"

#include <memory>

namespace mininginx::core {

class EventLoop {
public:
    virtual ~EventLoop() = default;

    // Multiplexing operations
    virtual bool add_fd(int fd, uint32_t events, EventCallback callback) = 0;
    virtual bool modify_fd(int fd, uint32_t events) = 0;
    virtual bool remove_fd(int fd) = 0;

    // Loop control
    virtual void poll_and_dispatch(int timeout_ms) = 0;
    virtual void run() = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual bool is_running() const = 0;

    // Platform-specific factory
    [[nodiscard]] static std::unique_ptr<EventLoop> create();
};

}  // namespace mininginx::core
