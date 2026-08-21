#pragma once

#include <cstdint>
#include <string>

namespace mininginx::config {

struct ServerConfig {
    std::string host{"0.0.0.0"};
    uint16_t port{8080};
    std::string root{"./public"};
    uint32_t workers{1};
    uint32_t keep_alive_timeout_sec{60};
    uint32_t request_timeout_sec{10};
    size_t max_connections{10000};
    size_t max_header_size{65536};
    size_t max_body_size{10 * 1024 * 1024};
    std::string access_log{"./logs/access.log"};
    std::string error_log{"./logs/error.log"};
    std::string log_level{"INFO"};

    [[nodiscard]] static ServerConfig load_from_file(const std::string& filepath);
    [[nodiscard]] bool validate(std::string& error_msg) const;
};

}  // namespace mininginx::config
