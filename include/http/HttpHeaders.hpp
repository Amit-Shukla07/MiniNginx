#pragma once

#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mininginx::http {

struct CaseInsensitiveHash {
    size_t operator()(std::string_view sv) const noexcept {
        size_t hash = 5381;
        for (char c : sv) {
            hash = ((hash << 5) + hash) + static_cast<size_t>(std::tolower(static_cast<unsigned char>(c)));
        }
        return hash;
    }
};

struct CaseInsensitiveEqual {
    bool operator()(std::string_view a, std::string_view b) const noexcept {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    }
};

class HttpHeaders {
public:
    HttpHeaders() = default;
    ~HttpHeaders() = default;

    void set(std::string key, std::string value);
    void add(std::string key, std::string value);
    [[nodiscard]] bool contains(std::string_view key) const;
    [[nodiscard]] std::optional<std::string_view> get(std::string_view key) const;
    void remove(std::string_view key);
    void clear();

    [[nodiscard]] size_t size() const noexcept { return headers_.size(); }
    [[nodiscard]] bool empty() const noexcept { return headers_.empty(); }

    using const_iterator = std::vector<std::pair<std::string, std::string>>::const_iterator;
    [[nodiscard]] const_iterator begin() const noexcept { return headers_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return headers_.end(); }

private:
    std::vector<std::pair<std::string, std::string>> headers_;
};

}  // namespace mininginx::http
