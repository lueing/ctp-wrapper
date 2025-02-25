#include <thread>
#include "gtest/gtest.h"
#include "events.h"

TEST(EventsTest, WaitOnce)
{
    lueing::Events events;

    std::thread t([&events] {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        events.NotifyOnce("event1");
    });

    events.WaitOnce("event1");
    std::cout << "WaitOnce" << std::endl;
}