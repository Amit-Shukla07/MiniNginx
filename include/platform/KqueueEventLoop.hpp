#pragma once

#include "core/EventLoop.hpp"

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mininginx::platform {

class KqueueEventLoop final : public core::EventLoop {
public:
    KqueueEventLoop();
    ~KqueueEventLoop() override;

    KqueueEventLoop(const KqueueEventLoop&) = delete;
    KqueueEventLoop& operator=(const KqueueEventLoop&) = delete;
    KqueueEventLoop(KqueueEventLoop&&) noexcept = delete;
    KqueueEventLoop& operator=(KqueueEventLoop&&) noexcept = delete;

    bool add_fd(int fd, uint32_t events, core::EventCallback callback) override;
    bool modify_fd(int fd, uint32_t events) override;
    bool remove_fd(int fd) override;

    void poll_and_dispatch(int timeout_ms) override;
    void run() override;
    void stop() override;
    [[nodiscard]] bool is_running() const override { return running_.load(std::memory_order_relaxed); }

private:
    struct HandlerEntry {
        uint32_t events{0};
        core::EventCallback callback;
    };

    int kq_fd_{-1};
    std::atomic<bool> running_{false};
    std::vector<struct kevent> event_list_;
    std::unordered_map<int, HandlerEntry> handlers_;
    std::mutex mutex_;
};

}  // namespace mininginx::platform
