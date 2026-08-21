#pragma once

#include "Buffer.hpp"
#include "EventLoop.hpp"
#include "HttpParser.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Router.hpp"
#include "Socket.hpp"

#include <ctime>
#include <functional>
#include <memory>
#include <string>

class Connection {
public:
    using CloseCallback = std::function<void(int fd)>;

    Connection(
        Socket socket,
        std::string client_ip,
        EventLoop* loop,
        Router* router,
        CloseCallback on_close
    );
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    void handle_events(int fd, int events);
    void close();

    [[nodiscard]] int fd() const noexcept { return socket_.fd(); }
    [[nodiscard]] time_t last_activity() const noexcept { return last_activity_; }
    [[nodiscard]] bool is_closed() const noexcept { return closed_; }

private:
    void handle_read();
    void handle_write();
    void process_request();
    void reset_for_next_request();

    Socket socket_;
    std::string client_ip_;
    EventLoop* loop_{nullptr};
    Router* router_{nullptr};
    CloseCallback on_close_;

    Buffer readBuffer_;
    Buffer writeBuffer_;
    HttpParser parser_;
    bool keepAlive_{true};
    time_t last_activity_{0};
    bool closed_{false};
};
