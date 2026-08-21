#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace mininginx::core {

class Socket {
public:
    explicit Socket(int fd = -1) noexcept;
    ~Socket();

    // Move-only semantics (RAII ownership of file descriptor)
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // Factory methods
    [[nodiscard]] static Socket create_tcp();

    // Socket configuration
    [[nodiscard]] bool set_non_blocking(bool non_blocking = true) const;
    [[nodiscard]] bool set_reuse_addr(bool reuse = true) const;
    [[nodiscard]] bool set_reuse_port(bool reuse = true) const;
    [[nodiscard]] bool set_tcp_nodelay(bool nodelay = true) const;
    [[nodiscard]] bool set_send_buffer_size(int size) const;
    [[nodiscard]] bool set_recv_buffer_size(int size) const;

    // Server operations
    [[nodiscard]] bool bind(const std::string& host, uint16_t port) const;
    [[nodiscard]] bool listen(int backlog = 4096) const;
    [[nodiscard]] std::pair<Socket, std::string> accept(uint16_t* client_port = nullptr) const;

    // Client operations
    [[nodiscard]] bool connect(const std::string& host, uint16_t port) const;

    // Teardown operations
    void shutdown(int how = 2 /* SHUT_RDWR */) const;
    void close();

    // Descriptor access
    [[nodiscard]] int get_fd() const noexcept { return fd_; }
    [[nodiscard]] int release() noexcept;
    void reset(int fd = -1) noexcept;
    [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0; }
    explicit operator bool() const noexcept { return is_valid(); }

    // Peer information
    [[nodiscard]] std::string get_peer_ip() const;
    [[nodiscard]] uint16_t get_peer_port() const;

private:
    int fd_{-1};
};

}  // namespace mininginx::core
