#include "server/StaticFileHandler.hpp"
#include "http/MimeTypes.hpp"

#include <fstream>
#include <sstream>

namespace mininginx::server {

StaticFileHandler::StaticFileHandler(std::string document_root)
    : document_root_(std::move(document_root)) {
    std::error_code ec;
    canonical_root_ = std::filesystem::weakly_canonical(document_root_, ec);
    if (ec) {
        canonical_root_ = std::filesystem::absolute(document_root_);
    }
}

std::optional<std::filesystem::path> StaticFileHandler::resolve_safe_path(std::string_view request_path) const {
    // Sanitize request path - remove query parameters or fragments if any
    auto clean_path = request_path;
    auto qpos = clean_path.find('?');
    if (qpos != std::string_view::npos) {
        clean_path = clean_path.substr(0, qpos);
    }

    // Strip leading slashes to append relative to root
    while (!clean_path.empty() && clean_path.front() == '/') {
        clean_path.remove_prefix(1);
    }

    std::filesystem::path target = canonical_root_;
    if (clean_path.empty()) {
        target /= "index.html";
    } else {
        target /= std::string(clean_path);
    }

    std::error_code ec;
    std::filesystem::path canonical_target = std::filesystem::weakly_canonical(target, ec);
    if (ec) {
        return std::nullopt;
    }

    // If target is a directory, append index.html
    if (std::filesystem::is_directory(canonical_target, ec)) {
        canonical_target /= "index.html";
    }

    // Security verification: The resolved canonical path MUST start with canonical_root_
    std::string root_str = canonical_root_.lexically_normal().string();
    std::string target_str = canonical_target.lexically_normal().string();

    if (target_str.rfind(root_str, 0) != 0) {
        // Path traversal attempt outside root!
        return std::nullopt;
    }

    if (!std::filesystem::exists(canonical_target, ec) || !std::filesystem::is_regular_file(canonical_target, ec)) {
        return std::nullopt;
    }

    return canonical_target;
}

http::HttpResponse StaticFileHandler::handle(const http::HttpRequest& request) {
    if (request.method() != http::HttpMethod::GET && request.method() != http::HttpMethod::HEAD) {
        return http::HttpResponse::error(http::HttpStatus::MethodNotAllowed, "Method Not Allowed");
    }

    auto safe_path_opt = resolve_safe_path(request.path());
    if (!safe_path_opt.has_value()) {
        return http::HttpResponse::not_found("File Not Found");
    }

    const auto& file_path = *safe_path_opt;
    std::error_code ec;
    auto file_size = std::filesystem::file_size(file_path, ec);
    if (ec) {
        return http::HttpResponse::error(http::HttpStatus::InternalServerError, "Failed to read file size");
    }

    auto mime_type = http::MimeTypes::get_mime_type(file_path.string());

    http::HttpResponse response(http::HttpStatus::OK);
    response.set_content_type(mime_type);
    response.set_content_length(file_size);
    response.set_keep_alive(request.is_keep_alive());

    if (request.method() == http::HttpMethod::HEAD) {
        // HEAD requires headers without response body
        return response;
    }

    // Read file content
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return http::HttpResponse::error(http::HttpStatus::Forbidden, "Unable to open file");
    }

    std::string content(file_size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(file_size));
    response.set_body(std::move(content));

    return response;
}

}  // namespace mininginx::server
