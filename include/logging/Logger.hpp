#pragma once

#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace mininginx::logging {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& instance();

    void init(LogLevel level, const std::string& access_log_path = "", const std::string& error_log_path = "");

    void log(LogLevel level, std::string_view message);
    void log_access(
        const std::string& client_ip,
        std::string_view method,
        std::string_view uri,
        std::string_view version,
        int status_code,
        size_t bytes_sent,
        double latency_ms,
        std::string_view user_agent = "-"
    );

    void debug(std::string_view message) { log(LogLevel::DEBUG, message); }
    void info(std::string_view message) { log(LogLevel::INFO, message); }
    void warn(std::string_view message) { log(LogLevel::WARN, message); }
    void error(std::string_view message) { log(LogLevel::ERROR, message); }

    [[nodiscard]] LogLevel level() const noexcept { return level_; }
    void set_level(LogLevel l) noexcept { level_ = l; }

private:
    Logger() = default;
    ~Logger();

    LogLevel level_{LogLevel::INFO};
    std::ofstream access_file_;
    std::ofstream error_file_;
    std::mutex mutex_;

    [[nodiscard]] static std::string get_iso_timestamp();
};

#define LOG_DEBUG(msg) mininginx::logging::Logger::instance().debug(msg)
#define LOG_INFO(msg)  mininginx::logging::Logger::instance().info(msg)
#define LOG_WARN(msg)  mininginx::logging::Logger::instance().warn(msg)
#define LOG_ERROR(msg) mininginx::logging::Logger::instance().error(msg)

}  // namespace mininginx::logging
