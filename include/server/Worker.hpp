#pragma once

#include "config/Config.hpp"
#include "core/Connection.hpp"
#include "core/EventLoop.hpp"
#include "core/Socket.hpp"
#include "core/TimerManager.hpp"
#include "server/Router.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mininginx::server {

class Worker {
public:
    explicit Worker(uint32_t worker_id);
    ~Worker();

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    Worker(Worker&&) noexcept = delete;
    Worker& operator=(Worker&&) noexcept = delete;

    void start();
    void stop();
    void join();

    void add_connection(
        core::Socket socket,
        std::string client_ip,
        uint16_t client_port,
        Router* router,
        const config::ServerConfig& config
    );

    [[nodiscard]] uint32_t id() const noexcept { return worker_id_; }
    [[nodiscard]] size_t connection_count() const;
    [[nodiscard]] core::EventLoop* loop() noexcept { return loop_.get(); }
    [[nodiscard]] core::TimerManager* timer_mgr() noexcept { return timer_mgr_.get(); }

    void run_loop_step(int timeout_ms = 100);

private:
    uint32_t worker_id_{0};
    std::unique_ptr<core::EventLoop> loop_;
    std::unique_ptr<core::TimerManager> timer_mgr_;
    std::unordered_map<int, std::shared_ptr<core::Connection>> connections_;
    mutable std::mutex conn_mutex_;
    std::thread thread_;
    std::atomic<bool> running_{false};

    void remove_connection(int fd);
};

}  // namespace mininginx::server
