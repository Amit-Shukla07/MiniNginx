cmake_minimum_required(VERSION 3.20)
project(MiniNginx VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -O2)

include_directories(
    include
    include/http
    include/server
    platform
)

set(PLATFORM_SOURCES "")
if(APPLE)
    list(APPEND PLATFORM_SOURCES platform/KqueueEventLoop.cpp)
elseif(UNIX)
    list(APPEND PLATFORM_SOURCES platform/EpollEventLoop.cpp)
endif()

set(CORE_SOURCES
    src/http/HttpParser.cpp
    src/http/HttpResponse.cpp
    src/server/EventLoop.cpp
    src/server/Socket.cpp
    src/server/Buffer.cpp
    src/server/Router.cpp
    src/server/Connection.cpp
    src/server/HttpServer.cpp
    ${PLATFORM_SOURCES}
)

add_library(mininginx_core STATIC ${CORE_SOURCES})

# Server executable
add_executable(mininginx src/main.cpp)
target_link_libraries(mininginx PRIVATE mininginx_core)
