#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

namespace mininginx::metrics {

class Metrics {
public:
    static Metrics& instance();

    void record_connection_opened();
    void record_connection_closed();
    void record_request();
    void record_response(int status_code);
    void record_bytes_read(size_t bytes);
    void record_bytes_written(size_t bytes);

    [[nodiscard]] uint64_t total_connections() const noexcept { return total_connections_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t active_connections() const noexcept { return active_connections_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t total_requests() const noexcept { return total_requests_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t responses_2xx() const noexcept { return responses_2xx_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t responses_3xx() const noexcept { return responses_3xx_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t responses_4xx() const noexcept { return responses_4xx_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t responses_5xx() const noexcept { return responses_5xx_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t bytes_read() const noexcept { return bytes_read_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t bytes_written() const noexcept { return bytes_written_.load(std::memory_order_relaxed); }
    [[nodiscard]] uint64_t uptime_seconds() const noexcept;

    [[nodiscard]] std::string to_json() const;

    void reset();

private:
    Metrics();
    ~Metrics() = default;

    std::atomic<uint64_t> total_connections_{0};
    std::atomic<uint64_t> active_connections_{0};
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> responses_2xx_{0};
    std::atomic<uint64_t> responses_3xx_{0};
    std::atomic<uint64_t> responses_4xx_{0};
    std::atomic<uint64_t> responses_5xx_{0};
    std::atomic<uint64_t> bytes_read_{0};
    std::atomic<uint64_t> bytes_written_{0};
    std::chrono::steady_clock::time_point start_time_;
};

}  // namespace mininginx::metrics
