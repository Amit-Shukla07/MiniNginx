#include "../../include/server/HttpServer.hpp"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

ServerConfig ServerConfig::load(const std::string& path) {
    ServerConfig cfg;
    std::ifstream f(path);
    if (!f.is_open()) return cfg;

    std::string key, val;
    while (f >> key >> val) {
        if (key == "host") cfg.host = val;
        else if (key == "port") cfg.port = static_cast<uint16_t>(std::stoi(val));
        else if (key == "root") cfg.root = val;
        else if (key == "keep_alive_timeout") cfg.keep_alive_timeout = std::stoi(val);
    }
    return cfg;
}

HttpServer::HttpServer(ServerConfig config)
    : config_(std::move(config)),
      loop_(EventLoop::create()),
      router_(config_.root),
      last_timeout_check_(std::time(nullptr)) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::init() {
    listener_ = Socket::create_tcp();
    if (!listener_.is_valid()) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    listener_.set_reuse_addr(true);
    if (!listener_.set_non_blocking(true)) {
        std::cerr << "Failed to set non-blocking on listener\n";
        return false;
    }

    if (!listener_.bind(config_.host, config_.port)) {
        std::cerr << "Failed to bind to " << config_.host << ":" << config_.port << "\n";
        return false;
    }

    if (!listener_.listen(1024)) {
        std::cerr << "Failed to listen\n";
        return false;
    }

    std::cout << "[INFO] MiniNginx listening on " << config_.host << ":" << config_.port
              << " (root: " << config_.root << ")\n";
    return true;
}

void HttpServer::start() {
    if (!listener_.is_valid()) return;

    running_ = true;

    loop_->add(
        listener_.fd(),
        EVENT_READ,
        [this](int, int) {
            handle_new_connection();
        }
    );

    while (running_) {
        loop_->poll(1000);
        check_timeouts();
    }
}

void HttpServer::handle_new_connection() {
    while (true) {
        auto [client_sock, client_ip] = listener_.accept();
        if (!client_sock.is_valid()) break;

        int fd = client_sock.fd();
        auto conn = std::make_unique<Connection>(
            std::move(client_sock),
            std::move(client_ip),
            loop_.get(),
            &router_,
            [this](int closed_fd) {
                connections_.erase(closed_fd);
            }
        );

        loop_->add(
            fd,
            EVENT_READ,
            [conn_ptr = conn.get()](int f, int ev) {
                conn_ptr->handle_events(f, ev);
            }
        );

        connections_[fd] = std::move(conn);
    }
}

void HttpServer::check_timeouts() {
    time_t now = std::time(nullptr);
    if (now - last_timeout_check_ < 1) return;
    last_timeout_check_ = now;

    std::vector<int> expired;
    for (const auto& [fd, conn] : connections_) {
        if (now - conn->last_activity() > config_.keep_alive_timeout) {
            expired.push_back(fd);
        }
    }

    for (int fd : expired) {
        auto it = connections_.find(fd);
        if (it != connections_.end()) {
            it->second->close();
        }
    }
}

void HttpServer::stop() {
    if (!running_) return;
    running_ = false;

    if (loop_ && listener_.is_valid()) {
        loop_->remove(listener_.fd());
    }
    listener_.close();

    for (auto& [fd, conn] : connections_) {
        conn->close();
    }
    connections_.clear();

    if (loop_) {
        loop_->stop();
    }

    std::cout << "[INFO] MiniNginx stopped cleanly\n";
}
