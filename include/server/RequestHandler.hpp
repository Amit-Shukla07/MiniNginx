#pragma once

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

#include <functional>
#include <memory>

namespace mininginx::server {

using HandlerFunc = std::function<http::HttpResponse(const http::HttpRequest&)>;

class RequestHandler {
public:
    virtual ~RequestHandler() = default;
    virtual http::HttpResponse handle(const http::HttpRequest& request) = 0;
};

}  // namespace mininginx::server
