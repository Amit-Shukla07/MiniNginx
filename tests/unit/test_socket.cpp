#include "core/Socket.hpp"
#include "test_framework.hpp"

using namespace mininginx::core;

TEST(SocketTest, RAIIAndMoveSemantics) {
    Socket sock = Socket::create_tcp();
    ASSERT_TRUE(sock.is_valid());
    int orig_fd = sock.get_fd();

    // Move construction
    Socket moved_sock(std::move(sock));
    ASSERT_FALSE(sock.is_valid());
    ASSERT_TRUE(moved_sock.is_valid());
    ASSERT_EQ(moved_sock.get_fd(), orig_fd);

    // Socket options
    ASSERT_TRUE(moved_sock.set_non_blocking(true));
    ASSERT_TRUE(moved_sock.set_reuse_addr(true));
    ASSERT_TRUE(moved_sock.set_tcp_nodelay(true));

    moved_sock.close();
    ASSERT_FALSE(moved_sock.is_valid());
}
