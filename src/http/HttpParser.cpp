#include "http/HttpParser.hpp"

#include <cctype>
#include <charconv>
#include <cstdlib>

namespace mininginx::http {

HttpParser::HttpParser() : limits_(Limits{}) {}

HttpParser::HttpParser(Limits limits) : limits_(limits) {}

void HttpParser::set_error(HttpStatus status) {
    state_ = ParserState::ERROR;
    error_status_ = status;
}

void HttpParser::reset() {
    state_ = ParserState::REQUEST_LINE;
    error_status_ = HttpStatus::BadRequest;
    request_.reset();
    total_headers_size_ = 0;
    content_length_ = 0;
    is_chunked_ = false;
    current_chunk_size_ = 0;
    current_chunk_read_ = 0;
}

bool HttpParser::parse(core::Buffer& buffer) {
    while (!buffer.empty() && state_ != ParserState::COMPLETE && state_ != ParserState::ERROR) {
        switch (state_) {
            case ParserState::REQUEST_LINE: {
                if (!parse_request_line(buffer)) {
                    return state_ != ParserState::ERROR;
                }
                break;
            }
            case ParserState::HEADERS: {
                if (!parse_headers(buffer)) {
                    return state_ != ParserState::ERROR;
                }
                break;
            }
            case ParserState::BODY: {
                if (!parse_body(buffer)) {
                    return state_ != ParserState::ERROR;
                }
                break;
            }
            case ParserState::CHUNK_HEADER:
            case ParserState::CHUNK_BODY:
            case ParserState::CHUNK_CRLF:
            case ParserState::CHUNK_TRAILER: {
                if (!parse_chunked_body(buffer)) {
                    return state_ != ParserState::ERROR;
                }
                break;
            }
            case ParserState::COMPLETE:
            case ParserState::ERROR:
                return state_ == ParserState::COMPLETE;
        }
    }
    return state_ != ParserState::ERROR;
}

bool HttpParser::parse_request_line(core::Buffer& buffer) {
    const char* crlf = buffer.find_crlf();
    if (!crlf) {
        if (buffer.readable_bytes() > limits_.max_request_line_len) {
            set_error(HttpStatus::UriTooLong);
            return false;
        }
        return false;
    }

    size_t line_len = static_cast<size_t>(crlf - buffer.peek());
    if (line_len > limits_.max_request_line_len) {
        set_error(HttpStatus::UriTooLong);
        return false;
    }

    std::string_view line(buffer.peek(), line_len);

    size_t first_space = line.find(' ');
    if (first_space == std::string_view::npos) {
        set_error(HttpStatus::BadRequest);
        return false;
    }

    size_t second_space = line.find(' ', first_space + 1);
    if (second_space == std::string_view::npos) {
        set_error(HttpStatus::BadRequest);
        return false;
    }

    std::string_view method_str = line.substr(0, first_space);
    std::string_view uri_str = line.substr(first_space + 1, second_space - first_space - 1);
    std::string_view version_str = line.substr(second_space + 1);

    if (method_str.empty() || uri_str.empty() || version_str.empty()) {
        set_error(HttpStatus::BadRequest);
        return false;
    }

    HttpMethod method = string_to_method(method_str);
    if (method == HttpMethod::UNKNOWN) {
        set_error(HttpStatus::NotImplemented);
        return false;
    }
    request_.set_method(method);
    request_.set_uri(std::string(uri_str));

    if (version_str != "HTTP/1.1" && version_str != "HTTP/1.0") {
        set_error(HttpStatus::HttpVersionNotSupported);
        return false;
    }
    request_.set_version(std::string(version_str));

    buffer.retrieve(line_len + 2);
    state_ = ParserState::HEADERS;
    return true;
}

bool HttpParser::parse_headers(core::Buffer& buffer) {
    while (true) {
        const char* crlf = buffer.find_crlf();
        if (!crlf) {
            if (total_headers_size_ + buffer.readable_bytes() > limits_.max_header_size) {
                set_error(HttpStatus::RequestHeaderFieldsTooLarge);
                return false;
            }
            return false;
        }

        size_t line_len = static_cast<size_t>(crlf - buffer.peek());
        total_headers_size_ += (line_len + 2);

        if (total_headers_size_ > limits_.max_header_size) {
            set_error(HttpStatus::RequestHeaderFieldsTooLarge);
            return false;
        }

        if (line_len == 0) {
            buffer.retrieve(2);

            auto te = request_.headers().get("Transfer-Encoding");
            if (te.has_value() && te->find("chunked") != std::string_view::npos) {
                is_chunked_ = true;
                state_ = ParserState::CHUNK_HEADER;
                return true;
            }

            auto cl = request_.headers().get("Content-Length");
            if (cl.has_value()) {
                std::string cl_str(cl.value());
                try {
                    size_t pos = 0;
                    unsigned long long val = std::stoull(cl_str, &pos);
                    if (pos != cl_str.size()) {
                        set_error(HttpStatus::BadRequest);
                        return false;
                    }
                    if (val > limits_.max_body_size) {
                        set_error(HttpStatus::PayloadTooLarge);
                        return false;
                    }
                    content_length_ = static_cast<size_t>(val);
                } catch (...) {
                    set_error(HttpStatus::BadRequest);
                    return false;
                }

                if (content_length_ == 0) {
                    state_ = ParserState::COMPLETE;
                } else {
                    state_ = ParserState::BODY;
                }
                return true;
            }

            state_ = ParserState::COMPLETE;
            return true;
        }

        if (request_.headers().size() >= limits_.max_headers_count) {
            set_error(HttpStatus::RequestHeaderFieldsTooLarge);
            return false;
        }

        std::string_view line(buffer.peek(), line_len);
        size_t colon = line.find(':');
        if (colon == std::string_view::npos || colon == 0) {
            set_error(HttpStatus::BadRequest);
            return false;
        }

        std::string_view name = line.substr(0, colon);
        std::string_view value = line.substr(colon + 1);

        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.remove_prefix(1);
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.remove_suffix(1);
        }

        request_.headers().add(std::string(name), std::string(value));
        buffer.retrieve(line_len + 2);
    }
}

bool HttpParser::parse_body(core::Buffer& buffer) {
    size_t needed = content_length_ - request_.body().size();
    size_t available = buffer.readable_bytes();
    size_t to_read = (available < needed) ? available : needed;

    if (to_read > 0) {
        request_.append_body(buffer.string_view_prefix(to_read));
        buffer.retrieve(to_read);
    }

    if (request_.body().size() == content_length_) {
        state_ = ParserState::COMPLETE;
        return true;
    }

    return false;
}

bool HttpParser::parse_chunked_body(core::Buffer& buffer) {
    while (true) {
        switch (state_) {
            case ParserState::CHUNK_HEADER: {
                const char* crlf = buffer.find_crlf();
                if (!crlf) return false;

                size_t line_len = static_cast<size_t>(crlf - buffer.peek());
                std::string_view line(buffer.peek(), line_len);
                size_t semi = line.find(';');
                if (semi != std::string_view::npos) {
                    line = line.substr(0, semi);
                }

                unsigned long long chunk_sz = 0;
                char* end_ptr = nullptr;
                std::string hex_str(line);
                chunk_sz = std::strtoull(hex_str.c_str(), &end_ptr, 16);
                if (end_ptr == hex_str.c_str()) {
                    set_error(HttpStatus::BadRequest);
                    return false;
                }

                current_chunk_size_ = static_cast<size_t>(chunk_sz);
                current_chunk_read_ = 0;
                buffer.retrieve(line_len + 2);

                if (current_chunk_size_ == 0) {
                    state_ = ParserState::CHUNK_TRAILER;
                } else {
                    if (request_.body().size() + current_chunk_size_ > limits_.max_body_size) {
                        set_error(HttpStatus::PayloadTooLarge);
                        return false;
                    }
                    state_ = ParserState::CHUNK_BODY;
                }
                break;
            }
            case ParserState::CHUNK_BODY: {
                size_t needed = current_chunk_size_ - current_chunk_read_;
                size_t available = buffer.readable_bytes();
                size_t to_read = (available < needed) ? available : needed;

                if (to_read > 0) {
                    request_.append_body(buffer.string_view_prefix(to_read));
                    buffer.retrieve(to_read);
                    current_chunk_read_ += to_read;
                }

                if (current_chunk_read_ == current_chunk_size_) {
                    state_ = ParserState::CHUNK_CRLF;
                } else {
                    return false;
                }
                break;
            }
            case ParserState::CHUNK_CRLF: {
                const char* crlf = buffer.find_crlf();
                if (!crlf) return false;
                buffer.retrieve(2);
                state_ = ParserState::CHUNK_HEADER;
                break;
            }
            case ParserState::CHUNK_TRAILER: {
                const char* crlf = buffer.find_crlf();
                if (!crlf) return false;
                size_t line_len = static_cast<size_t>(crlf - buffer.peek());
                buffer.retrieve(line_len + 2);
                if (line_len == 0) {
                    state_ = ParserState::COMPLETE;
                    return true;
                }
                break;
            }
            default:
                return true;
        }
    }
}

}  // namespace mininginx::http
