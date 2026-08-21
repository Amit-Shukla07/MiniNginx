#include "metrics/Metrics.hpp"

#include <sstream>

namespace mininginx::metrics {

Metrics::Metrics() : start_time_(std::chrono::steady_clock::now()) {}

Metrics& Metrics::instance() {
    static Metrics metrics;
    return metrics;
}

void Metrics::record_connection_opened() {
    total_connections_.fetch_add(1, std::memory_order_relaxed);
    active_connections_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::record_connection_closed() {
    if (active_connections_.load(std::memory_order_relaxed) > 0) {
        active_connections_.fetch_sub(1, std::memory_order_relaxed);
    }
}

void Metrics::record_request() {
    total_requests_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::record_response(int status_code) {
    if (status_code >= 200 && status_code < 300) {
        responses_2xx_.fetch_add(1, std::memory_order_relaxed);
    } else if (status_code >= 300 && status_code < 400) {
        responses_3xx_.fetch_add(1, std::memory_order_relaxed);
    } else if (status_code >= 400 && status_code < 500) {
        responses_4xx_.fetch_add(1, std::memory_order_relaxed);
    } else if (status_code >= 500 && status_code < 600) {
        responses_5xx_.fetch_add(1, std::memory_order_relaxed);
    }
}

void Metrics::record_bytes_read(size_t bytes) {
    bytes_read_.fetch_add(bytes, std::memory_order_relaxed);
}

void Metrics::record_bytes_written(size_t bytes) {
    bytes_written_.fetch_add(bytes, std::memory_order_relaxed);
}

uint64_t Metrics::uptime_seconds() const noexcept {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count()
    );
}

std::string Metrics::to_json() const {
    std::ostringstream oss;
    oss << "{\n"
        << "  \"server\": \"MiniNginx/1.0.0\",\n"
        << "  \"uptime_seconds\": " << uptime_seconds() << ",\n"
        << "  \"active_connections\": " << active_connections() << ",\n"
        << "  \"total_connections\": " << total_connections() << ",\n"
        << "  \"total_requests\": " << total_requests() << ",\n"
        << "  \"responses_2xx\": " << responses_2xx() << ",\n"
        << "  \"responses_3xx\": " << responses_3xx() << ",\n"
        << "  \"responses_4xx\": " << responses_4xx() << ",\n"
        << "  \"responses_5xx\": " << responses_5xx() << ",\n"
        << "  \"bytes_read\": " << bytes_read() << ",\n"
        << "  \"bytes_written\": " << bytes_written() << "\n"
        << "}\n";
    return oss.str();
}

void Metrics::reset() {
    total_connections_.store(0, std::memory_order_relaxed);
    active_connections_.store(0, std::memory_order_relaxed);
    total_requests_.store(0, std::memory_order_relaxed);
    responses_2xx_.store(0, std::memory_order_relaxed);
    responses_3xx_.store(0, std::memory_order_relaxed);
    responses_4xx_.store(0, std::memory_order_relaxed);
    responses_5xx_.store(0, std::memory_order_relaxed);
    bytes_read_.store(0, std::memory_order_relaxed);
    bytes_written_.store(0, std::memory_order_relaxed);
    start_time_ = std::chrono::steady_clock::now();
}

}  // namespace mininginx::metrics
