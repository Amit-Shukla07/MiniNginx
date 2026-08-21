#include "core/Buffer.hpp"
#include "test_framework.hpp"

using namespace mininginx::core;

TEST(BufferTest, InitialState) {
    Buffer buf(1024);
    ASSERT_EQ(buf.readable_bytes(), 0);
    ASSERT_EQ(buf.writable_bytes(), 1024);
    ASSERT_TRUE(buf.empty());
}

TEST(BufferTest, AppendAndRetrieve) {
    Buffer buf(64);
    std::string test_data = "GET /index.html HTTP/1.1\r\n";
    buf.append(test_data);

    ASSERT_EQ(buf.readable_bytes(), test_data.size());
    ASSERT_FALSE(buf.empty());
    ASSERT_EQ(buf.as_string_view(), test_data);

    std::string retrieved = buf.retrieve_as_string(test_data.size());
    ASSERT_EQ(retrieved, test_data);
    ASSERT_EQ(buf.readable_bytes(), 0);
    ASSERT_TRUE(buf.empty());
}

TEST(BufferTest, DynamicGrowthAndCompaction) {
    Buffer buf(16);
    std::string large_chunk(5000, 'X');
    buf.append(large_chunk);

    ASSERT_EQ(buf.readable_bytes(), 5000);
    ASSERT_EQ(buf.retrieve_all_as_string(), large_chunk);
    ASSERT_TRUE(buf.empty());

    // Subsequent append uses compacted space
    buf.append("Hello World");
    ASSERT_EQ(buf.readable_bytes(), 11);
}

TEST(BufferTest, CRLFFinding) {
    Buffer buf(128);
    buf.append("Line 1\r\nLine 2\r\n\r\n");

    const char* crlf1 = buf.find_crlf();
    ASSERT_TRUE(crlf1 != nullptr);
    ASSERT_EQ(std::string_view(buf.peek(), static_cast<size_t>(crlf1 - buf.peek())), "Line 1");

    const char* double_crlf = buf.find_double_crlf();
    ASSERT_TRUE(double_crlf != nullptr);
}
