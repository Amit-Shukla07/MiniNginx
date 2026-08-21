#include "core/Buffer.hpp"

#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

namespace mininginx::core {

Buffer::Buffer(size_t initial_size)
    : buffer_(kInitialPrepend + initial_size),
      reader_idx_(kInitialPrepend),
      writer_idx_(kInitialPrepend) {}

void Buffer::retrieve(size_t len) {
    if (len < readable_bytes()) {
        reader_idx_ += len;
    } else {
        retrieve_all();
    }
}

void Buffer::retrieve_until(const char* end) {
    if (end >= peek() && end <= begin_write()) {
        retrieve(static_cast<size_t>(end - peek()));
    }
}

void Buffer::retrieve_all() {
    reader_idx_ = kInitialPrepend;
    writer_idx_ = kInitialPrepend;
}

std::string Buffer::retrieve_as_string(size_t len) {
    size_t actual_len = std::min(len, readable_bytes());
    std::string result(peek(), actual_len);
    retrieve(actual_len);
    return result;
}

std::string Buffer::retrieve_all_as_string() {
    return retrieve_as_string(readable_bytes());
}

std::string_view Buffer::as_string_view() const noexcept {
    return std::string_view(peek(), readable_bytes());
}

std::string_view Buffer::string_view_prefix(size_t len) const noexcept {
    return std::string_view(peek(), std::min(len, readable_bytes()));
}

void Buffer::append(const char* data, size_t len) {
    if (!data || len == 0) return;
    ensure_writable_bytes(len);
    std::copy(data, data + len, begin_write());
    has_written(len);
}

void Buffer::append(const void* data, size_t len) {
    append(static_cast<const char*>(data), len);
}

void Buffer::append(std::string_view str) {
    append(str.data(), str.size());
}

void Buffer::has_written(size_t len) {
    writer_idx_ += len;
}

void Buffer::ensure_writable_bytes(size_t len) {
    if (writable_bytes() < len) {
        make_space(len);
    }
}

void Buffer::make_space(size_t len) {
    if (writable_bytes() + prependable_bytes() < len + kInitialPrepend) {
        buffer_.resize(writer_idx_ + len);
    } else {
        // Move readable bytes to the front to compact
        size_t readable = readable_bytes();
        std::copy(begin() + reader_idx_, begin() + writer_idx_, begin() + kInitialPrepend);
        reader_idx_ = kInitialPrepend;
        writer_idx_ = reader_idx_ + readable;
    }
}

const char* Buffer::find_crlf() const noexcept {
    const char* crlf = static_cast<const char*>(
        std::memchr(peek(), '\r', readable_bytes())
    );
    while (crlf) {
        if (crlf + 1 < begin_write() && *(crlf + 1) == '\n') {
            return crlf;
        }
        size_t remaining = static_cast<size_t>(begin_write() - (crlf + 1));
        crlf = static_cast<const char*>(std::memchr(crlf + 1, '\r', remaining));
    }
    return nullptr;
}

const char* Buffer::find_double_crlf() const noexcept {
    if (readable_bytes() < 4) return nullptr;
    const char* p = peek();
    const char* end = begin_write() - 3;
    while (p <= end) {
        if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') {
            return p;
        }
        p++;
    }
    return nullptr;
}

ssize_t Buffer::read_fd(int fd, int& saved_errno) {
    char extra_buf[65536];
    struct iovec vec[2];
    const size_t writable = writable_bytes();

    vec[0].iov_base = begin() + writer_idx_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extra_buf;
    vec[1].iov_len = sizeof(extra_buf);

    const int iovcnt = (writable < sizeof(extra_buf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) {
        saved_errno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        writer_idx_ += static_cast<size_t>(n);
    } else {
        writer_idx_ = buffer_.size();
        append(extra_buf, static_cast<size_t>(n) - writable);
    }
    return n;
}

ssize_t Buffer::write_fd(int fd, int& saved_errno) {
    if (empty()) return 0;
    ssize_t n = ::write(fd, peek(), readable_bytes());
    if (n < 0) {
        saved_errno = errno;
    } else {
        retrieve(static_cast<size_t>(n));
    }
    return n;
}

}  // namespace mininginx::core
