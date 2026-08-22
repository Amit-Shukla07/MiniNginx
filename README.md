# MiniNginx

A lightweight, cross-platform, event-driven HTTP/1.1 web server written from scratch in C++20.

MiniNginx demonstrates the core systems engineering principles behind production web servers: non-blocking sockets, OS-level I/O multiplexing (**kqueue** on macOS, **epoll** on Linux, **WSAPoll** on Windows), incremental HTTP parsing, and persistent connections on a single-threaded event loop.

---

## Supported Platforms

| Platform | Multiplexer | Implementation |
| :--- | :--- | :--- |
| **macOS / BSD** | `kqueue` & `kevent` | `platform/KqueueEventLoop.cpp` |
| **Linux** | `epoll` (`epoll_create1`, `epoll_wait`) | `platform/EpollEventLoop.cpp` |
| **Windows** | `Winsock2` & `WSAPoll` | `platform/WindowsEventLoop.cpp` |

---

## Project Structure

```
MiniNginx/
├── include/
│   ├── http/
│   │   ├── HttpParser.hpp
│   │   ├── HttpRequest.hpp
│   │   └── HttpResponse.hpp
│   └── server/
│       ├── Buffer.hpp
│       ├── Connection.hpp
│       ├── EventLoop.hpp
│       ├── HttpServer.hpp
│       ├── Router.hpp
│       └── Socket.hpp
│
├── src/
│   ├── http/
│   │   ├── HttpParser.cpp
│   │   └── HttpResponse.cpp
│   ├── server/
│   │   ├── Buffer.cpp
│   │   ├── Connection.cpp
│   │   ├── EventLoop.cpp
│   │   ├── HttpServer.cpp
│   │   ├── Router.cpp
│   │   └── Socket.cpp
│   └── main.cpp
│
├── platform/
│   ├── KqueueEventLoop.hpp     # macOS kqueue
│   ├── KqueueEventLoop.cpp
│   ├── EpollEventLoop.hpp      # Linux epoll
│   ├── EpollEventLoop.cpp
│   ├── WindowsEventLoop.hpp    # Windows Winsock2 WSAPoll
│   └── WindowsEventLoop.cpp
│
├── public/
│   ├── index.html
│   ├── style.css
│   └── test.html
│
├── config/
│   └── server.conf
├── CMakeLists.txt
├── Makefile
└── README.md
```

---

## Build & Run

### Prerequisites
* C++20 compliant compiler (`clang++`, `g++`, or MSVC on Windows)
* CMake 3.20+

### Linux / macOS
```bash
make build
make run
```

### Windows (PowerShell / Command Prompt)
```cmd
cmake -B build
cmake --build build --config Release
.\build\Release\mininginx.exe
```

By default, MiniNginx listens on `0.0.0.0:8080` and serves files from `./public`.

---

## Testing with curl

```bash
# Fetch index page
curl -i http://localhost:8080/

# Test HEAD request (headers only)
curl -I http://localhost:8080/

# Test 404 handling
curl -i http://localhost:8080/missing.html

# Test Path Traversal Protection
curl -i http://localhost:8080/../../../../etc/passwd
```
