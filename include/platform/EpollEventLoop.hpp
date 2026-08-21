#pragma once

#include "core/EventLoop.hpp"

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

#ifdef MININGINX_PLATFORM_LINUX
#include <sys/epoll.h>
#endif

namespace mininginx::platform {

class EpollEventLoop final : public core::EventLoop {
public:
    EpollEventLoop();
    ~EpollEventLoop() override;

    EpollEventLoop(const EpollEventLoop&) = delete;
    EpollEventLoop& operator=(const EpollEventLoop&) = delete;
    EpollEventLoop(EpollEventLoop&&) noexcept = delete;
    EpollEventLoop& operator=(EpollEventLoop&&) noexcept = delete;

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

    int epoll_fd_{-1};
    std::atomic<bool> running_{false};
#ifdef MININGINX_PLATFORM_LINUX
    std::vector<struct epoll_event> event_list_;
#endif
    std::unordered_map<int, HandlerEntry> handlers_;
    std::mutex mutex_;
};

}  // namespace mininginx::platform
