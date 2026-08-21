#include "platform/LinuxFileSender.hpp"

#include <unistd.h>
#include <cerrno>

#ifdef MININGINX_PLATFORM_LINUX
#include <sys/sendfile.h>
#endif

namespace mininginx::platform {

SendResult LinuxFileSender::send(int sock_fd, int file_fd, off_t& in_out_offset, size_t& in_out_remaining) {
    if (in_out_remaining == 0) {
        return SendResult::Done;
    }

#ifdef MININGINX_PLATFORM_LINUX
    ssize_t sent = ::sendfile(sock_fd, file_fd, &in_out_offset, in_out_remaining);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return SendResult::WouldBlock;
        }
        return SendResult::Error;
    }

    if (static_cast<size_t>(sent) >= in_out_remaining) {
        in_out_remaining = 0;
        return SendResult::Done;
    }

    in_out_remaining -= static_cast<size_t>(sent);
    return SendResult::WouldBlock;
#else
    (void)sock_fd;
    (void)file_fd;
    (void)in_out_offset;
    (void)in_out_remaining;
    return SendResult::Error;
#endif
}

}  // namespace mininginx::platform

#ifdef MININGINX_PLATFORM_LINUX
namespace mininginx::platform {
std::unique_ptr<FileSender> FileSender::create() {
    return std::make_unique<LinuxFileSender>();
}
}  // namespace mininginx::platform
#endif
