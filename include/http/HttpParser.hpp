#pragma once

#include "core/Buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpStatus.hpp"

namespace mininginx::http {

enum class ParserState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    CHUNK_HEADER,
    CHUNK_BODY,
    CHUNK_CRLF,
    CHUNK_TRAILER,
    COMPLETE,
    ERROR
};

struct ParserLimits {
    size_t max_request_line_len{8192};
    size_t max_header_size{65536};
    size_t max_headers_count{128};
    size_t max_body_size{10 * 1024 * 1024};  // 10 MB
};

class HttpParser {
public:
    using Limits = ParserLimits;

    HttpParser();
    explicit HttpParser(Limits limits);
    ~HttpParser() = default;

    // Parse bytes from buffer incrementally
    bool parse(core::Buffer& buffer);

    [[nodiscard]] bool is_complete() const noexcept { return state_ == ParserState::COMPLETE; }
    [[nodiscard]] bool has_error() const noexcept { return state_ == ParserState::ERROR; }
    [[nodiscard]] ParserState state() const noexcept { return state_; }
    [[nodiscard]] HttpStatus error_status() const noexcept { return error_status_; }
    [[nodiscard]] const HttpRequest& request() const noexcept { return request_; }
    [[nodiscard]] HttpRequest& request() noexcept { return request_; }

    void reset();

private:
    bool parse_request_line(core::Buffer& buffer);
    bool parse_headers(core::Buffer& buffer);
    bool parse_body(core::Buffer& buffer);
    bool parse_chunked_body(core::Buffer& buffer);

    void set_error(HttpStatus status);

    Limits limits_;
    ParserState state_{ParserState::REQUEST_LINE};
    HttpStatus error_status_{HttpStatus::BadRequest};
    HttpRequest request_;

    size_t total_headers_size_{0};
    size_t content_length_{0};
    bool is_chunked_{false};
    size_t current_chunk_size_{0};
    size_t current_chunk_read_{0};
};

}  // namespace mininginx::http
