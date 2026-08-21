#pragma once

#include <functional>
#include <memory>

enum EventType {
    EVENT_NONE = 0,
    EVENT_READ = 1 << 0,
    EVENT_WRITE = 1 << 1,
    EVENT_ERROR = 1 << 2
};

using EventCallback = std::function<void(int fd, int events)>;

class EventLoop {
public:
    virtual ~EventLoop() = default;

    virtual void add(int fd, int events, EventCallback cb) = 0;
    virtual void modify(int fd, int events) = 0;
    virtual void remove(int fd) = 0;
    virtual void poll(int timeout_ms) = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;

    static std::unique_ptr<EventLoop> create();
};
