#pragma once

#include "http/HttpHeaders.hpp"
#include "http/HttpMethod.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mininginx::http {

class HttpRequest {
public:
    HttpRequest() = default;
    ~HttpRequest() = default;

    HttpRequest(const HttpRequest&) = default;
    HttpRequest& operator=(const HttpRequest&) = default;
    HttpRequest(HttpRequest&&) noexcept = default;
    HttpRequest& operator=(HttpRequest&&) noexcept = default;

    // Getters
    [[nodiscard]] HttpMethod method() const noexcept { return method_; }
    [[nodiscard]] std::string_view method_string() const noexcept { return to_string_view(method_); }
    [[nodiscard]] const std::string& uri() const noexcept { return uri_; }
    [[nodiscard]] const std::string& path() const noexcept { return path_; }
    [[nodiscard]] const std::string& query() const noexcept { return query_; }
    [[nodiscard]] const std::string& version() const noexcept { return version_; }
    [[nodiscard]] int major_version() const noexcept { return major_version_; }
    [[nodiscard]] int minor_version() const noexcept { return minor_version_; }
    [[nodiscard]] const HttpHeaders& headers() const noexcept { return headers_; }
    [[nodiscard]] HttpHeaders& headers() noexcept { return headers_; }
    [[nodiscard]] const std::string& body() const noexcept { return body_; }
    [[nodiscard]] std::string& body() noexcept { return body_; }

    // Setters
    void set_method(HttpMethod m) noexcept { method_ = m; }
    void set_uri(std::string u);
    void set_path(std::string p) { path_ = std::move(p); }
    void set_query(std::string q) { query_ = std::move(q); }
    void set_version(std::string v);
    void set_body(std::string b) { body_ = std::move(b); }
    void append_body(std::string_view chunk) { body_.append(chunk.data(), chunk.size()); }

    // Queries
    [[nodiscard]] bool is_keep_alive() const noexcept;
    [[nodiscard]] std::optional<std::string_view> header(std::string_view key) const;
    [[nodiscard]] std::optional<std::string_view> query_param(std::string_view key) const;

    // Reset
    void reset();

private:
    HttpMethod method_{HttpMethod::UNKNOWN};
    std::string uri_;
    std::string path_;
    std::string query_;
    std::string version_{"HTTP/1.1"};
    int major_version_{1};
    int minor_version_{1};
    HttpHeaders headers_;
    std::string body_;
    mutable std::unordered_map<std::string, std::string> parsed_query_params_;
    mutable bool query_parsed_{false};

    void parse_uri_components();
};

}  // namespace mininginx::http
