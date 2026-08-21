#include "http/MimeTypes.hpp"
#include "test_framework.hpp"

using namespace mininginx::http;

TEST(MimeTest, StandardExtensionLookup) {
    ASSERT_EQ(MimeTypes::get_mime_type("index.html"), "text/html; charset=utf-8");
    ASSERT_EQ(MimeTypes::get_mime_type("/public/assets/style.css"), "text/css; charset=utf-8");
    ASSERT_EQ(MimeTypes::get_mime_type("app.js"), "application/javascript; charset=utf-8");
    ASSERT_EQ(MimeTypes::get_mime_type("data.json"), "application/json; charset=utf-8");
    ASSERT_EQ(MimeTypes::get_mime_type("photo.png"), "image/png");
    ASSERT_EQ(MimeTypes::get_mime_type("photo.jpg"), "image/jpeg");
    ASSERT_EQ(MimeTypes::get_mime_type("icon.svg"), "image/svg+xml");
    ASSERT_EQ(MimeTypes::get_mime_type("binary.bin"), "application/octet-stream");
    ASSERT_EQ(MimeTypes::get_mime_type("no_extension_file"), "application/octet-stream");
}
