#pragma once

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "server/RequestHandler.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace mininginx::server {

class StaticFileHandler final : public RequestHandler {
public:
    explicit StaticFileHandler(std::string document_root);
    ~StaticFileHandler() override = default;

    http::HttpResponse handle(const http::HttpRequest& request) override;

    [[nodiscard]] const std::string& document_root() const noexcept { return document_root_; }
    [[nodiscard]] std::optional<std::filesystem::path> resolve_safe_path(std::string_view request_path) const;

private:
    std::string document_root_;
    std::filesystem::path canonical_root_;
};

}  // namespace mininginx::server
