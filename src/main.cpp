#include "../include/server/HttpServer.hpp"

#include <csignal>
#include <iostream>
#include <memory>

static std::unique_ptr<HttpServer> g_server;

extern "C" void handle_signal(int) {
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::string config_path = "config/server.conf";
    if (argc > 1) {
        config_path = argv[1];
    }

    ServerConfig config = ServerConfig::load(config_path);

    g_server = std::make_unique<HttpServer>(config);
    if (!g_server->init()) {
        return 1;
    }

    g_server->start();
    return 0;
}
