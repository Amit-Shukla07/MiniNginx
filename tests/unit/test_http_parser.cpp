#include "core/Buffer.hpp"
#include "http/HttpParser.hpp"
#include "test_framework.hpp"

using namespace mininginx::core;
using namespace mininginx::http;

TEST(HttpParserTest, StandardGetRequest) {
    HttpParser parser;
    Buffer buf;
    std::string raw = "GET /index.html HTTP/1.1\r\n"
                      "Host: localhost:8080\r\n"
                      "User-Agent: curl/8.4.0\r\n"
                      "Accept: */*\r\n\r\n";
    buf.append(raw);

    bool ok = parser.parse(buf);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(parser.is_complete());
    ASSERT_FALSE(parser.has_error());

    const auto& req = parser.request();
    ASSERT_EQ(req.method(), HttpMethod::GET);
    ASSERT_EQ(req.path(), "/index.html");
    ASSERT_EQ(req.version(), "HTTP/1.1");
    ASSERT_TRUE(req.is_keep_alive());
    ASSERT_EQ(req.header("Host").value_or(""), "localhost:8080");
}

TEST(HttpParserTest, FragmentedPacketParsing) {
    HttpParser parser;
    Buffer buf;

    // Send chunk 1
    buf.append("GET /api/st");
    ASSERT_TRUE(parser.parse(buf));
    ASSERT_FALSE(parser.is_complete());

    // Send chunk 2
    buf.append("ats HTTP/1.1\r\nHost: ");
    ASSERT_TRUE(parser.parse(buf));
    ASSERT_FALSE(parser.is_complete());

    // Send chunk 3
    buf.append("localhost\r\nConnection: close\r\n\r\n");
    ASSERT_TRUE(parser.parse(buf));
    ASSERT_TRUE(parser.is_complete());

    const auto& req = parser.request();
    ASSERT_EQ(req.method(), HttpMethod::GET);
    ASSERT_EQ(req.path(), "/api/stats");
    ASSERT_FALSE(req.is_keep_alive());
}

TEST(HttpParserTest, PostRequestBody) {
    HttpParser parser;
    Buffer buf;
    std::string payload = "{\"name\":\"MiniNginx\",\"speed\":\"fast\"}";
    std::string raw = "POST /api/echo HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: " + std::to_string(payload.size()) + "\r\n\r\n" +
                      payload;
    buf.append(raw);

    ASSERT_TRUE(parser.parse(buf));
    ASSERT_TRUE(parser.is_complete());

    const auto& req = parser.request();
    ASSERT_EQ(req.method(), HttpMethod::POST);
    ASSERT_EQ(req.path(), "/api/echo");
    ASSERT_EQ(req.body(), payload);
}

TEST(HttpParserTest, ChunkedTransferEncoding) {
    HttpParser parser;
    Buffer buf;
    std::string raw = "POST /upload HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Transfer-Encoding: chunked\r\n\r\n"
                      "5\r\nHello\r\n"
                      "6\r\n World\r\n"
                      "0\r\n\r\n";
    buf.append(raw);

    ASSERT_TRUE(parser.parse(buf));
    ASSERT_TRUE(parser.is_complete());
    ASSERT_EQ(parser.request().body(), "Hello World");
}

TEST(HttpParserTest, MalformedRequestRejection) {
    HttpParser parser;
    Buffer buf;
    buf.append("INVALID_METHOD / HTTP/1.1\r\n\r\n");

    parser.parse(buf);
    ASSERT_TRUE(parser.has_error());
    ASSERT_EQ(parser.error_status(), HttpStatus::NotImplemented);
}

TEST(HttpParserTest, HeaderSizeLimitEnforcement) {
    HttpParser::Limits limits{.max_header_size = 100};
    HttpParser parser(limits);
    Buffer buf;
    buf.append("GET / HTTP/1.1\r\n"
               "X-Very-Long-Header-Name: This is an extremely long value that definitely exceeds the 100 byte limit\r\n\r\n");

    parser.parse(buf);
    ASSERT_TRUE(parser.has_error());
    ASSERT_EQ(parser.error_status(), HttpStatus::RequestHeaderFieldsTooLarge);
}
