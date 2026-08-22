#include "../../include/server/EventLoop.hpp"

#ifdef __APPLE__
#include "../../platform/KqueueEventLoop.hpp"
#elif defined(__linux__)
#include "../../platform/EpollEventLoop.hpp"
#elif defined(_WIN32)
#include "../../platform/WindowsEventLoop.hpp"
#endif

std::unique_ptr<EventLoop> EventLoop::create() {
#ifdef __APPLE__
    return std::make_unique<KqueueEventLoop>();
#elif defined(__linux__)
    return std::make_unique<EpollEventLoop>();
#elif defined(_WIN32)
    return std::make_unique<WindowsEventLoop>();
#else
    return nullptr;
#endif
}
