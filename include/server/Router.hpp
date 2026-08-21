#pragma once

#include "http/HttpMethod.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "server/RequestHandler.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mininginx::server {

class Router {
public:
    Router() = default;
    ~Router() = default;

    void add_route(http::HttpMethod method, std::string path, HandlerFunc handler);
    void get(std::string path, HandlerFunc handler) { add_route(http::HttpMethod::GET, std::move(path), std::move(handler)); }
    void post(std::string path, HandlerFunc handler) { add_route(http::HttpMethod::POST, std::move(path), std::move(handler)); }
    void head(std::string path, HandlerFunc handler) { add_route(http::HttpMethod::HEAD, std::move(path), std::move(handler)); }

    void set_fallback_handler(HandlerFunc handler);

    [[nodiscard]] http::HttpResponse route(const http::HttpRequest& request) const;

private:
    struct RouteKey {
        http::HttpMethod method;
        std::string path;

        bool operator==(const RouteKey& other) const noexcept {
            return method == other.method && path == other.path;
        }
    };

    struct RouteKeyHash {
        size_t operator()(const RouteKey& key) const noexcept {
            return std::hash<std::string>()(key.path) ^ (static_cast<size_t>(key.method) << 16);
        }
    };

    std::unordered_map<RouteKey, HandlerFunc, RouteKeyHash> exact_routes_;
    HandlerFunc fallback_handler_;
};

}  // namespace mininginx::server
