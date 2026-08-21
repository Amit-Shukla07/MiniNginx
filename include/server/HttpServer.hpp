#pragma once

#include "config/Config.hpp"
#include "core/EventLoop.hpp"
#include "core/Socket.hpp"
#include "server/Router.hpp"
#include "server/StaticFileHandler.hpp"
#include "server/Worker.hpp"

#include <atomic>
#include <memory>
#include <vector>

namespace mininginx::server {

class HttpServer {
public:
    explicit HttpServer(config::ServerConfig config);
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) noexcept = delete;
    HttpServer& operator=(HttpServer&&) noexcept = delete;

    bool init();
    void start();
    void stop();

    [[nodiscard]] Router& router() noexcept { return router_; }
    [[nodiscard]] const config::ServerConfig& config() const noexcept { return config_; }
    [[nodiscard]] bool is_running() const noexcept { return running_.load(std::memory_order_relaxed); }

private:
    void setup_routes();
    void handle_new_connection();
    void dispatch_connection(core::Socket client_sock, std::string client_ip, uint16_t client_port);

    config::ServerConfig config_;
    core::Socket listener_;
    std::unique_ptr<core::EventLoop> main_loop_;
    Router router_;
    std::unique_ptr<StaticFileHandler> static_handler_;
    std::vector<std::unique_ptr<Worker>> workers_;
    size_t next_worker_idx_{0};
    std::atomic<bool> running_{false};
};

}  // namespace mininginx::server
