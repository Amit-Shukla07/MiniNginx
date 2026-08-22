#include "WindowsEventLoop.hpp"

#include <iostream>

#ifdef _WIN32

WindowsEventLoop::WindowsEventLoop() = default;

WindowsEventLoop::~WindowsEventLoop() {
    stop();
}

void WindowsEventLoop::add(int fd, int events, EventCallback cb) {
    if (fd < 0) return;
    handlers_[fd] = Handler{events, std::move(cb)};
}

void WindowsEventLoop::modify(int fd, int events) {
    auto it = handlers_.find(fd);
    if (it != handlers_.end()) {
        it->second.events = events;
    }
}

void WindowsEventLoop::remove(int fd) {
    handlers_.erase(fd);
}

void WindowsEventLoop::poll(int timeout_ms) {
    if (handlers_.empty()) return;

    poll_fds_.clear();
    poll_fds_.reserve(handlers_.size());

    for (const auto& [fd, handler] : handlers_) {
        WSAPOLLFD pfd{};
        pfd.fd = static_cast<SOCKET>(fd);
        pfd.events = 0;
        if (handler.events & EVENT_READ) pfd.events |= POLLRDNORM;
        if (handler.events & EVENT_WRITE) pfd.events |= POLLWRNORM;
        poll_fds_.push_back(pfd);
    }

    running_ = true;
    int ret = ::WSAPoll(poll_fds_.data(), static_cast<ULONG>(poll_fds_.size()), timeout_ms);
    if (ret <= 0) return;

    for (const auto& pfd : poll_fds_) {
        if (pfd.revents == 0) continue;

        int fd = static_cast<int>(pfd.fd);
        int fired = 0;
        if (pfd.revents & (POLLRDNORM | POLLRDBAND)) fired |= EVENT_READ;
        if (pfd.revents & POLLWRNORM) fired |= EVENT_WRITE;
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) fired |= EVENT_ERROR;

        auto it = handlers_.find(fd);
        if (it != handlers_.end() && it->second.cb) {
            it->second.cb(fd, fired);
        }
    }
}

void WindowsEventLoop::stop() {
    running_ = false;
}

#else

WindowsEventLoop::WindowsEventLoop() = default;
WindowsEventLoop::~WindowsEventLoop() = default;
void WindowsEventLoop::add(int, int, EventCallback) {}
void WindowsEventLoop::modify(int, int) {}
void WindowsEventLoop::remove(int) {}
void WindowsEventLoop::poll(int) {}
void WindowsEventLoop::stop() {}

#endif
