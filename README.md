# MiniNginx

A lightweight, event-driven HTTP/1.1 web server written from scratch in C++20.

MiniNginx is designed to explore and demonstrate the core systems engineering principles behind production web servers like Nginx: non-blocking sockets, OS-level I/O multiplexing (`kqueue` on macOS, `epoll` on Linux), incremental HTTP parsing, and persistent connections on a single-threaded event loop.

---

## Architecture

```
Client
  ↓
TCP Socket (Non-blocking)
  ↓
kqueue (macOS) / epoll (Linux)
  ↓
Event Loop (Single-threaded)
  ↓
Connection State Machine (Buffer + HttpParser)
  ↓
Router (Path Sanitization & Static File Server)
  ↓
HTTP Response
```

---

## Features

- **I/O Multiplexing**: Native `kqueue` on macOS/BSD and `epoll` on Linux.
- **Non-Blocking Sockets**: `O_NONBLOCK` socket handling with `EAGAIN`/`EWOULDBLOCK` and `EINTR` recovery.
- **Incremental HTTP Parser**: Deterministic finite-state machine (FSM) supporting `GET`, `HEAD`, `POST`, Content-Length bodies, and fragmented TCP streams.
- **HTTP/1.1 Keep-Alive**: Persistent connection reuse across multiple requests.
- **Static File Serving**: Content-Length calculation, MIME type detection, and directory index resolution (`index.html`).
- **Path Traversal Protection**: Filesystem canonicalization preventing `../../etc/passwd` directory traversal exploits.
- **Connection Timeouts**: Automatic sweeping of idle connections to prevent socket exhaustion.

---

## Project Structure

```
MiniNginx/
├── CMakeLists.txt              # CMake build configuration
├── Makefile                    # Make convenience targets
├── README.md                   # Project overview
├── config/
│   └── server.conf             # Server configuration
├── include/
│   ├── Buffer.hpp              # Dynamic byte buffer
│   ├── Connection.hpp          # Client connection state machine
│   ├── EventLoop.hpp           # Event loop interface
│   ├── HttpParser.hpp          # Incremental HTTP/1.1 parser
│   ├── HttpRequest.hpp         # Request representation
│   ├── HttpResponse.hpp        # Response serializer
│   ├── HttpServer.hpp          # Server accept loop & lifecycle
│   ├── Router.hpp              # Static routing & MIME lookup
│   └── Socket.hpp              # RAII socket wrapper
├── src/
│   ├── Buffer.cpp
│   ├── Connection.cpp
│   ├── EventLoop.cpp
│   ├── HttpParser.cpp
│   ├── HttpResponse.cpp
│   ├── HttpServer.cpp
│   ├── Router.cpp
│   ├── Socket.cpp
│   └── main.cpp                # Server entry point
├── platform/
│   ├── KqueueEventLoop.hpp     # macOS kqueue implementation
│   ├── KqueueEventLoop.cpp
│   ├── EpollEventLoop.hpp      # Linux epoll implementation
│   └── EpollEventLoop.cpp
├── tests/
│   ├── BufferTest.cpp          # Buffer unit tests
│   ├── HttpParserTest.cpp      # Parser unit tests
│   └── ServerTest.cpp          # Router & static file tests
├── public/
│   ├── index.html              # Demo static page
│   ├── style.css               # Demo styling
│   └── test.html               # Secondary test page
└── docs/
    └── architecture.md         # Internal architectural documentation
```

---

## Build & Run

### Prerequisites
* C++20 compliant compiler (`clang++` or `g++`)
* CMake 3.20+

### Build
```bash
make build
# or: cmake -B build && cmake --build build
```

### Run
```bash
make run
# or: ./build/mininginx [config_file]
```

By default, MiniNginx listens on `0.0.0.0:8080` and serves files from `./public`.

### Test with curl
```bash
# Fetch main page
curl -i http://localhost:8080/

# Fetch secondary page
curl -i http://localhost:8080/test.html

# Test HEAD request
curl -I http://localhost:8080/

# Test 404 handler
curl -i http://localhost:8080/nonexistent.html
```

---

## Running Tests

```bash
make test
# or: ctest --test-dir build --output-on-failure
```

---

## Configuration (`config/server.conf`)

```nginx
host 0.0.0.0
port 8080
root ./public
keep_alive_timeout 60
```

---

## Limitations

- **No TLS/HTTPS**: Focuses strictly on HTTP/1.1 networking mechanics and OS multiplexing.
- **No HTTP/2 or HTTP/3**: Plaintext HTTP/1.1 only.
- **Single Process / Single Thread**: Designed to clearly illustrate event-driven architecture without threading complexity.
