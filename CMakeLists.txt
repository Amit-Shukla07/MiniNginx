cmake_minimum_required(VERSION 3.20)
project(MiniNginx VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd/clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(MININGINX_BUILD_TESTS "Build test suite" ON)
option(MININGINX_ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(MININGINX_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wnull-dereference
        -Wold-style-cast
        -Wnon-virtual-dtor
        -Woverloaded-virtual
    )
endif()

# Sanitizers
if(MININGINX_ENABLE_ASAN)
    message(STATUS "AddressSanitizer and UBsanitizer enabled")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

if(MININGINX_ENABLE_TSAN)
    message(STATUS "ThreadSanitizer enabled")
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()

# Include directories
include_directories(include)

# Platform detection and source selection
set(PLATFORM_SOURCES "")

if(APPLE)
    message(STATUS "Configuring MiniNginx for macOS (kqueue / Mac sendfile)")
    add_compile_definitions(MININGINX_PLATFORM_MACOS=1)
    list(APPEND PLATFORM_SOURCES
        src/platform/KqueueEventLoop.cpp
        src/platform/MacFileSender.cpp
    )
elseif(UNIX)
    message(STATUS "Configuring MiniNginx for Linux (epoll / Linux sendfile)")
    add_compile_definitions(MININGINX_PLATFORM_LINUX=1)
    list(APPEND PLATFORM_SOURCES
        src/platform/EpollEventLoop.cpp
        src/platform/LinuxFileSender.cpp
    )
else()
    message(FATAL_ERROR "Unsupported operating system platform for MiniNginx")
endif()

# Core sources
set(CORE_SOURCES
    src/core/Socket.cpp
    src/core/Buffer.cpp
    src/core/TimerManager.cpp
    src/core/Connection.cpp
    src/http/HttpParser.cpp
    src/http/HttpRequest.cpp
    src/http/HttpResponse.cpp
    src/http/HttpHeaders.cpp
    src/http/MimeTypes.cpp
    src/server/Router.cpp
    src/server/StaticFileHandler.cpp
    src/server/Worker.cpp
    src/server/HttpServer.cpp
    src/config/Config.cpp
    src/logging/Logger.cpp
    src/metrics/Metrics.cpp
    ${PLATFORM_SOURCES}
)

add_library(mininginx_core STATIC ${CORE_SOURCES})
target_include_directories(mininginx_core PUBLIC include)

# Main executable
add_executable(mininginx src/main.cpp)
target_link_libraries(mininginx PRIVATE mininginx_core)

# Testing
if(MININGINX_BUILD_TESTS)
    enable_testing()

    add_executable(mininginx_unit_tests
        tests/unit/test_main.cpp
        tests/unit/test_buffer.cpp
        tests/unit/test_http_parser.cpp
        tests/unit/test_router.cpp
        tests/unit/test_config.cpp
        tests/unit/test_timer.cpp
        tests/unit/test_mime.cpp
        tests/unit/test_socket.cpp
    )
    target_link_libraries(mininginx_unit_tests PRIVATE mininginx_core)

    add_test(NAME UnitTests COMMAND mininginx_unit_tests)

    # Fuzz harness executable
    add_executable(mininginx_fuzz_parser
        tests/fuzz/fuzz_http_parser.cpp
    )
    target_link_libraries(mininginx_fuzz_parser PRIVATE mininginx_core)
endif()
