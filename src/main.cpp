#include "config/Config.hpp"
#include "logging/Logger.hpp"
#include "server/HttpServer.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

static std::unique_ptr<mininginx::server::HttpServer> g_server;

extern "C" void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        if (g_server) {
            g_server->stop();
        }
    }
}

static void print_usage(const char* prog) {
    std::cout << "MiniNginx - High-Performance Event-Driven HTTP/1.1 Web Server\n\n"
              << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  -c, --config <file>     Path to configuration file (default: config/server.conf)\n"
              << "  -p, --port <port>       Port to listen on (overrides config)\n"
              << "  -w, --workers <count>   Number of worker threads (overrides config)\n"
              << "  -r, --root <path>       Document root directory for static files (overrides config)\n"
              << "  -v, --version           Display version information\n"
              << "  -h, --help              Show this help message\n";
}

int main(int argc, char* argv[]) {
    // Ignore SIGPIPE to avoid crashing when client abruptly resets connection
    std::signal(SIGPIPE, SIG_IGN);

    std::string config_path = "config/server.conf";
    int override_port = -1;
    int override_workers = -1;
    std::string override_root;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) config_path = argv[++i];
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) override_port = std::stoi(argv[++i]);
        } else if (arg == "-w" || arg == "--workers") {
            if (i + 1 < argc) override_workers = std::stoi(argv[++i]);
        } else if (arg == "-r" || arg == "--root") {
            if (i + 1 < argc) override_root = argv[++i];
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "MiniNginx version 1.0.0 (C++20, Event-Driven)\n";
            return 0;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    // Load configuration
    mininginx::config::ServerConfig config = mininginx::config::ServerConfig::load_from_file(config_path);
    if (override_port > 0) config.port = static_cast<uint16_t>(override_port);
    if (override_workers > 0) config.workers = static_cast<uint32_t>(override_workers);
    if (!override_root.empty()) config.root = override_root;

    // Initialize logging
    mininginx::logging::LogLevel log_lvl = mininginx::logging::LogLevel::INFO;
    if (config.log_level == "DEBUG") log_lvl = mininginx::logging::LogLevel::DEBUG;
    else if (config.log_level == "WARN") log_lvl = mininginx::logging::LogLevel::WARN;
    else if (config.log_level == "ERROR") log_lvl = mininginx::logging::LogLevel::ERROR;

    mininginx::logging::Logger::instance().init(log_lvl, config.access_log, config.error_log);

    // Register signal handlers for clean graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    g_server = std::make_unique<mininginx::server::HttpServer>(config);

    if (!g_server->init()) {
        std::cerr << "Failed to initialize MiniNginx server.\n";
        return 1;
    }

    g_server->start();
    return 0;
}
