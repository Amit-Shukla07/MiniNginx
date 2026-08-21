#include "core/TimerManager.hpp"
#include "test_framework.hpp"

#include <thread>

using namespace mininginx::core;

TEST(TimerTest, BasicTimerExpiration) {
    TimerManager tm;
    bool fired = false;

    tm.add_timer(Milliseconds(10), [&fired]() {
        fired = true;
    });

    ASSERT_EQ(tm.active_timers_count(), 1);
    ASSERT_FALSE(fired);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    tm.handle_expired_timers();

    ASSERT_TRUE(fired);
    ASSERT_EQ(tm.active_timers_count(), 0);
}

TEST(TimerTest, CancelTimer) {
    TimerManager tm;
    bool fired = false;

    TimerId tid = tm.add_timer(Milliseconds(10), [&fired]() {
        fired = true;
    });

    ASSERT_EQ(tm.active_timers_count(), 1);
    tm.cancel_timer(tid);
    ASSERT_EQ(tm.active_timers_count(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    tm.handle_expired_timers();

    ASSERT_FALSE(fired);
}

TEST(TimerTest, MultipleTimersPriorityOrder) {
    TimerManager tm;
    std::vector<int> execution_order;

    tm.add_timer(Milliseconds(50), [&execution_order]() { execution_order.push_back(3); });
    tm.add_timer(Milliseconds(10), [&execution_order]() { execution_order.push_back(1); });
    tm.add_timer(Milliseconds(30), [&execution_order]() { execution_order.push_back(2); });

    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    tm.handle_expired_timers();

    ASSERT_EQ(execution_order.size(), 3);
    ASSERT_EQ(execution_order[0], 1);
    ASSERT_EQ(execution_order[1], 2);
    ASSERT_EQ(execution_order[2], 3);
}
