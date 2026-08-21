#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

enum class HttpStatus {
    OK = 200,
    BadRequest = 400,
    NotFound = 404,
    MethodNotAllowed = 405,
    InternalServerError = 500
};

class HttpResponse {
public:
    explicit HttpResponse(HttpStatus status = HttpStatus::OK);

    void set_status(HttpStatus s) noexcept { status_ = s; }
    [[nodiscard]] HttpStatus status() const noexcept { return status_; }

    void set_header(std::string key, std::string value);
    void set_content_type(std::string_view type) { set_header("Content-Type", std::string(type)); }
    void set_content_length(size_t len) { set_header("Content-Length", std::to_string(len)); }
    void set_keep_alive(bool keep_alive);

    void set_body(std::string body);
    [[nodiscard]] const std::string& body() const noexcept { return body_; }

    [[nodiscard]] std::string serialize_headers() const;
    [[nodiscard]] std::string serialize() const;

    static HttpResponse ok(std::string body, std::string_view content_type = "text/html; charset=utf-8");
    static HttpResponse error(HttpStatus status, std::string message = "");

private:
    HttpStatus status_{HttpStatus::OK};
    std::string version_{"HTTP/1.1"};
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    bool keep_alive_{true};

    [[nodiscard]] static std::string get_gmt_date();
};
