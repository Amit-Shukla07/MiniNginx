#pragma once

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

class Router {
public:
    explicit Router(std::string document_root = "./public");

    [[nodiscard]] HttpResponse handle(const HttpRequest& request) const;
    [[nodiscard]] std::optional<std::filesystem::path> resolve_safe_path(std::string_view req_path) const;
    [[nodiscard]] static std::string_view get_mime_type(std::string_view path) noexcept;

    void set_document_root(std::string root);
    [[nodiscard]] const std::string& document_root() const noexcept { return doc_root_; }

private:
    std::string doc_root_;
    std::filesystem::path canonical_root_;
};
