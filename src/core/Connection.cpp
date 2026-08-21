#include "core/Connection.hpp"
#include "logging/Logger.hpp"
#include "metrics/Metrics.hpp"

#include <unistd.h>
#include <cerrno>

namespace mininginx::core {

Connection::Connection(
    Socket socket,
    std::string client_ip,
    uint16_t client_port,
    EventLoop* loop,
    TimerManager* timer_mgr,
    server::Router* router,
    Milliseconds keep_alive_timeout,
    Milliseconds request_timeout,
    http::ParserLimits limits
)
    : socket_(std::move(socket)),
      client_ip_(std::move(client_ip)),
      client_port_(client_port),
      loop_(loop),
      timer_mgr_(timer_mgr),
      router_(router),
      keep_alive_timeout_(keep_alive_timeout),
      request_timeout_(request_timeout),
      parser_(limits) {}

Connection::~Connection() {
    close();
}

void Connection::start() {
    (void)socket_.set_non_blocking(true);
    (void)socket_.set_tcp_nodelay(true);

    metrics::Metrics::instance().record_connection_opened();
    reset_timer();

    auto self = shared_from_this();
    loop_->add_fd(
        socket_.get_fd(),
        static_cast<uint32_t>(EventType::Read),
        [self](int fd, uint32_t events) {
            self->handle_events(fd, events);
        }
    );
}

void Connection::handle_events(int /*fd*/, uint32_t events) {
    if (state_ == ConnectionState::CLOSED) return;

    if (has_event(events, EventType::Error) || has_event(events, EventType::Hangup)) {
        close();
        return;
    }

    if (has_event(events, EventType::Read)) {
        handle_read();
    }

    if (state_ != ConnectionState::CLOSED && has_event(events, EventType::Write)) {
        handle_write();
    }
}

void Connection::handle_read() {
    int saved_errno = 0;
    ssize_t n = read_buf_.read_fd(socket_.get_fd(), saved_errno);

    if (n > 0) {
        metrics::Metrics::instance().record_bytes_read(static_cast<size_t>(n));
        reset_timer();

        if (state_ == ConnectionState::CONNECTED || state_ == ConnectionState::KEEP_ALIVE) {
            request_start_time_ = std::chrono::steady_clock::now();
        }

        parser_.parse(read_buf_);
        if (parser_.has_error()) {
            http::HttpResponse resp = http::HttpResponse::error(parser_.error_status());
            resp.set_keep_alive(false);
            keep_alive_ = false;

            std::string serialized = resp.serialize();
            write_buf_.append(serialized);
            metrics::Metrics::instance().record_response(to_code(resp.status()));

            state_ = ConnectionState::WRITING;
            loop_->modify_fd(socket_.get_fd(), static_cast<uint32_t>(EventType::Read | EventType::Write));
            handle_write();
            return;
        }

        if (parser_.is_complete()) {
            process_request();
        }
    } else if (n == 0) {
        // EOF from client
        close();
    } else {
        if (saved_errno != EAGAIN && saved_errno != EWOULDBLOCK && saved_errno != EINTR) {
            close();
        }
    }
}

void Connection::process_request() {
    state_ = ConnectionState::ROUTING;
    request_count_++;
    metrics::Metrics::instance().record_request();

    const auto& req = parser_.request();
    keep_alive_ = req.is_keep_alive();

    http::HttpResponse resp;
    if (router_) {
        resp = router_->route(req);
    } else {
        resp = http::HttpResponse::not_found();
    }

    resp.set_keep_alive(keep_alive_);

    std::string serialized = resp.serialize();
    write_buf_.append(serialized);

    metrics::Metrics::instance().record_response(to_code(resp.status()));

    // Observability / Access logging
    auto now = std::chrono::steady_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(now - request_start_time_).count();

    auto user_agent_opt = req.header("User-Agent");
    std::string_view user_agent = user_agent_opt.value_or("-");

    logging::Logger::instance().log_access(
        client_ip_,
        req.method_string(),
        req.uri(),
        req.version(),
        to_code(resp.status()),
        serialized.size(),
        latency_ms,
        user_agent
    );

    state_ = ConnectionState::WRITING;
    loop_->modify_fd(socket_.get_fd(), static_cast<uint32_t>(EventType::Read | EventType::Write));
    handle_write();
}

void Connection::handle_write() {
    if (state_ == ConnectionState::CLOSED) return;

    int saved_errno = 0;
    ssize_t n = write_buf_.write_fd(socket_.get_fd(), saved_errno);

    if (n > 0) {
        metrics::Metrics::instance().record_bytes_written(static_cast<size_t>(n));
        reset_timer();
    }

    if (write_buf_.empty()) {
        if (keep_alive_) {
            reset_for_next_request();
            loop_->modify_fd(socket_.get_fd(), static_cast<uint32_t>(EventType::Read));

            // Check if client pipelined the next request already in read_buf_
            if (!read_buf_.empty()) {
                request_start_time_ = std::chrono::steady_clock::now();
                if (parser_.parse(read_buf_) && parser_.is_complete()) {
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
    state_ = ConnectionState::KEEP_ALIVE;
    reset_timer();
}

void Connection::reset_timer() {
    if (!timer_mgr_) return;

    if (timer_id_ != 0) {
        timer_mgr_->cancel_timer(timer_id_);
    }

    auto self = shared_from_this();
    Milliseconds timeout = (state_ == ConnectionState::KEEP_ALIVE) ? keep_alive_timeout_ : request_timeout_;
    timer_id_ = timer_mgr_->add_timer(timeout, [self]() {
        self->on_timeout();
    });
}

void Connection::on_timeout() {
    if (state_ != ConnectionState::CLOSED) {
        close();
    }
}

void Connection::close() {
    if (state_ == ConnectionState::CLOSED) return;

    state_ = ConnectionState::CLOSED;

    if (timer_mgr_ && timer_id_ != 0) {
        timer_mgr_->cancel_timer(timer_id_);
        timer_id_ = 0;
    }

    int fd = socket_.get_fd();
    if (fd >= 0 && loop_) {
        loop_->remove_fd(fd);
    }

    socket_.close();
    metrics::Metrics::instance().record_connection_closed();

    if (close_cb_ && fd >= 0) {
        close_cb_(fd);
    }
}

}  // namespace mininginx::core
