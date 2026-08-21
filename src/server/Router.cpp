#include "server/Router.hpp"

namespace mininginx::server {

void Router::add_route(http::HttpMethod method, std::string path, HandlerFunc handler) {
    RouteKey key{.method = method, .path = std::move(path)};
    exact_routes_[std::move(key)] = std::move(handler);
}

void Router::set_fallback_handler(HandlerFunc handler) {
    fallback_handler_ = std::move(handler);
}

http::HttpResponse Router::route(const http::HttpRequest& request) const {
    RouteKey key{.method = request.method(), .path = request.path()};
    auto it = exact_routes_.find(key);
    if (it != exact_routes_.end() && it->second) {
        return it->second(request);
    }

    if (fallback_handler_) {
        return fallback_handler_(request);
    }

    return http::HttpResponse::not_found();
}

}  // namespace mininginx::server
