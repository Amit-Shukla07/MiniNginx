#include "Buffer.hpp"

#include <cassert>
#include <iostream>

void test_buffer_append_retrieve() {
    Buffer buf(16);
    assert(buf.empty());
    assert(buf.readable_bytes() == 0);

    buf.append("GET / HTTP/1.1\r\n");
    assert(buf.readable_bytes() == 16);
    assert(!buf.empty());

    assert(buf.find_crlf() != nullptr);
    assert(buf.retrieve_all_as_string() == "GET / HTTP/1.1\r\n");
    assert(buf.empty());
}

void test_buffer_fragmentation_compaction() {
    Buffer buf(32);
    buf.append("Part1");
    buf.retrieve(5);
    assert(buf.empty());

    std::string large(1000, 'A');
    buf.append(large);
    assert(buf.readable_bytes() == 1000);
    assert(buf.retrieve_all_as_string() == large);
}

void test_double_crlf() {
    Buffer buf(64);
    buf.append("Host: localhost\r\n\r\n");
    assert(buf.find_double_crlf() != nullptr);
}

int main() {
    test_buffer_append_retrieve();
    test_buffer_fragmentation_compaction();
    test_double_crlf();
    std::cout << "[PASS] All Buffer tests passed.\n";
    return 0;
}
