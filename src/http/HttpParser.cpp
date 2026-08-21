#include "../../include/http/HttpParser.hpp"

#include <algorithm>
#include <cctype>

void HttpParser::reset() {
    state_ = ParserState::START_LINE;
    error_status_ = HttpStatus::BadRequest;
    request_.reset();
    content_length_ = 0;
    total_headers_len_ = 0;
}

bool HttpParser::parse(Buffer& buffer) {
    while (!buffer.empty() && state_ != ParserState::COMPLETE && state_ != ParserState::ERROR) {
        switch (state_) {
            case ParserState::START_LINE:
                if (!parse_start_line(buffer)) return state_ != ParserState::ERROR;
                break;
            case ParserState::HEADERS:
                if (!parse_headers(buffer)) return state_ != ParserState::ERROR;
                break;
            case ParserState::BODY:
                if (!parse_body(buffer)) return state_ != ParserState::ERROR;
                break;
            case ParserState::COMPLETE:
            case ParserState::ERROR:
                return state_ == ParserState::COMPLETE;
        }
    }
    return state_ != ParserState::ERROR;
}

bool HttpParser::parse_start_line(Buffer& buffer) {
    size_t eol_len = 0;
    const char* eol = buffer.find_eol(eol_len);
    if (!eol) {
        if (buffer.readable_bytes() > 8192) {
            state_ = ParserState::ERROR;
            error_status_ = HttpStatus::BadRequest;
            return false;
        }
        return false;
    }

    size_t len = static_cast<size_t>(eol - buffer.peek());
    std::string_view line(buffer.peek(), len);

    size_t s1 = line.find(' ');
    size_t s2 = (s1 != std::string_view::npos) ? line.find(' ', s1 + 1) : std::string_view::npos;

    if (s1 == std::string_view::npos) {
        state_ = ParserState::ERROR;
        error_status_ = HttpStatus::BadRequest;
        return false;
    }

    std::string_view m_str = line.substr(0, s1);
    std::string_view t_str = (s2 != std::string_view::npos) ? line.substr(s1 + 1, s2 - s1 - 1) : line.substr(s1 + 1);
    std::string_view v_str = (s2 != std::string_view::npos) ? line.substr(s2 + 1) : "HTTP/1.1";

    HttpMethod m = string_to_method(m_str);
    if (m == HttpMethod::UNKNOWN) {
        state_ = ParserState::ERROR;
        error_status_ = HttpStatus::MethodNotAllowed;
        return false;
    }

    if (v_str != "HTTP/1.1" && v_str != "HTTP/1.0") {
        state_ = ParserState::ERROR;
        error_status_ = HttpStatus::BadRequest;
        return false;
    }

    request_.method = m;
    request_.method_str = std::string(m_str);
    request_.target = std::string(t_str);
    request_.version = std::string(v_str);

    auto qpos = request_.target.find('?');
    if (qpos != std::string::npos) {
        request_.path = request_.target.substr(0, qpos);
        request_.query = request_.target.substr(qpos + 1);
    } else {
        request_.path = request_.target;
        request_.query.clear();
    }

    buffer.retrieve(len + eol_len);
    state_ = ParserState::HEADERS;
    return true;
}

bool HttpParser::parse_headers(Buffer& buffer) {
    while (true) {
        size_t eol_len = 0;
        const char* eol = buffer.find_eol(eol_len);
        if (!eol) {
            if (total_headers_len_ + buffer.readable_bytes() > 65536) {
                state_ = ParserState::ERROR;
                error_status_ = HttpStatus::BadRequest;
                return false;
            }
            return false;
        }

        size_t len = static_cast<size_t>(eol - buffer.peek());
        total_headers_len_ += (len + eol_len);

        if (total_headers_len_ > 65536) {
            state_ = ParserState::ERROR;
            error_status_ = HttpStatus::BadRequest;
            return false;
        }

        if (len == 0) {
            buffer.retrieve(eol_len);

            std::string cl = request_.get_header("content-length");
            if (!cl.empty()) {
                try {
                    content_length_ = std::stoull(cl);
                } catch (...) {
                    state_ = ParserState::ERROR;
                    error_status_ = HttpStatus::BadRequest;
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

        std::string_view line(buffer.peek(), len);
        size_t colon = line.find(':');
        if (colon == std::string_view::npos || colon == 0) {
            state_ = ParserState::ERROR;
            error_status_ = HttpStatus::BadRequest;
            return false;
        }

        std::string key(line.substr(0, colon));
        std::string_view val = line.substr(colon + 1);

        while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.remove_prefix(1);
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) val.remove_suffix(1);

        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        request_.headers[key] = std::string(val);
        buffer.retrieve(len + eol_len);
    }
}

bool HttpParser::parse_body(Buffer& buffer) {
    size_t needed = content_length_ - request_.body.size();
    size_t available = buffer.readable_bytes();
    size_t to_read = std::min(needed, available);

    if (to_read > 0) {
        request_.body.append(buffer.peek(), to_read);
        buffer.retrieve(to_read);
    }

    if (request_.body.size() == content_length_) {
        state_ = ParserState::COMPLETE;
        return true;
    }
    return false;
}
