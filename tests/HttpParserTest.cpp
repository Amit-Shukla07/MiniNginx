#include "HttpParser.hpp"

#include <cassert>
#include <iostream>

void test_valid_get() {
    HttpParser parser;
    Buffer buf;
    buf.append("GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: test\r\n\r\n");

    assert(parser.parse(buf));
    assert(parser.is_complete());
    assert(!parser.has_error());

    assert(parser.request().method == HttpMethod::GET);
    assert(parser.request().path == "/index.html");
    assert(parser.request().version == "HTTP/1.1");
    assert(parser.request().get_header("host") == "localhost:8080");
    assert(parser.request().is_keep_alive());
}

void test_fragmented_request() {
    HttpParser parser;
    Buffer buf;

    buf.append("GET /style.");
    assert(parser.parse(buf));
    assert(!parser.is_complete());

    buf.append("css HTTP/1.1\r\nHost: loc");
    assert(parser.parse(buf));
    assert(!parser.is_complete());

    buf.append("alhost\r\nConnection: close\r\n\r\n");
    assert(parser.parse(buf));
    assert(parser.is_complete());

    assert(parser.request().path == "/style.css");
    assert(!parser.request().is_keep_alive());
}

void test_post_with_body() {
    HttpParser parser;
    Buffer buf;
    std::string payload = "key=value&num=123";
    buf.append("POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: " +
               std::to_string(payload.size()) + "\r\n\r\n" + payload);

    assert(parser.parse(buf));
    assert(parser.is_complete());
    assert(parser.request().method == HttpMethod::POST);
    assert(parser.request().body == payload);
}

void test_malformed_request() {
    HttpParser parser;
    Buffer buf;
    buf.append("GARBAGE_HTTP_REQUEST\r\n\r\n");

    parser.parse(buf);
    assert(parser.has_error());
}

int main() {
    test_valid_get();
    test_fragmented_request();
    test_post_with_body();
    test_malformed_request();
    std::cout << "[PASS] All HttpParser tests passed.\n";
    return 0;
}
