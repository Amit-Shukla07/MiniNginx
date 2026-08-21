#include "platform/EpollEventLoop.hpp"

#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#ifdef MININGINX_PLATFORM_LINUX
#include <sys/epoll.h>
#endif

namespace mininginx::platform {

#ifdef MININGINX_PLATFORM_LINUX

EpollEventLoop::EpollEventLoop()
    : epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)), event_list_(1024) {
    if (epoll_fd_ < 0) {
        std::cerr << "Failed to create epoll descriptor: " << std::strerror(errno) << std::endl;
    }
}

EpollEventLoop::~EpollEventLoop() {
    stop();
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

static uint32_t to_epoll_events(uint32_t events) {
    uint32_t epoll_events = 0;
    if (core::has_event(events, core::EventType::Read)) {
        epoll_events |= EPOLLIN | EPOLLRDHUP;
    }
    if (core::has_event(events, core::EventType::Write)) {
        epoll_events |= EPOLLOUT;
    }
    if (core::has_event(events, core::EventType::EdgeTriggered)) {
        epoll_events |= EPOLLET;
    }
    return epoll_events;
}

bool EpollEventLoop::add_fd(int fd, uint32_t events, core::EventCallback callback) {
    if (epoll_fd_ < 0 || fd < 0) return false;

    struct epoll_event ev{};
    ev.events = to_epoll_events(events);
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[fd] = HandlerEntry{events, std::move(callback)};
    }
    return true;
}

bool EpollEventLoop::modify_fd(int fd, uint32_t events) {
    if (epoll_fd_ < 0 || fd < 0) return false;

    struct epoll_event ev{};
    ev.events = to_epoll_events(events);
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(fd);
        if (it != handlers_.end()) {
            it->second.events = events;
        }
    }
    return true;
}

bool EpollEventLoop::remove_fd(int fd) {
    if (epoll_fd_ < 0 || fd < 0) return false;

    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.erase(fd);
    }
    return true;
}

void EpollEventLoop::poll_and_dispatch(int timeout_ms) {
    if (epoll_fd_ < 0) return;

    int nfds = ::epoll_wait(
        epoll_fd_,
        event_list_.data(),
        static_cast<int>(event_list_.size()),
        timeout_ms
    );

    if (nfds < 0) {
        if (errno == EINTR) {
            return;
        }
        return;
    }

    struct PendingDispatch {
        int fd;
        uint32_t fired;
        core::EventCallback cb;
    };
    std::vector<PendingDispatch> ready_list;
    ready_list.reserve(static_cast<size_t>(nfds));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int i = 0; i < nfds; ++i) {
            const struct epoll_event& ev = event_list_[static_cast<size_t>(i)];
            int fd = ev.data.fd;

            uint32_t fired = 0;
            if ((ev.events & EPOLLIN) != 0) {
                fired |= static_cast<uint32_t>(core::EventType::Read);
            }
            if ((ev.events & EPOLLOUT) != 0) {
                fired |= static_cast<uint32_t>(core::EventType::Write);
            }
            if ((ev.events & (EPOLLRDHUP | EPOLLHUP)) != 0) {
                fired |= static_cast<uint32_t>(core::EventType::Hangup);
            }
            if ((ev.events & EPOLLERR) != 0) {
                fired |= static_cast<uint32_t>(core::EventType::Error);
            }

            auto it = handlers_.find(fd);
            if (it != handlers_.end() && it->second.callback) {
                ready_list.push_back({fd, fired, it->second.callback});
            }
        }
    }

    for (const auto& item : ready_list) {
        item.cb(item.fd, item.fired);
    }
}

void EpollEventLoop::run() {
    running_.store(true, std::memory_order_release);
    while (running_.load(std::memory_order_acquire)) {
        poll_and_dispatch(100);
    }
}

void EpollEventLoop::stop() {
    running_.store(false, std::memory_order_release);
}

#else

EpollEventLoop::EpollEventLoop() = default;
EpollEventLoop::~EpollEventLoop() = default;
bool EpollEventLoop::add_fd(int, uint32_t, core::EventCallback) { return false; }
bool EpollEventLoop::modify_fd(int, uint32_t) { return false; }
bool EpollEventLoop::remove_fd(int) { return false; }
void EpollEventLoop::poll_and_dispatch(int) {}
void EpollEventLoop::run() {}
void EpollEventLoop::stop() {}

#endif

}  // namespace mininginx::platform

#ifdef MININGINX_PLATFORM_LINUX
namespace mininginx::core {
std::unique_ptr<EventLoop> EventLoop::create() {
    return std::make_unique<platform::EpollEventLoop>();
}
}  // namespace mininginx::core
#endif
