#pragma once

#include "../include/server/EventLoop.hpp"

#include <sys/event.h>
#include <unordered_map>
#include <vector>

class KqueueEventLoop : public EventLoop {
public:
    KqueueEventLoop();
    ~KqueueEventLoop() override;

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

    int kq_fd_{-1};
    bool running_{false};
    std::vector<struct kevent> eventList_;
    std::unordered_map<int, Handler> handlers_;
};
