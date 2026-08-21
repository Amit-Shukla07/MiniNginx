#include "core/Socket.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

namespace mininginx::core {

Socket::Socket(int fd) noexcept : fd_(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Socket Socket::create_tcp() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    return Socket(fd);
}

bool Socket::set_non_blocking(bool non_blocking) const {
    if (fd_ < 0) return false;
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) return false;
    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return ::fcntl(fd_, F_SETFL, flags) >= 0;
}

bool Socket::set_reuse_addr(bool reuse) const {
    if (fd_ < 0) return false;
    int optval = reuse ? 1 : 0;
    return ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == 0;
}

bool Socket::set_reuse_port(bool reuse) const {
    if (fd_ < 0) return false;
#ifdef SO_REUSEPORT
    int optval = reuse ? 1 : 0;
    return ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == 0;
#else
    (void)reuse;
    return true;
#endif
}

bool Socket::set_tcp_nodelay(bool nodelay) const {
    if (fd_ < 0) return false;
    int optval = nodelay ? 1 : 0;
    return ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == 0;
}

bool Socket::set_send_buffer_size(int size) const {
    if (fd_ < 0) return false;
    return ::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == 0;
}

bool Socket::set_recv_buffer_size(int size) const {
    if (fd_ < 0) return false;
    return ::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == 0;
}

bool Socket::bind(const std::string& host, uint16_t port) const {
    if (fd_ < 0) return false;

    struct sockaddr_in addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (host == "0.0.0.0" || host.empty()) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            return false;
        }
    }

    return ::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0;
}

bool Socket::listen(int backlog) const {
    if (fd_ < 0) return false;
    return ::listen(fd_, backlog) == 0;
}

std::pair<Socket, std::string> Socket::accept(uint16_t* client_port) const {
    if (fd_ < 0) return {Socket(-1), ""};

    struct sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = -1;

    while (true) {
        client_fd = ::accept(fd_, reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);
        if (client_fd >= 0) break;
        if (errno == EINTR) continue;  // Retry on signal interrupt
        return {Socket(-1), ""};
    }

    char ip_buf[INET_ADDRSTRLEN];
    const char* ip_str = ::inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
    std::string ip = ip_str ? ip_str : "";

    if (client_port) {
        *client_port = ntohs(client_addr.sin_port);
    }

    return {Socket(client_fd), ip};
}

bool Socket::connect(const std::string& host, uint16_t port) const {
    if (fd_ < 0) return false;

    struct sockaddr_in addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        return false;
    }

    while (true) {
        int res = ::connect(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
        if (res == 0) return true;
        if (errno == EINTR) continue;
        return false;
    }
}

void Socket::shutdown(int how) const {
    if (fd_ >= 0) {
        ::shutdown(fd_, how);
    }
}

void Socket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

int Socket::release() noexcept {
    int temp = fd_;
    fd_ = -1;
    return temp;
}

void Socket::reset(int fd) noexcept {
    close();
    fd_ = fd;
}

std::string Socket::get_peer_ip() const {
    if (fd_ < 0) return "";
    struct sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (::getpeername(fd_, reinterpret_cast<struct sockaddr*>(&addr), &len) != 0) {
        return "";
    }
    char buf[INET_ADDRSTRLEN];
    const char* p = ::inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
    return p ? std::string(p) : "";
}

uint16_t Socket::get_peer_port() const {
    if (fd_ < 0) return 0;
    struct sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (::getpeername(fd_, reinterpret_cast<struct sockaddr*>(&addr), &len) != 0) {
        return 0;
    }
    return ntohs(addr.sin_port);
}

}  // namespace mininginx::core
