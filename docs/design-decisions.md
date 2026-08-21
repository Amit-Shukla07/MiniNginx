# Architecture & Design Decisions

### 1. Why kqueue (macOS/BSD) and epoll (Linux)?
Traditional POSIX `select()` and `poll()` have O(N) complexity where N is the highest file descriptor value. At thousands of concurrent connections, `select()` wastes substantial CPU scanning fd sets. `kqueue` and `epoll` provide O(1) kernel-level event registration and retrieval, scaling seamlessly to 10,000+ connections.

### 2. Why Non-Blocking Sockets (`O_NONBLOCK`)?
If a socket is blocking, a slow client writing 1 byte or a delayed TCP ACK blocks the entire thread. By placing all listening and client sockets in non-blocking mode, the thread never blocks on syscalls and handles `EAGAIN` / `EWOULDBLOCK` by returning immediately to the event loop.

### 3. Why Modern C++20?
C++20 provides zero-overhead abstractions:
* `std::string_view` enables zero-copy substring parsing without heap allocation.
* RAII guarantees deterministic file descriptor and memory cleanup without garbage collection pauses.
* Move semantics (`std::move`) enable efficient transfer of socket ownership.
* `std::atomic` ensures lock-free metrics on multicore systems.

### 4. Why an Incremental Finite-State Machine Parser?
TCP is a byte-stream protocol. An HTTP request can arrive split across arbitrary packet boundaries. A deterministic FSM maintains parser state across read intervals without copying data back and forth, parsing full requests as soon as sufficient bytes arrive.

### 5. Why a Priority-Queue Min-Heap for Timers?
Min-heaps offer O(log N) timer insertion and O(1) inspection of the earliest deadline. The minimum remaining duration is passed directly as the timeout to `kevent()` / `epoll_wait()`, guaranteeing that the event loop sleeps in the kernel when idle and wakes up precisely when the next timer expires.

### 6. Why Zero-Copy `sendfile()`?
Standard file transfers read data into a user-space buffer before writing it to a socket (`read()` -> user buffer -> `write()`), incurring 2 context switches and 2 buffer copies. `sendfile()` streams bytes directly between the file page cache and the network interface inside the kernel, saving CPU cycles and memory bus bandwidth.
