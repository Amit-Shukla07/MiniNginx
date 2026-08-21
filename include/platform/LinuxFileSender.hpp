#pragma once

#include "platform/FileSender.hpp"

namespace mininginx::platform {

class LinuxFileSender final : public FileSender {
public:
    LinuxFileSender() = default;
    ~LinuxFileSender() override = default;

    SendResult send(int sock_fd, int file_fd, off_t& in_out_offset, size_t& in_out_remaining) override;
};

}  // namespace mininginx::platform
