#include "../include/server/HttpServer.hpp"

#ifdef _WIN32
#include <winsock2.h>
#endif

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
#ifdef _WIN32
    WSADATA wsa_data;
    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        return 1;
    }
#else
    std::signal(SIGPIPE, SIG_IGN);
#endif

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::string config_path = "config/server.conf";
    if (argc > 1) {
        config_path = argv[1];
    }

    ServerConfig config = ServerConfig::load(config_path);

    g_server = std::make_unique<HttpServer>(config);
    if (!g_server->init()) {
#ifdef _WIN32
        ::WSACleanup();
#endif
        return 1;
    }

    g_server->start();

#ifdef _WIN32
    ::WSACleanup();
#endif
    return 0;
}
