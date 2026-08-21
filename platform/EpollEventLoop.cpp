#include "EpollEventLoop.hpp"

#include <unistd.h>
#include <cerrno>
#include <iostream>

#ifdef __linux__

EpollEventLoop::EpollEventLoop()
    : epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)), eventList_(512) {
    if (epoll_fd_ < 0) {
        std::cerr << "Failed to create epoll\n";
    }
}

EpollEventLoop::~EpollEventLoop() {
    stop();
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

static uint32_t to_epoll(int events) {
    uint32_t ep = 0;
    if (events & EVENT_READ) ep |= EPOLLIN | EPOLLRDHUP;
    if (events & EVENT_WRITE) ep |= EPOLLOUT;
    return ep;
}

void EpollEventLoop::add(int fd, int events, EventCallback cb) {
    if (epoll_fd_ < 0 || fd < 0) return;

    struct epoll_event ev{};
    ev.events = to_epoll(events);
    ev.data.fd = fd;

    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    handlers_[fd] = Handler{events, std::move(cb)};
}

void EpollEventLoop::modify(int fd, int events) {
    if (epoll_fd_ < 0 || fd < 0) return;

    struct epoll_event ev{};
    ev.events = to_epoll(events);
    ev.data.fd = fd;

    ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
    handlers_[fd].events = events;
}

void EpollEventLoop::remove(int fd) {
    if (epoll_fd_ < 0 || fd < 0) return;

    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    handlers_.erase(fd);
}

void EpollEventLoop::poll(int timeout_ms) {
    if (epoll_fd_ < 0) return;

    running_ = true;
    int nfds = ::epoll_wait(epoll_fd_, eventList_.data(), static_cast<int>(eventList_.size()), timeout_ms);

    if (nfds < 0) {
        if (errno == EINTR) return;
        return;
    }

    for (int i = 0; i < nfds; ++i) {
        const auto& ev = eventList_[static_cast<size_t>(i)];
        int fd = ev.data.fd;

        int fired = 0;
        if (ev.events & EPOLLIN) fired |= EVENT_READ;
        if (ev.events & EPOLLOUT) fired |= EVENT_WRITE;
        if (ev.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) fired |= EVENT_ERROR;

        auto it = handlers_.find(fd);
        if (it != handlers_.end() && it->second.cb) {
            it->second.cb(fd, fired);
        }
    }
}

void EpollEventLoop::stop() {
    running_ = false;
}

#else

EpollEventLoop::EpollEventLoop() = default;
EpollEventLoop::~EpollEventLoop() = default;
void EpollEventLoop::add(int, int, EventCallback) {}
void EpollEventLoop::modify(int, int) {}
void EpollEventLoop::remove(int) {}
void EpollEventLoop::poll(int) {}
void EpollEventLoop::stop() {}

#endif
