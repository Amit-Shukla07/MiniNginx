#include "Router.hpp"

#include <fstream>

Router::Router(std::string document_root) {
    set_document_root(std::move(document_root));
}

void Router::set_document_root(std::string root) {
    doc_root_ = std::move(root);
    std::error_code ec;
    canonical_root_ = std::filesystem::weakly_canonical(doc_root_, ec);
    if (ec) {
        canonical_root_ = std::filesystem::absolute(doc_root_);
    }
}

std::string_view Router::get_mime_type(std::string_view path) noexcept {
    auto dot = path.rfind('.');
    if (dot == std::string_view::npos) return "application/octet-stream";

    auto ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "js") return "application/javascript; charset=utf-8";
    if (ext == "json") return "application/json; charset=utf-8";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "txt") return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

std::optional<std::filesystem::path> Router::resolve_safe_path(std::string_view req_path) const {
    auto path = req_path;
    while (!path.empty() && path.front() == '/') path.remove_prefix(1);

    std::filesystem::path target = canonical_root_;
    if (path.empty()) {
        target /= "index.html";
    } else {
        target /= std::string(path);
    }

    std::error_code ec;
    std::filesystem::path canonical_target = std::filesystem::weakly_canonical(target, ec);
    if (ec) return std::nullopt;

    if (std::filesystem::is_directory(canonical_target, ec)) {
        canonical_target /= "index.html";
    }

    // Security check: Must reside within canonical document root
    std::string root_str = canonical_root_.lexically_normal().string();
    std::string target_str = canonical_target.lexically_normal().string();

    if (target_str.rfind(root_str, 0) != 0) {
        return std::nullopt;
    }

    if (!std::filesystem::exists(canonical_target, ec) || !std::filesystem::is_regular_file(canonical_target, ec)) {
        return std::nullopt;
    }

    return canonical_target;
}

HttpResponse Router::handle(const HttpRequest& request) const {
    if (request.method != HttpMethod::GET && request.method != HttpMethod::HEAD) {
        return HttpResponse::error(HttpStatus::MethodNotAllowed, "Method Not Allowed");
    }

    auto safe_path = resolve_safe_path(request.path);
    if (!safe_path.has_value()) {
        return HttpResponse::error(HttpStatus::NotFound, "File Not Found");
    }

    std::error_code ec;
    auto file_size = std::filesystem::file_size(*safe_path, ec);
    if (ec) {
        return HttpResponse::error(HttpStatus::InternalServerError, "Error reading file");
    }

    HttpResponse resp(HttpStatus::OK);
    resp.set_content_type(get_mime_type(safe_path->string()));
    resp.set_content_length(file_size);
    resp.set_keep_alive(request.is_keep_alive());

    if (request.method == HttpMethod::HEAD) {
        return resp;
    }

    std::ifstream file(*safe_path, std::ios::binary);
    if (!file.is_open()) {
        return HttpResponse::error(HttpStatus::NotFound, "Unable to open file");
    }

    std::string content(file_size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(file_size));
    resp.set_body(std::move(content));

    return resp;
}
