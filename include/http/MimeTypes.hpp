#pragma once

#include <string_view>

namespace mininginx::http {

class MimeTypes {
public:
    [[nodiscard]] static std::string_view get_mime_type(std::string_view path) noexcept;
    [[nodiscard]] static std::string_view get_mime_type_from_extension(std::string_view ext) noexcept;
};

}  // namespace mininginx::http
