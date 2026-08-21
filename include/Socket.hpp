#pragma once

#include <cstdint>
#include <string>
#include <utility>

class Socket {
public:
    explicit Socket(int fd = -1) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    static Socket create_tcp();

    bool set_non_blocking(bool non_blocking = true) const;
    bool set_reuse_addr(bool reuse = true) const;
    bool set_tcp_nodelay(bool nodelay = true) const;

    bool bind(const std::string& host, uint16_t port) const;
    bool listen(int backlog = 1024) const;
    std::pair<Socket, std::string> accept() const;

    void close();
    [[nodiscard]] int fd() const noexcept { return fd_; }
    [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0; }

private:
    int fd_{-1};
};
