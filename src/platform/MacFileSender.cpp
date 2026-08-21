#include "platform/MacFileSender.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>

namespace mininginx::platform {

SendResult MacFileSender::send(int sock_fd, int file_fd, off_t& in_out_offset, size_t& in_out_remaining) {
    if (in_out_remaining == 0) {
        return SendResult::Done;
    }

    off_t len = static_cast<off_t>(in_out_remaining);
    // macOS sendfile: int sendfile(int fd, int s, off_t offset, off_t *len, struct sf_hdtr *hdtr, int flags);
    int ret = ::sendfile(file_fd, sock_fd, in_out_offset, &len, nullptr, 0);

    // len now contains the number of bytes actually transmitted before return
    if (len > 0) {
        in_out_offset += len;
        if (static_cast<size_t>(len) >= in_out_remaining) {
            in_out_remaining = 0;
            return SendResult::Done;
        }
        in_out_remaining -= static_cast<size_t>(len);
    }

    if (ret == 0) {
        return (in_out_remaining == 0) ? SendResult::Done : SendResult::WouldBlock;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return SendResult::WouldBlock;
    }

    return SendResult::Error;
}

}  // namespace mininginx::platform

#ifdef MININGINX_PLATFORM_MACOS
namespace mininginx::platform {
std::unique_ptr<FileSender> FileSender::create() {
    return std::make_unique<MacFileSender>();
}
}  // namespace mininginx::platform
#endif
