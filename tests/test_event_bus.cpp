#include <gtest/gtest.h>
#include "shell/event_bus.h"

class MockListener : public Listener {
public:
    int call_count = 0;
    Event last_event{};
    void on_event(const Event& e) override {
        call_count++;
        last_event = e;
    }
};

TEST(EventBusTest, PublishAndSubscribe) {
    EventBus bus;
    MockListener listener;

    // Subscribe to TOUCH_EVENT
    bus.subscribe(EventType::TOUCH_EVENT, &listener);

    // Publish event
    Event e{};
    e.type = EventType::TOUCH_EVENT;
    bus.publish(e);

    // Listener should receive
    ASSERT_EQ(listener.call_count, 1);
}

TEST(EventBusTest, MultipleListeners) {
    EventBus bus;
    MockListener l1, l2;

    bus.subscribe(EventType::TOUCH_EVENT, &l1);
    bus.subscribe(EventType::TOUCH_EVENT, &l2);

    Event e{};
    e.type = EventType::TOUCH_EVENT;
    bus.publish(e);

    ASSERT_EQ(l1.call_count, 1);
    ASSERT_EQ(l2.call_count, 1);
}

TEST(EventBusTest, Unsubscribe) {
    EventBus bus;
    MockListener listener;

    bus.subscribe(EventType::TOUCH_EVENT, &listener);
    bus.unsubscribe(EventType::TOUCH_EVENT, &listener);

    Event e{};
    e.type = EventType::TOUCH_EVENT;
    bus.publish(e);

    ASSERT_EQ(listener.call_count, 0);
}

#if defined(ARDUINO)
void setup() {
    ::testing::InitGoogleTest();
}
void loop() {
    RUN_ALL_TESTS();
}
#else
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
