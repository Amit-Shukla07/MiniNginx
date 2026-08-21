#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>

enum class HttpMethod {
    GET,
    HEAD,
    POST,
    UNKNOWN
};

inline HttpMethod string_to_method(std::string_view s) noexcept {
    if (s == "GET") return HttpMethod::GET;
    if (s == "HEAD") return HttpMethod::HEAD;
    if (s == "POST") return HttpMethod::POST;
    return HttpMethod::UNKNOWN;
}

inline std::string_view method_to_string(HttpMethod m) noexcept {
    switch (m) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::POST: return "POST";
        case HttpMethod::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

struct HttpRequest {
    HttpMethod method{HttpMethod::UNKNOWN};
    std::string method_str;
    std::string target;
    std::string path;
    std::string query;
    std::string version{"HTTP/1.1"};
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    [[nodiscard]] bool is_keep_alive() const noexcept {
        auto it = headers.find("connection");
        if (it != headers.end()) {
            std::string val = it->second;
            std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (val == "close") return false;
            if (val == "keep-alive") return true;
        }
        return version == "HTTP/1.1";
    }

    [[nodiscard]] std::string get_header(std::string key) const {
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        auto it = headers.find(key);
        if (it != headers.end()) return it->second;
        return "";
    }

    void reset() {
        method = HttpMethod::UNKNOWN;
        method_str.clear();
        target.clear();
        path.clear();
        query.clear();
        version = "HTTP/1.1";
        headers.clear();
        body.clear();
    }
};
