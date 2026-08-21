#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mininginx::core {

class Buffer {
public:
    static constexpr size_t kInitialPrepend = 8;
    static constexpr size_t kInitialCapacity = 4096;

    explicit Buffer(size_t initial_size = kInitialCapacity);
    ~Buffer() = default;

    Buffer(const Buffer&) = default;
    Buffer& operator=(const Buffer&) = default;
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

    // Size queries
    [[nodiscard]] size_t readable_bytes() const noexcept {
        return writer_idx_ - reader_idx_;
    }
    [[nodiscard]] size_t writable_bytes() const noexcept {
        return buffer_.size() - writer_idx_;
    }
    [[nodiscard]] size_t prependable_bytes() const noexcept {
        return reader_idx_;
    }
    [[nodiscard]] bool empty() const noexcept {
        return readable_bytes() == 0;
    }

    // Direct pointer access
    [[nodiscard]] const char* peek() const noexcept {
        return begin() + reader_idx_;
    }
    [[nodiscard]] char* begin_write() noexcept {
        return begin() + writer_idx_;
    }
    [[nodiscard]] const char* begin_write() const noexcept {
        return begin() + writer_idx_;
    }

    // Retrieval operations
    void retrieve(size_t len);
    void retrieve_until(const char* end);
    void retrieve_all();
    [[nodiscard]] std::string retrieve_as_string(size_t len);
    [[nodiscard]] std::string retrieve_all_as_string();
    [[nodiscard]] std::string_view as_string_view() const noexcept;
    [[nodiscard]] std::string_view string_view_prefix(size_t len) const noexcept;

    // Append operations
    void append(const char* data, size_t len);
    void append(const void* data, size_t len);
    void append(std::string_view str);
    void has_written(size_t len);

    // Buffer management
    void ensure_writable_bytes(size_t len);

    // Search helpers
    [[nodiscard]] const char* find_crlf() const noexcept;
    [[nodiscard]] const char* find_double_crlf() const noexcept;

    // Socket I/O
    ssize_t read_fd(int fd, int& saved_errno);
    ssize_t write_fd(int fd, int& saved_errno);

private:
    [[nodiscard]] char* begin() noexcept { return buffer_.data(); }
    [[nodiscard]] const char* begin() const noexcept { return buffer_.data(); }
    void make_space(size_t len);

    std::vector<char> buffer_;
    size_t reader_idx_{kInitialPrepend};
    size_t writer_idx_{kInitialPrepend};
};

}  // namespace mininginx::core
