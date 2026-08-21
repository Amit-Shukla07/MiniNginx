#include "../../include/server/Connection.hpp"

#include <unistd.h>
#include <cerrno>
#include <iostream>

Connection::Connection(
    Socket socket,
    std::string client_ip,
    EventLoop* loop,
    Router* router,
    CloseCallback on_close
)
    : socket_(std::move(socket)),
      client_ip_(std::move(client_ip)),
      loop_(loop),
      router_(router),
      on_close_(std::move(on_close)),
      last_activity_(std::time(nullptr)) {
    (void)socket_.set_non_blocking(true);
    (void)socket_.set_tcp_nodelay(true);
}

Connection::~Connection() {
    close();
}

void Connection::handle_events(int /*fd*/, int events) {
    if (closed_) return;

    last_activity_ = std::time(nullptr);

    if (events & EVENT_ERROR) {
        close();
        return;
    }

    if (events & EVENT_READ) {
        handle_read();
    }

    if (!closed_ && (events & EVENT_WRITE)) {
        handle_write();
    }
}

void Connection::handle_read() {
    int saved_errno = 0;
    ssize_t n = readBuffer_.read_fd(socket_.fd(), saved_errno);

    if (n > 0) {
        parser_.parse(readBuffer_);
        if (parser_.has_error()) {
            HttpResponse resp = HttpResponse::error(parser_.error_status());
            resp.set_keep_alive(false);
            keepAlive_ = false;

            std::string raw = resp.serialize();
            writeBuffer_.append(raw);
            loop_->modify(socket_.fd(), EVENT_READ | EVENT_WRITE);
            handle_write();
            return;
        }

        if (parser_.is_complete()) {
            process_request();
        }
    } else if (n == 0) {
        close();
    } else {
        if (saved_errno != EAGAIN && saved_errno != EWOULDBLOCK && saved_errno != EINTR) {
            close();
        }
    }
}

void Connection::process_request() {
    const auto& req = parser_.request();
    keepAlive_ = req.is_keep_alive();

    HttpResponse resp;
    if (router_) {
        resp = router_->handle(req);
    } else {
        resp = HttpResponse::error(HttpStatus::NotFound);
    }
    resp.set_keep_alive(keepAlive_);

    std::cout << "[INFO] " << req.method_str << " " << req.target << " "
              << static_cast<int>(resp.status()) << " (" << client_ip_ << ")\n";

    std::string raw = resp.serialize();
    writeBuffer_.append(raw);

    loop_->modify(socket_.fd(), EVENT_READ | EVENT_WRITE);
    handle_write();
}

void Connection::handle_write() {
    if (closed_) return;

    int saved_errno = 0;
    ssize_t n = writeBuffer_.write_fd(socket_.fd(), saved_errno);

    if (writeBuffer_.empty()) {
        if (keepAlive_) {
            reset_for_next_request();
            loop_->modify(socket_.fd(), EVENT_READ);

            if (!readBuffer_.empty()) {
                if (parser_.parse(readBuffer_) && parser_.is_complete()) {
                    process_request();
                }
            }
        } else {
            close();
        }
    } else if (n < 0) {
        if (saved_errno != EAGAIN && saved_errno != EWOULDBLOCK && saved_errno != EINTR) {
            close();
        }
    }
}

void Connection::reset_for_next_request() {
    parser_.reset();
    last_activity_ = std::time(nullptr);
}

void Connection::close() {
    if (closed_) return;
    closed_ = true;

    int fd = socket_.fd();
    if (fd >= 0 && loop_) {
        loop_->remove(fd);
    }
    socket_.close();

    if (on_close_ && fd >= 0) {
        on_close_(fd);
    }
}
