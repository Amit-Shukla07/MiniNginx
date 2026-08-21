#include "http/HttpHeaders.hpp"

#include <algorithm>

namespace mininginx::http {

static bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

void HttpHeaders::set(std::string key, std::string value) {
    for (auto& pair : headers_) {
        if (iequals(pair.first, key)) {
            pair.second = std::move(value);
            return;
        }
    }
    headers_.emplace_back(std::move(key), std::move(value));
}

void HttpHeaders::add(std::string key, std::string value) {
    headers_.emplace_back(std::move(key), std::move(value));
}

bool HttpHeaders::contains(std::string_view key) const {
    return std::any_of(headers_.begin(), headers_.end(), [key](const auto& pair) {
        return iequals(pair.first, key);
    });
}

std::optional<std::string_view> HttpHeaders::get(std::string_view key) const {
    for (const auto& pair : headers_) {
        if (iequals(pair.first, key)) {
            return pair.second;
        }
    }
    return std::nullopt;
}

void HttpHeaders::remove(std::string_view key) {
    headers_.erase(
        std::remove_if(headers_.begin(), headers_.end(), [key](const auto& pair) {
            return iequals(pair.first, key);
        }),
        headers_.end()
    );
}

void HttpHeaders::clear() {
    headers_.clear();
}

}  // namespace mininginx::http
