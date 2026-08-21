#include "server/Worker.hpp"

#include <chrono>

namespace mininginx::server {

Worker::Worker(uint32_t worker_id)
    : worker_id_(worker_id),
      loop_(core::EventLoop::create()),
      timer_mgr_(std::make_unique<core::TimerManager>()) {}

Worker::~Worker() {
    stop();
    join();
}

void Worker::start() {
    running_.store(true, std::memory_order_release);
    thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
            int next_timeout = timer_mgr_->get_next_timeout_ms();
            loop_->poll_and_dispatch(next_timeout);
            timer_mgr_->handle_expired_timers();
        }
    });
}

void Worker::run_loop_step(int timeout_ms) {
    int next_timeout = timer_mgr_->get_next_timeout_ms();
    int timeout = (timeout_ms < next_timeout) ? timeout_ms : next_timeout;
    loop_->poll_and_dispatch(timeout);
    timer_mgr_->handle_expired_timers();
}

void Worker::stop() {
    running_.store(false, std::memory_order_release);
    if (loop_) {
        loop_->stop();
    }

    std::lock_guard<std::mutex> lock(conn_mutex_);
    for (auto& [fd, conn] : connections_) {
        if (conn) {
            conn->close();
        }
    }
    connections_.clear();
}

void Worker::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Worker::add_connection(
    core::Socket socket,
    std::string client_ip,
    uint16_t client_port,
    Router* router,
    const config::ServerConfig& config
) {
    int fd = socket.get_fd();
    if (fd < 0) return;

    http::ParserLimits limits{
        .max_request_line_len = 8192,
        .max_header_size = config.max_header_size,
        .max_headers_count = 128,
        .max_body_size = config.max_body_size
    };

    auto conn = std::make_shared<core::Connection>(
        std::move(socket),
        std::move(client_ip),
        client_port,
        loop_.get(),
        timer_mgr_.get(),
        router,
        core::Milliseconds(config.keep_alive_timeout_sec * 1000),
        core::Milliseconds(config.request_timeout_sec * 1000),
        limits
    );

    conn->set_close_callback([this](int closed_fd) {
        remove_connection(closed_fd);
    });

    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        connections_[fd] = conn;
    }

    conn->start();
}

void Worker::remove_connection(int fd) {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    connections_.erase(fd);
}

size_t Worker::connection_count() const {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    return connections_.size();
}

}  // namespace mininginx::server
