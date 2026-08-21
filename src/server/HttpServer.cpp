#include "server/HttpServer.hpp"
#include "logging/Logger.hpp"
#include "metrics/Metrics.hpp"

#include <iostream>

namespace mininginx::server {

HttpServer::HttpServer(config::ServerConfig config)
    : config_(std::move(config)),
      main_loop_(core::EventLoop::create()) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::init() {
    std::string error_msg;
    if (!config_.validate(error_msg)) {
        LOG_ERROR("Config validation failed: " + error_msg);
        return false;
    }

    listener_ = core::Socket::create_tcp();
    if (!listener_.is_valid()) {
        LOG_ERROR("Failed to create TCP listening socket");
        return false;
    }

    if (!listener_.set_reuse_addr(true)) {
        LOG_WARN("Failed to set SO_REUSEADDR on listener socket");
    }

    if (!listener_.set_reuse_port(true)) {
        LOG_WARN("Failed to set SO_REUSEPORT on listener socket");
    }

    if (!listener_.set_non_blocking(true)) {
        LOG_ERROR("Failed to set non-blocking on listener socket");
        return false;
    }

    if (!listener_.bind(config_.host, config_.port)) {
        LOG_ERROR("Failed to bind " + config_.host + ":" + std::to_string(config_.port) + ": Address already in use");
        return false;
    }

    if (!listener_.listen(4096)) {
        LOG_ERROR("Failed to listen on socket");
        return false;
    }

    setup_routes();

    // Create workers
    uint32_t num_workers = config_.workers;
    for (uint32_t i = 0; i < num_workers; ++i) {
        workers_.push_back(std::make_unique<Worker>(i + 1));
    }

    LOG_INFO("MiniNginx initialized on " + config_.host + ":" + std::to_string(config_.port) +
             " with " + std::to_string(num_workers) + " worker(s)");
    return true;
}

void HttpServer::setup_routes() {
    // Built-in /api/health endpoint
    router_.get("/api/health", [](const http::HttpRequest&) {
        return http::HttpResponse::json("{\"status\":\"ok\",\"service\":\"MiniNginx\"}");
    });

    // Built-in /api/stats endpoint
    router_.get("/api/stats", [](const http::HttpRequest&) {
        return http::HttpResponse::json(metrics::Metrics::instance().to_json());
    });

    // Static file handler fallback
    static_handler_ = std::make_unique<StaticFileHandler>(config_.root);
    router_.set_fallback_handler([this](const http::HttpRequest& req) {
        return static_handler_->handle(req);
    });
}

void HttpServer::start() {
    if (!listener_.is_valid()) {
        LOG_ERROR("Cannot start server: listener socket is not valid");
        return;
    }

    running_.store(true, std::memory_order_release);

    // Start workers
    for (auto& worker : workers_) {
        worker->start();
    }

    // Register listener on main event loop
    main_loop_->add_fd(
        listener_.get_fd(),
        static_cast<uint32_t>(core::EventType::Read),
        [this](int, uint32_t) {
            handle_new_connection();
        }
    );

    LOG_INFO("MiniNginx server running. Press Ctrl+C to terminate.");

    while (running_.load(std::memory_order_acquire)) {
        main_loop_->poll_and_dispatch(100);
    }
}

void HttpServer::handle_new_connection() {
    while (true) {
        uint16_t client_port = 0;
        auto [client_sock, client_ip] = listener_.accept(&client_port);
        if (!client_sock.is_valid()) {
            break;  // No more pending incoming connections (EAGAIN / EWOULDBLOCK)
        }

        if (metrics::Metrics::instance().active_connections() >= config_.max_connections) {
            LOG_WARN("Max concurrent connections reached, rejecting connection from " + client_ip);
            client_sock.close();
            continue;
        }

        dispatch_connection(std::move(client_sock), std::move(client_ip), client_port);
    }
}

void HttpServer::dispatch_connection(core::Socket client_sock, std::string client_ip, uint16_t client_port) {
    if (workers_.empty()) {
        client_sock.close();
        return;
    }

    // Distribute connections round-robin among workers
    size_t idx = next_worker_idx_++ % workers_.size();
    workers_[idx]->add_connection(
        std::move(client_sock),
        std::move(client_ip),
        client_port,
        &router_,
        config_
    );
}

void HttpServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    LOG_INFO("Initiating graceful shutdown...");

    if (main_loop_ && listener_.is_valid()) {
        main_loop_->remove_fd(listener_.get_fd());
    }
    listener_.close();

    for (auto& worker : workers_) {
        worker->stop();
    }

    for (auto& worker : workers_) {
        worker->join();
    }

    if (main_loop_) {
        main_loop_->stop();
    }

    LOG_INFO("MiniNginx shutdown complete. All resources cleanly released.");
}

}  // namespace mininginx::server
