cmake_minimum_required(VERSION 3.20)
project(MiniNginx VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -O2)

include_directories(include platform)

set(PLATFORM_SOURCES "")
if(APPLE)
    list(APPEND PLATFORM_SOURCES platform/KqueueEventLoop.cpp)
elseif(UNIX)
    list(APPEND PLATFORM_SOURCES platform/EpollEventLoop.cpp)
endif()

set(CORE_SOURCES
    src/EventLoop.cpp
    src/Socket.cpp
    src/Connection.cpp
    src/Buffer.cpp
    src/HttpParser.cpp
    src/HttpResponse.cpp
    src/HttpServer.cpp
    src/Router.cpp
    ${PLATFORM_SOURCES}
)

add_library(mininginx_core STATIC ${CORE_SOURCES})

# Server executable
add_executable(mininginx src/main.cpp)
target_link_libraries(mininginx PRIVATE mininginx_core)

# Tests
enable_testing()

add_executable(BufferTest tests/BufferTest.cpp)
target_link_libraries(BufferTest PRIVATE mininginx_core)
add_test(NAME BufferTest COMMAND BufferTest)

add_executable(HttpParserTest tests/HttpParserTest.cpp)
target_link_libraries(HttpParserTest PRIVATE mininginx_core)
add_test(NAME HttpParserTest COMMAND HttpParserTest)

add_executable(ServerTest tests/ServerTest.cpp)
target_link_libraries(ServerTest PRIVATE mininginx_core)
add_test(NAME ServerTest COMMAND ServerTest)
