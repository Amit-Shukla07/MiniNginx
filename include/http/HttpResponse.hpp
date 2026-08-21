#pragma once

#include "http/HttpHeaders.hpp"
#include "http/HttpStatus.hpp"

#include <string>
#include <string_view>

namespace mininginx::http {

class HttpResponse {
public:
    explicit HttpResponse(HttpStatus status = HttpStatus::OK);
    ~HttpResponse() = default;

    HttpResponse(const HttpResponse&) = default;
    HttpResponse& operator=(const HttpResponse&) = default;
    HttpResponse(HttpResponse&&) noexcept = default;
    HttpResponse& operator=(HttpResponse&&) noexcept = default;

    // Status
    [[nodiscard]] HttpStatus status() const noexcept { return status_; }
    void set_status(HttpStatus s) noexcept { status_ = s; }

    // Headers
    [[nodiscard]] const HttpHeaders& headers() const noexcept { return headers_; }
    [[nodiscard]] HttpHeaders& headers() noexcept { return headers_; }
    void set_header(std::string key, std::string value) { headers_.set(std::move(key), std::move(value)); }
    void set_content_type(std::string_view type) { headers_.set("Content-Type", std::string(type)); }
    void set_content_length(size_t len) { headers_.set("Content-Length", std::to_string(len)); }
    void set_keep_alive(bool keep_alive);

    // Body
    [[nodiscard]] const std::string& body() const noexcept { return body_; }
    [[nodiscard]] std::string& body() noexcept { return body_; }
    void set_body(std::string b);

    // Serialization
    [[nodiscard]] std::string serialize_headers() const;
    [[nodiscard]] std::string serialize() const;

    // Factory methods
    [[nodiscard]] static HttpResponse ok(std::string body, std::string_view content_type = "text/html; charset=utf-8");
    [[nodiscard]] static HttpResponse json(std::string json_str);
    [[nodiscard]] static HttpResponse error(HttpStatus status, std::string message = "");
    [[nodiscard]] static HttpResponse not_found(std::string message = "");
    [[nodiscard]] static HttpResponse redirect(std::string location, bool permanent = false);

private:
    HttpStatus status_{HttpStatus::OK};
    std::string version_{"HTTP/1.1"};
    HttpHeaders headers_;
    std::string body_;
    bool keep_alive_{true};

    [[nodiscard]] static std::string get_current_gmt_date();
};

}  // namespace mininginx::http
