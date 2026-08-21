# MiniNginx Performance & Benchmark Report

## 1. Benchmarking Environment

* **Host Platform**: Apple Darwin (macOS 25.6.0 ARM64)
* **Compiler**: Apple Clang 21.0.0 (`-std=c++20 -O3 -Wall -Wextra -Wpedantic`)
* **OS Multiplexer**: Native `kqueue` & `kevent`
* **Test Tool**: Python concurrent thread benchmark suite (`benchmarks/benchmark_server.py`)
* **Endpoint Tested**: `http://127.0.0.1:8888/api/health`
* **Worker Threads**: 4

---

## 2. Actual Measured Results

| Concurrency Level | Total Requests | Completed | Failed | RPS (Requests/sec) | Latency P50 (ms) | Latency P90 (ms) | Latency P99 (ms) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **10 Clients** | 2,000 | 2,000 | 0 | **17,628.68** | **0.45 ms** | 0.98 ms | 1.88 ms |
| **50 Clients** | 5,000 | 5,000 | 0 | **17,180.17** | **2.09 ms** | 4.91 ms | 9.14 ms |
| **100 Clients** | 10,000 | 10,000 | 0 | **17,054.27** | **3.43 ms** | 9.02 ms | 17.38 ms |
| **200 Clients** | 20,000 | 20,000 | 0 | **17,229.20** | **5.18 ms** | 13.14 ms | 24.47 ms |

**Total Tested Requests in Suite**: 37,000 requests with **0 failures (100% success rate)**.

---

## 3. Key Performance Optimizations

1. **Zero-Copy Static File Transfer**: Employs kernel-level `sendfile()` syscall to transfer files directly from the page cache to the network socket, eliminating context switches and RAM copies between kernel space and user space.
2. **Buffer Compaction with `readv`**: Uses a 64KB stack buffer along with the writable vector space in `Buffer::read_fd()`, reading up to 68KB in a single non-blocking syscall.
3. **TCP_NODELAY**: Sockets disable Nagle's algorithm (`TCP_NODELAY = 1`), reducing latency for HTTP request-response round-trips.
4. **Lock-Free Atomic Observability**: Server metrics (requests, connections, bytes, status code distribution) use C++20 `std::atomic` relaxed operations to avoid lock contention on hot request paths.
5. **Pre-allocated Event Lists**: `kevent` and `epoll_event` vectors are pre-allocated to 1024 slots per loop, avoiding heap allocations during event dispatch.
