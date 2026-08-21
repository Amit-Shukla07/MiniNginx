#pragma once

#include <iostream>
#include <string_view>

namespace mininginx::http {

enum class HttpMethod {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE_,
    OPTIONS,
    PATCH,
    UNKNOWN
};

inline constexpr std::string_view to_string_view(HttpMethod method) noexcept {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE_: return "DELETE";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

inline HttpMethod string_to_method(std::string_view sv) noexcept {
    if (sv == "GET") return HttpMethod::GET;
    if (sv == "HEAD") return HttpMethod::HEAD;
    if (sv == "POST") return HttpMethod::POST;
    if (sv == "PUT") return HttpMethod::PUT;
    if (sv == "DELETE") return HttpMethod::DELETE_;
    if (sv == "OPTIONS") return HttpMethod::OPTIONS;
    if (sv == "PATCH") return HttpMethod::PATCH;
    return HttpMethod::UNKNOWN;
}

inline std::ostream& operator<<(std::ostream& os, HttpMethod method) {
    return os << to_string_view(method);
}

}  // namespace mininginx::http
