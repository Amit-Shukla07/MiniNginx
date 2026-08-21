#pragma once

#include "../include/server/EventLoop.hpp"

#ifdef __linux__
#include <sys/epoll.h>
#endif

#include <unordered_map>
#include <vector>

class EpollEventLoop : public EventLoop {
public:
    EpollEventLoop();
    ~EpollEventLoop() override;

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

    int epoll_fd_{-1};
    bool running_{false};
#ifdef __linux__
    std::vector<struct epoll_event> eventList_;
#endif
    std::unordered_map<int, Handler> handlers_;
};
