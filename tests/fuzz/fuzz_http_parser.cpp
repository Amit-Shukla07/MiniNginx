#include "core/Buffer.hpp"
#include "http/HttpParser.hpp"

#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

void run_fuzzer(size_t iterations) {
    std::cout << "Starting MiniNginx HTTP Parser Fuzz Tester (" << iterations << " iterations)...\n";

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<size_t> len_dist(1, 2048);

    const std::vector<std::string> seed_corpus = {
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",
        "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\n0123456789",
        "POST /chunked HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWiki\r\n0\r\n\r\n",
        "GET /a/../b HTTP/1.0\r\n\r\n",
        "INVALID HTTP SYNTAX\r\n\r\n",
        "GET / HTTP/1.1\r\nLong: " + std::string(8000, 'A') + "\r\n\r\n"
    };

    size_t successful_parses = 0;
    size_t rejected_parses = 0;

    for (size_t iter = 0; iter < iterations; ++iter) {
        mininginx::http::HttpParser parser;
        mininginx::core::Buffer buf;

        // Either take a seed and mutate, or generate pure random garbage
        std::string payload;
        if (iter < seed_corpus.size()) {
            payload = seed_corpus[iter];
        } else if (rng() % 2 == 0 && !seed_corpus.empty()) {
            payload = seed_corpus[rng() % seed_corpus.size()];
            // Mutate a few random bytes
            size_t num_mutations = 1 + (rng() % 10);
            for (size_t m = 0; m < num_mutations && !payload.empty(); ++m) {
                size_t pos = rng() % payload.size();
                payload[pos] = static_cast<char>(byte_dist(rng));
            }
        } else {
            size_t len = len_dist(rng);
            payload.resize(len);
            for (size_t i = 0; i < len; ++i) {
                payload[i] = static_cast<char>(byte_dist(rng));
            }
        }

        buf.append(payload);
        bool ok = parser.parse(buf);

        if (ok && parser.is_complete()) {
            successful_parses++;
        } else {
            rejected_parses++;
        }
    }

    std::cout << "[FUZZ PASS] Fuzzed " << iterations << " inputs without crashing!\n"
              << "  Parsed successfully: " << successful_parses << "\n"
              << "  Safely rejected/incomplete: " << rejected_parses << "\n";
}

int main(int argc, char* argv[]) {
    size_t iters = 10000;
    if (argc > 1) {
        iters = static_cast<size_t>(std::stoul(argv[1]));
    }
    run_fuzzer(iters);
    return 0;
}
