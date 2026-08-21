#pragma once

#include "Buffer.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

enum class ParserState {
    START_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

class HttpParser {
public:
    HttpParser() = default;

    bool parse(Buffer& buffer);

    [[nodiscard]] bool is_complete() const noexcept { return state_ == ParserState::COMPLETE; }
    [[nodiscard]] bool has_error() const noexcept { return state_ == ParserState::ERROR; }
    [[nodiscard]] HttpStatus error_status() const noexcept { return error_status_; }
    [[nodiscard]] const HttpRequest& request() const noexcept { return request_; }
    [[nodiscard]] HttpRequest& request() noexcept { return request_; }

    void reset();

private:
    bool parse_start_line(Buffer& buffer);
    bool parse_headers(Buffer& buffer);
    bool parse_body(Buffer& buffer);

    ParserState state_{ParserState::START_LINE};
    HttpStatus error_status_{HttpStatus::BadRequest};
    HttpRequest request_;
    size_t content_length_{0};
    size_t total_headers_len_{0};
};
