#include "http/MimeTypes.hpp"

#include <cctype>
#include <string>

namespace mininginx::http {

std::string_view MimeTypes::get_mime_type_from_extension(std::string_view ext) noexcept {
    if (ext.empty()) return "application/octet-stream";

    // Skip leading dot if present
    if (ext.front() == '.') {
        ext.remove_prefix(1);
    }

    // Lookup lowercase
    if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "js" || ext == "mjs") return "application/javascript; charset=utf-8";
    if (ext == "json") return "application/json; charset=utf-8";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    if (ext == "webp") return "image/webp";
    if (ext == "pdf") return "application/pdf";
    if (ext == "txt" || ext == "text") return "text/plain; charset=utf-8";
    if (ext == "xml") return "application/xml; charset=utf-8";
    if (ext == "wasm") return "application/wasm";
    if (ext == "mp4") return "video/mp4";
    if (ext == "webm") return "video/webm";
    if (ext == "mp3") return "audio/mpeg";
    if (ext == "wav") return "audio/wav";
    if (ext == "woff") return "font/woff";
    if (ext == "woff2") return "font/woff2";
    if (ext == "ttf") return "font/ttf";
    if (ext == "otf") return "font/otf";
    if (ext == "zip") return "application/zip";
    if (ext == "gz") return "application/gzip";
    if (ext == "tar") return "application/x-tar";

    return "application/octet-stream";
}

std::string_view MimeTypes::get_mime_type(std::string_view path) noexcept {
    auto dot_pos = path.rfind('.');
    if (dot_pos == std::string_view::npos) {
        return "application/octet-stream";
    }
    // Also ensure no slash after dot
    auto slash_pos = path.rfind('/');
    if (slash_pos != std::string_view::npos && slash_pos > dot_pos) {
        return "application/octet-stream";
    }

    return get_mime_type_from_extension(path.substr(dot_pos + 1));
}

}  // namespace mininginx::http
