# MiniNginx Technical Interview Preparation Guide

This guide covers 25 core systems, networking, C++, and operating systems interview questions based directly on the MiniNginx implementation.

---

### Networking & OS Fundamentals

#### Q1: What is the difference between blocking and non-blocking I/O?
**Answer**: With blocking I/O, a syscall (`read()`, `write()`, `accept()`) halts thread execution until data is available or buffer space frees up. In non-blocking I/O (`O_NONBLOCK`), if an operation cannot complete immediately, the syscall returns `-1` immediately with `errno` set to `EAGAIN` or `EWOULDBLOCK`, allowing the thread to perform other work or yield to an event loop.

#### Q2: How does `select()` differ from `kqueue`/`epoll` in scalability?
**Answer**: `select()` passes an array/bitmask of file descriptors from user space to kernel space on every call, requiring an O(N) scan across all descriptors. `kqueue` and `epoll` are stateful in-kernel event registries: registrations persist across polls and returning events only include the descriptors that actually fired (O(1) complexity relative to total registered descriptors).

#### Q3: What is the difference between Level-Triggered (LT) and Edge-Triggered (ET) multiplexing?
**Answer**: Level-Triggered notifies the application continuously as long as a condition remains true (e.g., bytes in receive buffer). Edge-Triggered notifies the application only when a state change occurs (e.g., new bytes arrive). In ET mode, the application MUST drain the socket in a loop until `EAGAIN`; otherwise, pending bytes may remain stuck indefinitely.

#### Q4: Why must an event loop handle `EINTR`?
**Answer**: `EINTR` indicates that a blocking or polling syscall was interrupted by an asynchronous POSIX signal handler before any I/O occurred. The event loop must catch `EINTR` and immediately retry the polling syscall rather than treating it as a fatal failure.

#### Q5: What is the purpose of `SO_REUSEADDR` and `SO_REUSEPORT`?
**Answer**: `SO_REUSEADDR` allows a server to bind to a port that currently has sockets in the `TIME_WAIT` TCP state, preventing "Address already in use" errors during quick server restarts. `SO_REUSEPORT` enables multiple worker sockets/threads to bind to the exact same port, with the kernel load-balancing incoming SYN packets across them.

#### Q6: Why disable Nagle's algorithm (`TCP_NODELAY`) on HTTP servers?
**Answer**: Nagle's algorithm buffers small outgoing packets until a full TCP segment (MSS) is accumulated or an ACK is received for previously sent packets. In HTTP request-response workflows (especially keep-alive and chunked data), this causes artificial 40ms–200ms delayed-ACK latencies. Disabling Nagle via `TCP_NODELAY` forces immediate transmission.

#### Q7: What is `SIGPIPE` and why must a web server ignore it?
**Answer**: If a client abruptly closes its TCP connection (e.g. browser tab closed or network disconnected) and the server attempts to write to the socket, the kernel raises the `SIGPIPE` signal. The default action for `SIGPIPE` is terminating the process. Servers must call `signal(SIGPIPE, SIG_IGN)` to prevent crashes and handle write failures via `errno == EPIPE`.

---

### HTTP Protocol & Parsing

#### Q8: Why can't you parse HTTP requests with a single `recv()` call?
**Answer**: TCP is an unsegmented byte-stream protocol, not a message protocol. Packet boundaries are arbitrary: a single HTTP request can arrive fragmented across multiple IP packets, or multiple HTTP requests can arrive clumped together in a single packet (pipelining). An incremental parser with a finite-state machine (FSM) is required to accumulate and parse tokens across read boundaries.

#### Q9: How does HTTP/1.1 Keep-Alive work and how does MiniNginx implement it?
**Answer**: Keep-Alive maintains the underlying TCP connection open for subsequent requests, eliminating the 3-way handshake overhead for every resource. MiniNginx keeps the socket in `EventType::Read` after sending a response, resets the HTTP parser, resets the keep-alive timeout in `TimerManager`, and immediately processes any pipelined bytes left in `read_buf_`.

#### Q10: How does Chunked Transfer Encoding work?
**Answer**: When `Transfer-Encoding: chunked` is present, the sender transmits data in chunks without knowing total `Content-Length` in advance. Each chunk consists of a hexadecimal size line followed by `\r\n`, the raw chunk payload, and another `\r\n`. A chunk of size `0\r\n\r\n` denotes end-of-stream.

#### Q11: What is the difference between `GET` and `HEAD` methods?
**Answer**: `HEAD` is identical to `GET` except the server MUST NOT send a message body in the response. It is used to inspect headers (e.g., `Content-Length`, `Last-Modified`, `Content-Type`) without downloading the payload.

---

### Zero-Copy & Memory Management

#### Q12: How does `sendfile()` achieve zero-copy?
**Answer**: Standard file transmission uses `read(file_fd, user_buf)` and `write(sock_fd, user_buf)`, causing 2 context switches and copying data from kernel page cache -> user buffer -> kernel socket buffer -> NIC. `sendfile()` instructs the kernel to transfer pages directly from the filesystem page cache to the network device buffer, avoiding all user-space copies and context switches.

#### Q13: What are the differences between macOS `sendfile()` and Linux `sendfile()`?
**Answer**:
* **macOS**: `int sendfile(int fd, int s, off_t offset, off_t *len, struct sf_hdtr *hdtr, int flags)`. Takes `fd` (file) first, `s` (socket) second, and updates `*len` in-place with bytes sent.
* **Linux**: `ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count)`. Takes `out_fd` (socket) first, `in_fd` (file) second, and returns bytes written as `ssize_t`.

#### Q14: How does dynamic buffer compaction work in MiniNginx?
**Answer**: `Buffer` maintains `reader_idx_` and `writer_idx_`. When data is consumed, `reader_idx_` advances. When more space is needed for append, if the unread bytes can fit in the reclaimed space at the beginning of the vector, `std::copy` moves readable bytes to index 0 (compaction) rather than reallocating memory.

---

### Concurrency & Architecture

#### Q15: What is the Reactor Pattern?
**Answer**: The Reactor architectural pattern demultiplexes concurrent service requests delivered to an application by one or more inputs. The service handler uses an event loop to wait synchronously on incoming events and dispatches each event asynchronously to its associated request handler.

#### Q16: How does MiniNginx scale to multiple CPU cores?
**Answer**: MiniNginx utilizes a Master-Worker architecture where the main thread accepts incoming TCP connections and dispatches them round-robin to a pool of N Worker threads, each running an independent event loop and timer manager, eliminating shared mutable state on hot request paths.

#### Q17: Why use a min-heap for connection timers instead of checking all connections?
**Answer**: Iterating over 10,000 connections every tick to check for timeouts is an O(N) operation that burns CPU. A min-heap maintains the next earliest deadline at the top in O(1). The server sets the event loop poll timeout to this exact duration, allowing the kernel to sleep when idle and waking up only when a timer expires or I/O is ready.

#### Q18: What is a Slowloris attack and how does MiniNginx mitigate it?
**Answer**: A Slowloris attack opens thousands of HTTP connections and sends partial header lines at slow intervals (e.g. 1 byte every 5s) to exhaust server connection limits. MiniNginx assigns a strict request timer (e.g. 10s) upon connection; if a full HTTP request header isn't parsed before deadline expiration, the timer triggers and closes the connection.

#### Q19: How does MiniNginx prevent path traversal attacks like `/../../../../etc/passwd`?
**Answer**: It resolves both the document root and the requested path into canonical, absolute paths via `std::filesystem::weakly_canonical()` and verifies that the canonical path begins with the exact prefix string of the canonical document root.

#### Q20: What are the benefits of C++20 `std::string_view` during HTTP parsing?
**Answer**: `std::string_view` provides a non-owning reference to a contiguous character sequence (pointer + length). Parsing header keys, values, URIs, and query parameters creates string views into the existing `Buffer` memory without allocating heap strings on each token.

---

### Interview Quick Fire

#### Q21: What is the purpose of `SO_RCVBUF` / `SO_SNDBUF`?
**Answer**: Configures kernel TCP socket send/receive window sizes in bytes.

#### Q22: What does `shutdown(fd, SHUT_RDWR)` do versus `close(fd)`?
**Answer**: `shutdown()` sends a TCP FIN packet immediately and terminates the connection flow regardless of other references to the file descriptor; `close()` decrements the file descriptor reference count and only closes the underlying socket when the reference count reaches 0.

#### Q23: Why is `std::enable_shared_from_this` used for Connection?
**Answer**: It allows the connection object to safely generate `shared_ptr<Connection>` instances inside its own member functions to pass into event loop callbacks, guaranteeing the connection remains alive during async dispatch.

#### Q24: What is the difference between HTTP status 400, 413, 414, and 431?
**Answer**:
* `400`: Bad Request (malformed syntax)
* `413`: Payload Too Large (request body exceeds limit)
* `414`: URI Too Long (URI length exceeds limit)
* `431`: Request Header Fields Too Large (headers block exceeds limit)

#### Q25: How does graceful shutdown work in MiniNginx?
**Answer**: Upon receiving `SIGINT` or `SIGTERM`, the server sets an atomic running flag to false, closes the listener socket to stop accepting new clients, flushes remaining write buffers on active connections, cleanly closes worker event loops, joins worker threads, and releases all resources without memory or descriptor leaks.
