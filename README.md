# MiniNginx

> **A production-oriented, event-driven HTTP/1.1 web server written in modern C++20, using non-blocking sockets and OS-level I/O multiplexing with `kqueue` on macOS and `epoll` on Linux.**

[![Language](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Build](https://img.shields.io/badge/Build-CMake%20%7C%20Make-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/Platform-macOS%20(kqueue)%20%7C%20Linux%20(epoll)-orange.svg)]()
[![Sanitizers](https://img.shields.io/badge/Sanitizers-ASan%20%7C%20UBSan%20Clean-success.svg)]()
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

---

## Architecture Overview

MiniNginx uses an **Event-Driven Reactor Pattern** with scalable Multi-Worker threading. Sockets are non-blocking and demultiplexed using OS-native readiness notifications (`kqueue` on macOS and `epoll` on Linux).

```
                        HTTP Clients
                             │
                             ▼
               ┌───────────────────────────┐
               │   TCP Non-blocking Server │
               └─────────────┬─────────────┘
                             │
                             ▼
               ┌───────────────────────────┐
               │      Worker Manager       │
               └─────────────┬─────────────┘
                             │
              ┌──────────────┴──────────────┐
              │                             │
              ▼                             ▼
       ┌──────────────┐              ┌──────────────┐
       │   Worker 0   │    ...       │   Worker N   │
       │  (EventLoop) │              │  (EventLoop) │
       └──────┬───────┘              └──────┬───────┘
              │                             │
     ┌────────┴────────┐                    │
     │ Connection Mgr  │                    │
     └────────┬────────┘                    │
              │                             │
     ┌────────┴────────┐                    │
     │   HttpParser    │                    │
     └────────┬────────┘                    │
              │                             │
     ┌────────┴────────┐                    │
     │     Router      │                    │
     │ (Static & APIs) │                    │
     └────────┬────────┘                    │
              │                             │
     ┌────────┴────────┐                    │
     │  HttpResponse   │                    │
     └────────┬────────┘                    │
              │                             │
     ┌────────┴────────┐                    │
     │ ZeroCopy/Buffer │                    │
     └────────┬────────┘                    │
              │                             │
              ▼                             ▼
        Client Socket                 Client Socket
```

---

## Key Features

- ⚡ **High-Performance Event Multiplexing**: Native kernel event notification using `kqueue` (`EVFILT_READ`, `EVFILT_WRITE`) on macOS and `epoll` (`epoll_wait`, `EPOLLIN`, `EPOLLOUT`) on Linux.
- 🚀 **Zero-Copy File Transfer**: Direct kernel-space static asset streaming via platform-specialized `sendfile()` syscalls.
- 🧩 **Incremental FSM HTTP/1.1 Parser**: Deterministic zero-copy finite state machine handling fragmented packets, chunked transfer encoding, pipelining, and strict RFC 9112 limits.
- ⏱️ **Heap-Based Timer Manager**: O(log N) min-heap priority queue computing exact sleep durations for event loops, enforcing keep-alive and Slowloris request deadlines.
- 🔄 **HTTP/1.1 Keep-Alive & Pipelining**: Persistent connections supporting sequential and pipelined HTTP requests over a single TCP socket.
- 🛡️ **Defensive Security & Path Traversal Protection**: Strict filesystem canonicalization (`weakly_canonical`) preventing `../../etc/passwd` escapes; bounded header, URI, and body sizes.
- 📊 **Real-time Observability & Metrics**: Lock-free atomic runtime counters exposed via live JSON `/api/stats` and structured access logging.
- 👥 **Multi-Worker Scaling**: Independent event loops per worker thread with lock-free connection assignment and atomic metrics aggregation.
- 🛑 **Graceful Shutdown**: Intercepts `SIGINT`/`SIGTERM` to drain active sockets, cancel timers, join worker threads, and release resources with zero leaks.

---

## Benchmark Results (Measured)

Benchmarks executed on Apple Silicon (macOS Darwin ARM64) using the integrated benchmarking suite (`benchmarks/benchmark_server.py`) targeting `http://127.0.0.1:8888/api/health`:

| Concurrency | Requests | Success Rate | Requests / Sec (RPS) | Latency P50 | Latency P90 | Latency P99 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **10 Clients** | 2,000 | **100%** (2000/2000) | **17,628.68 req/s** | **0.45 ms** | 0.98 ms | 1.88 ms |
| **50 Clients** | 5,000 | **100%** (5000/5000) | **17,180.17 req/s** | **2.09 ms** | 4.91 ms | 9.14 ms |
| **100 Clients** | 10,000 | **100%** (10000/10000) | **17,054.27 req/s** | **3.43 ms** | 9.02 ms | 17.38 ms |
| **200 Clients** | 20,000 | **100%** (20000/20000) | **17,229.20 req/s** | **5.18 ms** | 13.14 ms | 24.47 ms |

*All 37,000 requests in the test suite completed with 0 errors.*

---

## Quick Start

### Prerequisites
* C++20 compliant compiler (`clang++` on macOS, `g++-12+` or `clang++` on Linux)
* CMake 3.20+
* Python 3.8+ (for integration and benchmark suites)

### 1. Build the Server
```bash
# Using Makefile
make build

# Or using CMake directly
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 2. Run MiniNginx
```bash
# Run with default configuration (config/server.conf)
./build/mininginx

# Or specify custom port and workers
./build/mininginx -p 8080 -w 4 -r ./public
```

### 3. Verify in Terminal
```bash
# Test static asset
curl http://localhost:8080/

# Test health check endpoint
curl http://localhost:8080/api/health

# Inspect live runtime metrics
curl http://localhost:8080/api/stats
```

Open your browser at `http://localhost:8080/` to explore the **Live Real-time Metrics Dashboard** and **Interactive HTTP Console**.

---

## Testing & Quality Assurance

### Run Unit Tests
```bash
make test
# Or: ./build/mininginx_unit_tests
```

### Run HTTP Parser Fuzzer (20,000 iterations)
```bash
./build/mininginx_fuzz_parser 20000
```

### Run Live Server Integration Tests
```bash
make test-integration
# Or: python3 tests/integration/test_server_integration.py
```

### Run Sanitizers (AddressSanitizer + UndefinedBehaviorSanitizer)
```bash
make sanitize
./build/asan/mininginx_unit_tests
```

### Run Stress & Resilience Test Suite
```bash
bash scripts/stress_test.sh
```

### Run Benchmark Suite
```bash
bash scripts/benchmark.sh
```

---

## Repository Structure

```
MiniNginx/
├── CMakeLists.txt              # CMake build definitions
├── CMakePresets.json           # Build presets (Debug, Release, ASan)
├── Makefile                    # Make convenience targets
├── README.md                   # Project documentation
├── LICENSE                     # MIT License
├── .gitignore                  # Git ignore rules
├── .clang-format               # Code formatting style
├── .clang-tidy                 # Static analysis configuration
│
├── config/
│   ├── server.conf             # Default production configuration
│   └── server.example.conf     # Example annotated configuration
│
├── include/
│   ├── core/                   # Socket, Buffer, EventLoop, Timer, Connection
│   ├── http/                   # HTTP parser, request, response, headers, mime
│   ├── server/                 # Router, static file handler, worker, server
│   ├── platform/               # Kqueue, epoll, macOS sendfile, Linux sendfile
│   ├── config/                 # Nginx-style config parser
│   ├── logging/                # Structured access & error logger
│   └── metrics/                # Lock-free atomic runtime stats
│
├── src/                        # Subsystem implementations
├── tests/
│   ├── unit/                   # CTest unit test suite
│   ├── integration/            # Python live server integration tests
│   ├── stress/                 # Connection storm & Slowloris stress tests
│   └── fuzz/                   # HTTP parser fuzzer
│
├── benchmarks/
│   ├── benchmark_server.py     # Concurrent load testing tool
│   └── results/                # Recorded benchmark results (JSON)
│
├── scripts/
│   ├── build.sh                # Automated build script
│   ├── test.sh                 # Full test runner script
│   ├── benchmark.sh            # Benchmark runner script
│   └── stress_test.sh          # Stress test launcher
│
├── public/                     # Modern web demo UI (HTML, CSS, JS)
│   ├── index.html
│   ├── style.css
│   └── app.js
│
├── docs/                       # Detailed architectural deep dives
│   ├── architecture.md
│   ├── event-loop.md
│   ├── http-parser.md
│   ├── connection-lifecycle.md
│   ├── performance.md
│   ├── security.md
│   ├── design-decisions.md
│   └── interview-qa.md
│
└── .github/workflows/
    └── ci.yml                  # GitHub Actions CI matrix for macOS and Ubuntu
```

---

## Configuration Reference

Example `config/server.conf`:
```nginx
server {
    host 0.0.0.0;
    port 8080;

    root ./public;

    workers 4;

    keep_alive_timeout 60;
    request_timeout 10;

    max_connections 10000;
    max_header_size 65536;
    max_body_size 10485760;

    access_log ./logs/access.log;
    error_log ./logs/error.log;
    log_level INFO;
}
```

---

## Technical Interview Preparation

A complete 25-question interview preparation guide covering network programming, socket state machines, I/O multiplexing, zero-copy mechanics, memory safety, and C++20 patterns is available in [docs/interview-qa.md](docs/interview-qa.md).

---

## License

This project is licensed under the [MIT License](LICENSE).
