#include "logging/Logger.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace mininginx::logging {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (access_file_.is_open()) access_file_.close();
    if (error_file_.is_open()) error_file_.close();
}

void Logger::init(LogLevel level, const std::string& access_log_path, const std::string& error_log_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;

    if (!access_log_path.empty()) {
        std::filesystem::path p(access_log_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        access_file_.open(access_log_path, std::ios::app);
    }

    if (!error_log_path.empty()) {
        std::filesystem::path p(error_log_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        error_file_.open(error_log_path, std::ios::app);
    }
}

std::string Logger::get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    std::ostringstream oss;
    oss << buf << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Logger::log(LogLevel level, std::string_view message) {
    if (level < level_) return;

    std::string timestamp = get_iso_timestamp();
    const char* level_str = "INFO";
    switch (level) {
        case LogLevel::DEBUG: level_str = "DEBUG"; break;
        case LogLevel::INFO:  level_str = "INFO";  break;
        case LogLevel::WARN:  level_str = "WARN";  break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::ostream& out = (level >= LogLevel::WARN) ? std::cerr : std::cout;
    out << "[" << timestamp << "] [" << level_str << "] " << message << std::endl;

    if (error_file_.is_open() && level >= LogLevel::WARN) {
        error_file_ << "[" << timestamp << "] [" << level_str << "] " << message << std::endl;
    }
}

void Logger::log_access(
    const std::string& client_ip,
    std::string_view method,
    std::string_view uri,
    std::string_view version,
    int status_code,
    size_t bytes_sent,
    double latency_ms,
    std::string_view user_agent
) {
    std::string timestamp = get_iso_timestamp();
    std::ostringstream oss;
    oss << (client_ip.empty() ? "127.0.0.1" : client_ip) << " - - [" << timestamp << "] \""
        << method << " " << uri << " " << version << "\" "
        << status_code << " " << bytes_sent << " \""
        << user_agent << "\" " << std::fixed << std::setprecision(2) << latency_ms << "ms";

    std::string line = oss.str();

    std::lock_guard<std::mutex> lock(mutex_);
    if (access_file_.is_open()) {
        access_file_ << line << std::endl;
    } else {
        std::cout << "[ACCESS] " << line << std::endl;
    }
}

}  // namespace mininginx::logging
