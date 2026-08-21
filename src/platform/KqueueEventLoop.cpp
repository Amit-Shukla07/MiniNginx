#include "platform/KqueueEventLoop.hpp"

#include <unistd.h>

#include <cerrno>
#include <iostream>

namespace mininginx::platform {

KqueueEventLoop::KqueueEventLoop() : kq_fd_(::kqueue()), event_list_(1024) {
    if (kq_fd_ < 0) {
        std::cerr << "Failed to create kqueue descriptor: " << std::strerror(errno) << std::endl;
    }
}

KqueueEventLoop::~KqueueEventLoop() {
    stop();
    if (kq_fd_ >= 0) {
        ::close(kq_fd_);
        kq_fd_ = -1;
    }
}

bool KqueueEventLoop::add_fd(int fd, uint32_t events, core::EventCallback callback) {
    if (kq_fd_ < 0 || fd < 0) return false;

    struct kevent changes[2];
    int n = 0;
    const bool edge = core::has_event(events, core::EventType::EdgeTriggered);
    const u_short flags = static_cast<u_short>(EV_ADD | EV_ENABLE | (edge ? EV_CLEAR : 0));

    if (core::has_event(events, core::EventType::Read)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, flags, 0, 0, nullptr);
    }
    if (core::has_event(events, core::EventType::Write)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, flags, 0, 0, nullptr);
    }

    if (n > 0) {
        int ret = ::kevent(kq_fd_, changes, n, nullptr, 0, nullptr);
        if (ret < 0) {
            return false;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[fd] = HandlerEntry{events, std::move(callback)};
    }
    return true;
}

bool KqueueEventLoop::modify_fd(int fd, uint32_t events) {
    if (kq_fd_ < 0 || fd < 0) return false;

    uint32_t old_events = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(fd);
        if (it == handlers_.end()) {
            return false;
        }
        old_events = it->second.events;
    }

    struct kevent changes[4];
    int n = 0;
    const bool edge = core::has_event(events, core::EventType::EdgeTriggered);
    const u_short add_flags = static_cast<u_short>(EV_ADD | EV_ENABLE | (edge ? EV_CLEAR : 0));

    const bool was_read = core::has_event(old_events, core::EventType::Read);
    const bool want_read = core::has_event(events, core::EventType::Read);
    if (want_read && !was_read) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, add_flags, 0, 0, nullptr);
    } else if (!want_read && was_read) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    }

    const bool was_write = core::has_event(old_events, core::EventType::Write);
    const bool want_write = core::has_event(events, core::EventType::Write);
    if (want_write && !was_write) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, add_flags, 0, 0, nullptr);
    } else if (!want_write && was_write) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    }

    if (n > 0) {
        int ret = ::kevent(kq_fd_, changes, n, nullptr, 0, nullptr);
        if (ret < 0) {
            return false;
        }
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

bool KqueueEventLoop::remove_fd(int fd) {
    if (kq_fd_ < 0 || fd < 0) return false;

    uint32_t old_events = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(fd);
        if (it == handlers_.end()) {
            return false;
        }
        old_events = it->second.events;
        handlers_.erase(it);
    }

    struct kevent changes[2];
    int n = 0;

    if (core::has_event(old_events, core::EventType::Read)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    }
    if (core::has_event(old_events, core::EventType::Write)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    }

    if (n > 0) {
        ::kevent(kq_fd_, changes, n, nullptr, 0, nullptr);
    }

    return true;
}

void KqueueEventLoop::poll_and_dispatch(int timeout_ms) {
    if (kq_fd_ < 0) return;

    struct timespec ts{};
    struct timespec* pts = nullptr;
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        pts = &ts;
    }

    int nev = ::kevent(
        kq_fd_,
        nullptr,
        0,
        event_list_.data(),
        static_cast<int>(event_list_.size()),
        pts
    );

    if (nev < 0) {
        if (errno == EINTR) {
            return;
        }
        return;
    }

    // Collect callbacks with their flags to invoke safely
    struct PendingDispatch {
        int fd;
        uint32_t fired;
        core::EventCallback cb;
    };
    std::vector<PendingDispatch> ready_list;
    ready_list.reserve(static_cast<size_t>(nev));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int i = 0; i < nev; ++i) {
            const struct kevent& ev = event_list_[static_cast<size_t>(i)];
            int fd = static_cast<int>(ev.ident);

            uint32_t fired = 0;
            if (ev.filter == EVFILT_READ) {
                fired |= static_cast<uint32_t>(core::EventType::Read);
            }
            if (ev.filter == EVFILT_WRITE) {
                fired |= static_cast<uint32_t>(core::EventType::Write);
            }
            if ((ev.flags & EV_EOF) != 0) {
                fired |= static_cast<uint32_t>(core::EventType::Hangup);
            }
            if ((ev.flags & EV_ERROR) != 0) {
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

void KqueueEventLoop::run() {
    running_.store(true, std::memory_order_release);
    while (running_.load(std::memory_order_acquire)) {
        poll_and_dispatch(100);
    }
}

void KqueueEventLoop::stop() {
    running_.store(false, std::memory_order_release);
}

}  // namespace mininginx::platform

#ifdef MININGINX_PLATFORM_MACOS
namespace mininginx::core {
std::unique_ptr<EventLoop> EventLoop::create() {
    return std::make_unique<platform::KqueueEventLoop>();
}
}  // namespace mininginx::core
#endif
