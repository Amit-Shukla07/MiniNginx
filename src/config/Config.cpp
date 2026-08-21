#include "config/Config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace mininginx::config {

static std::string trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return std::string(s);
}

ServerConfig ServerConfig::load_from_file(const std::string& filepath) {
    ServerConfig cfg;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file " << filepath << ", using default configuration.\n";
        return cfg;
    }

    std::string line;
    int line_number = 0;

    while (std::getline(file, line)) {
        line_number++;
        // Remove comments
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        line = trim(line);
        if (line.empty() || line == "server {" || line == "}") {
            continue;
        }

        // Remove trailing semicolon if present
        if (!line.empty() && line.back() == ';') {
            line.pop_back();
            line = trim(line);
        }

        std::istringstream iss(line);
        std::string key, value;
        if (!(iss >> key >> value)) {
            continue;
        }

        try {
            if (key == "host") {
                cfg.host = value;
            } else if (key == "port") {
                cfg.port = static_cast<uint16_t>(std::stoul(value));
            } else if (key == "root") {
                cfg.root = value;
            } else if (key == "workers") {
                cfg.workers = static_cast<uint32_t>(std::stoul(value));
            } else if (key == "keep_alive_timeout") {
                cfg.keep_alive_timeout_sec = static_cast<uint32_t>(std::stoul(value));
            } else if (key == "request_timeout") {
                cfg.request_timeout_sec = static_cast<uint32_t>(std::stoul(value));
            } else if (key == "max_connections") {
                cfg.max_connections = std::stoull(value);
            } else if (key == "max_header_size") {
                cfg.max_header_size = std::stoull(value);
            } else if (key == "max_body_size") {
                cfg.max_body_size = std::stoull(value);
            } else if (key == "access_log") {
                cfg.access_log = value;
            } else if (key == "error_log") {
                cfg.error_log = value;
            } else if (key == "log_level") {
                cfg.log_level = value;
            }
        } catch (const std::exception& e) {
            std::cerr << "Config error at line " << line_number << " for key '" << key << "': " << e.what() << "\n";
        }
    }

    return cfg;
}

bool ServerConfig::validate(std::string& error_msg) const {
    if (port == 0) {
        error_msg = "Invalid port number: 0";
        return false;
    }
    if (workers == 0 || workers > 64) {
        error_msg = "Worker count must be between 1 and 64";
        return false;
    }
    if (max_connections == 0) {
        error_msg = "max_connections must be greater than 0";
        return false;
    }
    if (max_header_size < 1024) {
        error_msg = "max_header_size must be at least 1024 bytes";
        return false;
    }
    if (max_body_size == 0) {
        error_msg = "max_body_size must be greater than 0";
        return false;
    }
    return true;
}

}  // namespace mininginx::config
