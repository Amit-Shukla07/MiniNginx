#pragma once

#include "Connection.hpp"
#include "EventLoop.hpp"
#include "Router.hpp"
#include "Socket.hpp"

#include <memory>
#include <string>
#include <unordered_map>

struct ServerConfig {
    std::string host{"0.0.0.0"};
    uint16_t port{8080};
    std::string root{"./public"};
    int keep_alive_timeout{60};  // seconds

    static ServerConfig load(const std::string& path);
};

class HttpServer {
public:
    explicit HttpServer(ServerConfig config);
    ~HttpServer();

    bool init();
    void start();
    void stop();

private:
    void handle_new_connection();
    void check_timeouts();

    ServerConfig config_;
    Socket listener_;
    std::unique_ptr<EventLoop> loop_;
    Router router_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    bool running_{false};
    time_t last_timeout_check_{0};
};
