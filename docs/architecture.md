# MiniNginx Architecture

MiniNginx is a small, event-driven HTTP/1.1 web server implemented in C++20. It demonstrates how a single-threaded server efficiently handles multiple concurrent connections using non-blocking I/O and kernel-level event multiplexing.

---

## 1. System Overview

```
                      Client Browser / curl
                                │
                                ▼
                       ┌─────────────────┐
                       │  Listening TCP  │
                       │     Socket      │
                       └────────┬────────┘
                                │
                                ▼
                       ┌─────────────────┐
                       │   Event Loop    │
                       │ (kqueue / epoll)│
                       └────────┬────────┘
                                │
                                ▼
                       ┌─────────────────┐
                       │   Connection    │
                       │ (Buffer/Parser) │
                       └────────┬────────┘
                                │
                                ▼
                       ┌─────────────────┐
                       │     Router      │
                       │ (Static Files)  │
                       └─────────────────┘
```

---

## 2. Core Components

### Sockets & Non-Blocking Mode (`Socket.hpp`, `Socket.cpp`)
All sockets wrap standard POSIX file descriptors using RAII. Sockets are placed into non-blocking mode via:
```cpp
fcntl(fd, F_SETFL, O_NONBLOCK);
```
In non-blocking mode, socket reads and writes never stall the thread. If no data is available or kernel send buffers are full, syscalls return `-1` with `errno` set to `EAGAIN` / `EWOULDBLOCK`, allowing the server to immediately resume polling other connections.

### Event Multiplexing (`EventLoop.hpp`, `KqueueEventLoop.cpp`, `EpollEventLoop.cpp`)
The server uses native OS multiplexing to monitor descriptors for readiness:
* **macOS / BSD**: `kqueue()` and `kevent()` with `EVFILT_READ` and `EVFILT_WRITE`.
* **Linux**: `epoll_create1()`, `epoll_ctl()`, and `epoll_wait()` with `EPOLLIN` and `EPOLLOUT`.

When the listening socket is readable, the server accepts incoming connections in a loop until `EAGAIN`. When client sockets are readable/writable, the loop triggers corresponding connection handlers.

### Dynamic Buffer (`Buffer.hpp`, `Buffer.cpp`)
TCP is an arbitrary byte stream. An HTTP request may arrive fragmented across multiple `read()` calls, or multiple requests may arrive in a single packet.
`Buffer` maintains contiguous storage with `reader_` and `writer_` indices, supporting:
* Scatter/gather reads via `readv()` with a 64KB stack buffer.
* In-place compaction to reclaim consumed space without unnecessary allocations.

### Incremental HTTP/1.1 Parser (`HttpParser.hpp`, `HttpParser.cpp`)
A deterministic finite-state machine (FSM):
`START_LINE` ➔ `HEADERS` ➔ `BODY` ➔ `COMPLETE` (or `ERROR`).
The parser consumes raw bytes from the connection's `Buffer` and constructs an `HttpRequest` object without blocking.

### Static File Serving & Security (`Router.hpp`, `Router.cpp`)
* **Path Traversal Defense**: The router validates that every requested canonical path resides strictly inside `document_root` via `std::filesystem::weakly_canonical()`, rejecting `/../../../../etc/passwd` escapes.
* **MIME Types**: Maps file extensions (`.html`, `.css`, `.js`, `.json`, `.png`, `.jpg`, `.txt`) to standard MIME types.
* **HEAD Support**: Returns `Content-Length` and headers without sending the response body.

### Keep-Alive & Timeouts
* **Persistent Connections**: In HTTP/1.1, connections remain open after a request unless `Connection: close` is specified. After sending a response, the connection resets its parser and remains registered in the event loop for subsequent requests.
* **Idle Timeouts**: The server sweeps active connections and closes sockets whose idle duration exceeds `keep_alive_timeout` (default 60s), preventing idle socket exhaustion.
