#pragma once

#include <sys/types.h>
#include <memory>

namespace mininginx::platform {

enum class SendResult {
    Done,
    WouldBlock,
    Error
};

class FileSender {
public:
    virtual ~FileSender() = default;

    // Zero-copy transfer of remaining bytes from file_fd to sock_fd starting at in_out_offset
    // Updates in_out_offset and in_out_remaining
    virtual SendResult send(int sock_fd, int file_fd, off_t& in_out_offset, size_t& in_out_remaining) = 0;

    [[nodiscard]] static std::unique_ptr<FileSender> create();
};

}  // namespace mininginx::platform
