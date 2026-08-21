#include "KqueueEventLoop.hpp"

#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

KqueueEventLoop::KqueueEventLoop() : kq_fd_(::kqueue()), eventList_(512) {
    if (kq_fd_ < 0) {
        std::cerr << "Failed to create kqueue\n";
    }
}

KqueueEventLoop::~KqueueEventLoop() {
    stop();
    if (kq_fd_ >= 0) {
        ::close(kq_fd_);
        kq_fd_ = -1;
    }
}

void KqueueEventLoop::add(int fd, int events, EventCallback cb) {
    if (kq_fd_ < 0 || fd < 0) return;

    struct kevent changes[2];
    int n = 0;

    if (events & EVENT_READ) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    }
    if (events & EVENT_WRITE) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    }

    if (n > 0) {
        ::kevent(kq_fd_, changes, n, nullptr, 0, nullptr);
    }

    handlers_[fd] = Handler{events, std::move(cb)};
}

void KqueueEventLoop::modify(int fd, int events) {
    if (kq_fd_ < 0 || fd < 0) return;

    auto it = handlers_.find(fd);
    if (it == handlers_.end()) return;

    int old_events = it->second.events;
    struct kevent changes[4];
    int n = 0;

    if ((events & EVENT_READ) && !(old_events & EVENT_READ)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    } else if (!(events & EVENT_READ) && (old_events & EVENT_READ)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    }

    if ((events & EVENT_WRITE) && !(old_events & EVENT_WRITE)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    } else if (!(events & EVENT_WRITE) && (old_events & EVENT_WRITE)) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    }

    if (n > 0) {
        ::kevent(kq_fd_, changes, n, nullptr, 0, nullptr);
    }

    it->second.events = events;
}

void KqueueEventLoop::remove(int fd) {
    if (kq_fd_ < 0 || fd < 0) return;

    auto it = handlers_.find(fd);
    if (it == handlers_.end()) return;

    int old_events = it->second.events;
    struct kevent changes[2];
    int n = 0;

    if (old_events & EVENT_READ) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    }
    if (old_events & EVENT_WRITE) {
        EV_SET(&changes[n++], static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    }

    if (n > 0) {
        ::kevent(kq_fd_, changes, n, nullptr, 0, nullptr);
    }

    handlers_.erase(it);
}

void KqueueEventLoop::poll(int timeout_ms) {
    if (kq_fd_ < 0) return;

    struct timespec ts{};
    struct timespec* pts = nullptr;
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000;
        pts = &ts;
    }

    running_ = true;
    int nev = ::kevent(kq_fd_, nullptr, 0, eventList_.data(), static_cast<int>(eventList_.size()), pts);

    if (nev < 0) {
        if (errno == EINTR) return;
        return;
    }

    for (int i = 0; i < nev; ++i) {
        const auto& ev = eventList_[static_cast<size_t>(i)];
        int fd = static_cast<int>(ev.ident);

        int fired = 0;
        if (ev.filter == EVFILT_READ) fired |= EVENT_READ;
        if (ev.filter == EVFILT_WRITE) fired |= EVENT_WRITE;
        if (ev.flags & (EV_EOF | EV_ERROR)) fired |= EVENT_ERROR;

        auto it = handlers_.find(fd);
        if (it != handlers_.end() && it->second.cb) {
            it->second.cb(fd, fired);
        }
    }
}

void KqueueEventLoop::stop() {
    running_ = false;
}
