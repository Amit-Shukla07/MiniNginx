#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

class Buffer {
public:
    explicit Buffer(size_t initial_size = 4096);

    [[nodiscard]] size_t readable_bytes() const noexcept { return writer_ - reader_; }
    [[nodiscard]] size_t writable_bytes() const noexcept { return data_.size() - writer_; }
    [[nodiscard]] bool empty() const noexcept { return readable_bytes() == 0; }

    [[nodiscard]] const char* peek() const noexcept { return data_.data() + reader_; }
    [[nodiscard]] char* begin_write() noexcept { return data_.data() + writer_; }

    void has_written(size_t len) noexcept { writer_ += len; }
    void retrieve(size_t len) noexcept;
    void retrieve_all() noexcept;
    [[nodiscard]] std::string retrieve_all_as_string();

    void append(const char* data, size_t len);
    void append(std::string_view str);

    [[nodiscard]] const char* find_crlf() const noexcept;
    [[nodiscard]] const char* find_double_crlf() const noexcept;

    ssize_t read_fd(int fd, int& saved_errno);
    ssize_t write_fd(int fd, int& saved_errno);

private:
    void ensure_writable(size_t len);

    std::vector<char> data_;
    size_t reader_{0};
    size_t writer_{0};
};
