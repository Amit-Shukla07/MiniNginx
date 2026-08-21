#include "config/Config.hpp"
#include "test_framework.hpp"

#include <fstream>

using namespace mininginx::config;

TEST(ConfigTest, DefaultValues) {
    ServerConfig cfg;
    std::string err;
    ASSERT_TRUE(cfg.validate(err));
    ASSERT_EQ(cfg.host, "0.0.0.0");
    ASSERT_EQ(cfg.port, 8080);
    ASSERT_EQ(cfg.workers, 1);
    ASSERT_EQ(cfg.keep_alive_timeout_sec, 60);
}

TEST(ConfigTest, LoadFromFile) {
    std::string tmp_cfg_path = "/tmp/mininginx_test.conf";
    {
        std::ofstream ofs(tmp_cfg_path);
        ofs << "server {\n"
            << "    host 127.0.0.1;\n"
            << "    port 9090;\n"
            << "    workers 4;\n"
            << "    root ./custom_public;\n"
            << "    keep_alive_timeout 45;\n"
            << "    request_timeout 5;\n"
            << "}\n";
    }

    ServerConfig cfg = ServerConfig::load_from_file(tmp_cfg_path);
    ASSERT_EQ(cfg.host, "127.0.0.1");
    ASSERT_EQ(cfg.port, 9090);
    ASSERT_EQ(cfg.workers, 4);
    ASSERT_EQ(cfg.root, "./custom_public");
    ASSERT_EQ(cfg.keep_alive_timeout_sec, 45);
    ASSERT_EQ(cfg.request_timeout_sec, 5);

    std::string err;
    ASSERT_TRUE(cfg.validate(err));
}

TEST(ConfigTest, ValidationFailures) {
    ServerConfig cfg;
    cfg.port = 0;
    std::string err;
    ASSERT_FALSE(cfg.validate(err));

    cfg.port = 8080;
    cfg.workers = 1000;
    ASSERT_FALSE(cfg.validate(err));
}
