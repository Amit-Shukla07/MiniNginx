#include "../../include/server/Buffer.hpp"

#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

Buffer::Buffer(size_t initial_size) : data_(initial_size) {}

void Buffer::retrieve(size_t len) noexcept {
    if (len < readable_bytes()) {
        reader_ += len;
    } else {
        retrieve_all();
    }
}

void Buffer::retrieve_all() noexcept {
    reader_ = 0;
    writer_ = 0;
}

std::string Buffer::retrieve_all_as_string() {
    std::string str(peek(), readable_bytes());
    retrieve_all();
    return str;
}

void Buffer::append(const char* data, size_t len) {
    if (!data || len == 0) return;
    ensure_writable(len);
    std::memcpy(begin_write(), data, len);
    has_written(len);
}

void Buffer::append(std::string_view str) {
    append(str.data(), str.size());
}

void Buffer::ensure_writable(size_t len) {
    if (writable_bytes() >= len) return;

    if (writable_bytes() + reader_ >= len) {
        size_t readable = readable_bytes();
        std::memmove(data_.data(), peek(), readable);
        reader_ = 0;
        writer_ = readable;
    } else {
        data_.resize(writer_ + len);
    }
}

const char* Buffer::find_crlf() const noexcept {
    if (empty()) return nullptr;
    const char* p = peek();
    const char* end = p + readable_bytes() - 1;
    for (; p < end; ++p) {
        if (*p == '\r' && *(p + 1) == '\n') {
            return p;
        }
    }
    return nullptr;
}

const char* Buffer::find_double_crlf() const noexcept {
    if (readable_bytes() < 4) return nullptr;
    const char* p = peek();
    const char* end = p + readable_bytes() - 3;
    for (; p <= end; ++p) {
        if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') {
            return p;
        }
    }
    return nullptr;
}

ssize_t Buffer::read_fd(int fd, int& saved_errno) {
    char extra_buf[65536];
    struct iovec vec[2];
    const size_t writable = writable_bytes();

    vec[0].iov_base = begin_write();
    vec[0].iov_len = writable;
    vec[1].iov_base = extra_buf;
    vec[1].iov_len = sizeof(extra_buf);

    const int iovcnt = (writable < sizeof(extra_buf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) {
        saved_errno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        writer_ += static_cast<size_t>(n);
    } else {
        writer_ = data_.size();
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
