# Event Loop & I/O Multiplexing Architecture

## 1. Overview

MiniNginx uses an OS-native I/O multiplexing architecture to handle thousands of concurrent client connections on a small, fixed number of worker threads.

### Why not Thread-per-Connection?
* **Memory footprint**: Thread stacks in C++ typically allocate 2MB to 8MB per thread. 10,000 threads would require 20GB–80GB of RAM simply for stack space.
* **Context switching overhead**: Scheduling tens of thousands of operating system threads causes extreme CPU cache thrashing and kernel scheduler latency.
* **Synchronization hazards**: Shared state between thousands of preemptive threads requires heavy locking and mutex contention.

By contrast, MiniNginx connection objects occupy < 8KB of RAM, and single-threaded event loops handle thousands of connections with zero mutex contention during request processing.

---

## 2. Platform Implementations

### 2.1 macOS: `kqueue` & `kevent`
* `kqueue` is the stateful kernel event notification facility in BSD and macOS Darwin.
* Sockets register filter interests using `EVFILT_READ` and `EVFILT_WRITE`.
* `kevent` processes event batches in a single syscall:
```cpp
int nev = ::kevent(kq_fd_, nullptr, 0, event_list_.data(), event_list_.size(), &timespec_timeout);
```
* Sockets register `EV_ADD | EV_ENABLE` and remove on closure using `EV_DELETE`.

### 2.2 Linux: `epoll`
* `epoll` is the Linux equivalent high-performance kernel multiplexer with O(1) performance relative to active descriptors.
* Implemented via `epoll_create1(EPOLL_CLOEXEC)`, `epoll_ctl(EPOLL_CTL_ADD / MOD / DEL)`, and `epoll_wait()`.
* Listens on `EPOLLIN`, `EPOLLOUT`, `EPOLLRDHUP`, and `EPOLLHUP`.

---

## 3. Level-Triggered vs Edge-Triggered

MiniNginx is designed with **Level-Triggered (LT)** as its core mode:
* In Level-Triggered mode, the kernel continues to notify the event loop as long as unread bytes remain in the kernel receive buffer or writable space remains in the send buffer.
* This eliminates the hazard of socket starvation or dropped events in complex edge-triggered drains, while maintaining peak throughput (~17,000+ RPS).
* Edge-triggered flags (`EV_CLEAR` / `EPOLLET`) are configurable through `core::EventType::EdgeTriggered`.
