#include "http/HttpRequest.hpp"

#include <algorithm>

namespace mininginx::http {

void HttpRequest::set_uri(std::string u) {
    uri_ = std::move(u);
    parse_uri_components();
}

void HttpRequest::set_version(std::string v) {
    version_ = std::move(v);
    if (version_ == "HTTP/1.1") {
        major_version_ = 1;
        minor_version_ = 1;
    } else if (version_ == "HTTP/1.0") {
        major_version_ = 1;
        minor_version_ = 0;
    } else if (version_ == "HTTP/2.0" || version_ == "HTTP/2") {
        major_version_ = 2;
        minor_version_ = 0;
    }
}

void HttpRequest::parse_uri_components() {
    auto question_pos = uri_.find('?');
    if (question_pos != std::string::npos) {
        path_ = uri_.substr(0, question_pos);
        query_ = uri_.substr(question_pos + 1);
    } else {
        path_ = uri_;
        query_.clear();
    }
    query_parsed_ = false;
    parsed_query_params_.clear();
}

bool HttpRequest::is_keep_alive() const noexcept {
    auto conn = headers_.get("Connection");
    if (conn.has_value()) {
        std::string_view val = *conn;
        if (val.size() == 5 &&
            (val[0] == 'c' || val[0] == 'C') &&
            (val[1] == 'l' || val[1] == 'L') &&
            (val[2] == 'o' || val[2] == 'O') &&
            (val[3] == 's' || val[3] == 'S') &&
            (val[4] == 'e' || val[4] == 'E')) {
            return false;
        }
        if (val.size() == 10 &&
            (val[0] == 'k' || val[0] == 'K') &&
            (val[5] == 'a' || val[5] == 'A')) {
            return true;
        }
    }

    // Default by protocol version
    if (major_version_ == 1 && minor_version_ >= 1) {
        return true;  // HTTP/1.1 defaults to keep-alive
    }
    return false;     // HTTP/1.0 defaults to close
}

std::optional<std::string_view> HttpRequest::header(std::string_view key) const {
    return headers_.get(key);
}

std::optional<std::string_view> HttpRequest::query_param(std::string_view key) const {
    if (!query_parsed_ && !query_.empty()) {
        query_parsed_ = true;
        size_t start = 0;
        while (start < query_.size()) {
            size_t end = query_.find('&', start);
            if (end == std::string::npos) end = query_.size();

            std::string_view pair(query_.data() + start, end - start);
            size_t eq_pos = pair.find('=');
            if (eq_pos != std::string_view::npos) {
                parsed_query_params_[std::string(pair.substr(0, eq_pos))] =
                    std::string(pair.substr(eq_pos + 1));
            } else if (!pair.empty()) {
                parsed_query_params_[std::string(pair)] = "";
            }
            start = end + 1;
        }
    }

    auto it = parsed_query_params_.find(std::string(key));
    if (it != parsed_query_params_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void HttpRequest::reset() {
    method_ = HttpMethod::UNKNOWN;
    uri_.clear();
    path_.clear();
    query_.clear();
    version_ = "HTTP/1.1";
    major_version_ = 1;
    minor_version_ = 1;
    headers_.clear();
    body_.clear();
    parsed_query_params_.clear();
    query_parsed_ = false;
}

}  // namespace mininginx::http
