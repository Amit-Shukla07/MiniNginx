#include "http/HttpResponse.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace mininginx::http {

HttpResponse::HttpResponse(HttpStatus status) : status_(status) {
    headers_.set("Server", "MiniNginx/1.0.0");
    headers_.set("Date", get_current_gmt_date());
    set_keep_alive(true);
}

void HttpResponse::set_keep_alive(bool keep_alive) {
    keep_alive_ = keep_alive;
    if (keep_alive_) {
        headers_.set("Connection", "keep-alive");
    } else {
        headers_.set("Connection", "close");
    }
}

void HttpResponse::set_body(std::string b) {
    body_ = std::move(b);
    set_content_length(body_.size());
}

std::string HttpResponse::get_current_gmt_date() {
    std::time_t now = std::time(nullptr);
    struct tm gmt{};
#if defined(_WIN32)
    gmtime_s(&gmt, &now);
#else
    gmtime_r(&now, &gmt);
#endif
    char buf[128];
    // RFC 7231 / RFC 9110 date format: "Sun, 06 Nov 1994 08:49:37 GMT"
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
    return std::string(buf);
}

std::string HttpResponse::serialize_headers() const {
    std::string result;
    result.reserve(256);

    // Status Line: HTTP/1.1 200 OK\r\n
    result += version_;
    result += " ";
    result += std::to_string(to_code(status_));
    result += " ";
    result += get_reason_phrase(status_);
    result += "\r\n";

    // Headers
    for (const auto& [k, v] : headers_) {
        result += k;
        result += ": ";
        result += v;
        result += "\r\n";
    }

    result += "\r\n";
    return result;
}

std::string HttpResponse::serialize() const {
    std::string result = serialize_headers();
    result.append(body_);
    return result;
}

HttpResponse HttpResponse::ok(std::string body, std::string_view content_type) {
    HttpResponse resp(HttpStatus::OK);
    resp.set_content_type(content_type);
    resp.set_body(std::move(body));
    return resp;
}

HttpResponse HttpResponse::json(std::string json_str) {
    HttpResponse resp(HttpStatus::OK);
    resp.set_content_type("application/json; charset=utf-8");
    resp.set_body(std::move(json_str));
    return resp;
}

HttpResponse HttpResponse::error(HttpStatus status, std::string message) {
    HttpResponse resp(status);
    resp.set_content_type("text/html; charset=utf-8");
    if (message.empty()) {
        resp.set_body(get_default_body(status));
    } else {
        resp.set_body(std::move(message));
    }
    return resp;
}

HttpResponse HttpResponse::not_found(std::string message) {
    return error(HttpStatus::NotFound, std::move(message));
}

HttpResponse HttpResponse::redirect(std::string location, bool permanent) {
    HttpResponse resp(permanent ? HttpStatus::MovedPermanently : HttpStatus::Found);
    resp.set_header("Location", std::move(location));
    resp.set_body("");
    return resp;
}

}  // namespace mininginx::http
