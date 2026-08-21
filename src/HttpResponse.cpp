#include "HttpResponse.hpp"

#include <ctime>

static std::string_view get_reason(HttpStatus status) noexcept {
    switch (status) {
        case HttpStatus::OK: return "OK";
        case HttpStatus::BadRequest: return "Bad Request";
        case HttpStatus::NotFound: return "Not Found";
        case HttpStatus::MethodNotAllowed: return "Method Not Allowed";
        case HttpStatus::InternalServerError: return "Internal Server Error";
    }
    return "Unknown";
}

HttpResponse::HttpResponse(HttpStatus status) : status_(status) {
    headers_["Server"] = "MiniNginx/1.0";
    headers_["Date"] = get_gmt_date();
    set_keep_alive(true);
}

void HttpResponse::set_header(std::string key, std::string value) {
    headers_[std::move(key)] = std::move(value);
}

void HttpResponse::set_keep_alive(bool keep_alive) {
    keep_alive_ = keep_alive;
    if (keep_alive_) {
        headers_["Connection"] = "keep-alive";
    } else {
        headers_["Connection"] = "close";
    }
}

void HttpResponse::set_body(std::string body) {
    body_ = std::move(body);
    set_content_length(body_.size());
}

std::string HttpResponse::get_gmt_date() {
    std::time_t now = std::time(nullptr);
    struct tm gmt{};
#if defined(_WIN32)
    gmtime_s(&gmt, &now);
#else
    gmtime_r(&now, &gmt);
#endif
    char buf[128];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
    return std::string(buf);
}

std::string HttpResponse::serialize_headers() const {
    std::string result;
    result.reserve(256);

    result += version_;
    result += " ";
    result += std::to_string(static_cast<int>(status_));
    result += " ";
    result += get_reason(status_);
    result += "\r\n";

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

HttpResponse HttpResponse::error(HttpStatus status, std::string message) {
    HttpResponse resp(status);
    resp.set_content_type("text/html; charset=utf-8");
    if (message.empty()) {
        int code = static_cast<int>(status);
        std::string_view reason = get_reason(status);
        resp.set_body("<html><head><title>" + std::to_string(code) + " " + std::string(reason) +
                      "</title></head><body><center><h1>" + std::to_string(code) + " " +
                      std::string(reason) + "</h1></center><hr><center>MiniNginx</center></body></html>\n");
    } else {
        resp.set_body(std::move(message));
    }
    return resp;
}
