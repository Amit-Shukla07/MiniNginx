#pragma once

#include "../include/server/EventLoop.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <unordered_map>
#include <vector>

class WindowsEventLoop : public EventLoop {
public:
    WindowsEventLoop();
    ~WindowsEventLoop() override;

    void add(int fd, int events, EventCallback cb) override;
    void modify(int fd, int events) override;
    void remove(int fd) override;
    void poll(int timeout_ms) override;
    void stop() override;
    bool is_running() const override { return running_; }

private:
    struct Handler {
        int events{0};
        EventCallback cb;
    };

    bool running_{false};
    std::unordered_map<int, Handler> handlers_;
#ifdef _WIN32
    std::vector<WSAPOLLFD> poll_fds_;
#endif
};
