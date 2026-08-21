#pragma once

#include "core/Buffer.hpp"
#include "core/EventLoop.hpp"
#include "core/Socket.hpp"
#include "core/TimerManager.hpp"
#include "http/HttpParser.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "platform/FileSender.hpp"
#include "server/Router.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace mininginx::core {

enum class ConnectionState {
    CONNECTED,
    READING,
    PARSING,
    ROUTING,
    WRITING,
    KEEP_ALIVE,
    CLOSING,
    CLOSED
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using CloseCallback = std::function<void(int fd)>;

    Connection(
        Socket socket,
        std::string client_ip,
        uint16_t client_port,
        EventLoop* loop,
        TimerManager* timer_mgr,
        server::Router* router,
        Milliseconds keep_alive_timeout = Milliseconds(60000),
        Milliseconds request_timeout = Milliseconds(10000),
        http::ParserLimits limits = http::ParserLimits{}
    );
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) noexcept = delete;
    Connection& operator=(Connection&&) noexcept = delete;

    void start();
    void handle_events(int fd, uint32_t events);

    void set_close_callback(CloseCallback cb) { close_cb_ = std::move(cb); }
    [[nodiscard]] int get_fd() const noexcept { return socket_.get_fd(); }
    [[nodiscard]] ConnectionState state() const noexcept { return state_; }
    [[nodiscard]] const std::string& client_ip() const noexcept { return client_ip_; }
    [[nodiscard]] uint16_t client_port() const noexcept { return client_port_; }
    [[nodiscard]] uint64_t request_count() const noexcept { return request_count_; }

    void close();

private:
    void handle_read();
    void handle_write();
    void process_request();
    void reset_timer();
    void on_timeout();
    void reset_for_next_request();

    Socket socket_;
    std::string client_ip_;
    uint16_t client_port_{0};
    EventLoop* loop_{nullptr};
    TimerManager* timer_mgr_{nullptr};
    server::Router* router_{nullptr};
    Milliseconds keep_alive_timeout_;
    Milliseconds request_timeout_;

    Buffer read_buf_;
    Buffer write_buf_;
    http::HttpParser parser_;
    ConnectionState state_{ConnectionState::CONNECTED};
    TimerId timer_id_{0};
    uint64_t request_count_{0};
    bool keep_alive_{true};

    // Observability
    std::chrono::steady_clock::time_point request_start_time_;
    CloseCallback close_cb_;
};

}  // namespace mininginx::core
