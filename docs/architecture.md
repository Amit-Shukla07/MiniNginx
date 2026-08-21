# MiniNginx Architecture Specification

## 1. High-Level System Architecture

MiniNginx uses an **Event-Driven Reactor Pattern** with scalable Multi-Worker threading. Sockets are non-blocking and demultiplexed using OS-native readiness notifications (`kqueue` on macOS / BSD and `epoll` on Linux).

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
     └────────┴────────┘                    │
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

## 2. Subsystem Descriptions

### 2.1 Core Network Layer (`core/Socket.hpp`, `core/Buffer.hpp`)
* **`Socket`**: RAII file descriptor wrapper. Guarantees no socket leaks via move semantics and automatic `close()` upon destruction. Sets `O_NONBLOCK`, `SO_REUSEADDR`, `SO_REUSEPORT`, and `TCP_NODELAY` (disabling Nagle's algorithm for low-latency HTTP request-response pipelining).
* **`Buffer`**: Contiguous dynamic byte buffer maintaining explicit `reader_idx_` and `writer_idx_`. Supports fast compaction, zero-allocation reads via `readv` with a 64KB stack buffer, and sub-string search helpers (`find_crlf`, `find_double_crlf`).

### 2.2 Multiplexing Engine (`platform/KqueueEventLoop.hpp`, `platform/EpollEventLoop.hpp`)
* Abstracted behind `core::EventLoop`.
* macOS: Utilizes `kqueue()` with `EVFILT_READ` and `EVFILT_WRITE`.
* Linux: Utilizes `epoll_create1(EPOLL_CLOEXEC)` with `EPOLLIN`, `EPOLLOUT`, and `EPOLLRDHUP`.
* Both implementations are fully thread-safe for inter-thread descriptor modification and dispatch.

### 2.3 Connection State Machine (`core/Connection.hpp`)
Tracks single client lifecycle through distinct states:
`CONNECTED` ➔ `READING` ➔ `PARSING` ➔ `ROUTING` ➔ `WRITING` ➔ `KEEP_ALIVE` ➔ `CLOSING` ➔ `CLOSED`.
Each connection encapsulates its own buffers, parser, keep-alive timers, and request statistics.

### 2.4 HTTP Engine (`http/HttpParser.hpp`, `http/HttpRequest.hpp`, `http/HttpResponse.hpp`)
* Zero-copy string_view incremental Finite-State Machine (FSM).
* Processes chunked data, enforces header caps (64KB default) and payload bounds (10MB default).
* Fully supports RFC 7230 / RFC 9112 semantics, HTTP/1.0 and HTTP/1.1 persistent connections, and pipelined batch execution.

### 2.5 Zero-Copy File Transfer (`platform/MacFileSender.hpp`, `platform/LinuxFileSender.hpp`)
* Implements direct kernel-space static asset streaming via macOS `sendfile()` and Linux `sendfile()`.
* Handles partial writes, non-blocking `EAGAIN` retries, and offset synchronization without user-space buffer allocation.

### 2.6 Timer & Keep-Alive Manager (`core/TimerManager.hpp`)
* Min-heap priority queue storing millisecond-resolution deadlines.
* Computes exact timeout durations for `EventLoop::poll_and_dispatch()`, preventing CPU busy-looping while enforcing strict Slowloris and idle keep-alive timeouts.
