#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace mininginx::http {

enum class HttpStatus : int {
    // 2xx Success
    OK = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,

    // 3xx Redirection
    MovedPermanently = 301,
    Found = 302,
    NotModified = 304,

    // 4xx Client Errors
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    RequestTimeout = 408,
    PayloadTooLarge = 413,
    UriTooLong = 414,
    UnsupportedMediaType = 415,
    RequestHeaderFieldsTooLarge = 431,

    // 5xx Server Errors
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HttpVersionNotSupported = 505
};

inline constexpr int to_code(HttpStatus status) noexcept {
    return static_cast<int>(status);
}

inline constexpr std::string_view get_reason_phrase(HttpStatus status) noexcept {
    switch (status) {
        case HttpStatus::OK: return "OK";
        case HttpStatus::Created: return "Created";
        case HttpStatus::Accepted: return "Accepted";
        case HttpStatus::NoContent: return "No Content";
        case HttpStatus::MovedPermanently: return "Moved Permanently";
        case HttpStatus::Found: return "Found";
        case HttpStatus::NotModified: return "Not Modified";
        case HttpStatus::BadRequest: return "Bad Request";
        case HttpStatus::Unauthorized: return "Unauthorized";
        case HttpStatus::Forbidden: return "Forbidden";
        case HttpStatus::NotFound: return "Not Found";
        case HttpStatus::MethodNotAllowed: return "Method Not Allowed";
        case HttpStatus::RequestTimeout: return "Request Timeout";
        case HttpStatus::PayloadTooLarge: return "Payload Too Large";
        case HttpStatus::UriTooLong: return "URI Too Long";
        case HttpStatus::UnsupportedMediaType: return "Unsupported Media Type";
        case HttpStatus::RequestHeaderFieldsTooLarge: return "Request Header Fields Too Large";
        case HttpStatus::InternalServerError: return "Internal Server Error";
        case HttpStatus::NotImplemented: return "Not Implemented";
        case HttpStatus::BadGateway: return "Bad Gateway";
        case HttpStatus::ServiceUnavailable: return "Service Unavailable";
        case HttpStatus::GatewayTimeout: return "Gateway Timeout";
        case HttpStatus::HttpVersionNotSupported: return "HTTP Version Not Supported";
    }
    return "Unknown Status";
}

inline std::string get_default_body(HttpStatus status) {
    int code = to_code(status);
    std::string_view reason = get_reason_phrase(status);
    return "<!DOCTYPE html>\n<html>\n<head><title>" +
           std::to_string(code) + " " + std::string(reason) +
           "</title></head>\n<body>\n<center><h1>" +
           std::to_string(code) + " " + std::string(reason) +
           "</h1></center>\n<hr><center>MiniNginx/1.0.0</center>\n</body>\n</html>\n";
}

inline std::ostream& operator<<(std::ostream& os, HttpStatus status) {
    return os << to_code(status) << " " << get_reason_phrase(status);
}

}  // namespace mininginx::http
