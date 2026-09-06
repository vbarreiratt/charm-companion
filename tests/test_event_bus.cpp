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

class CascadingListener : public Listener {
public:
    EventBus& bus;
    EventType trigger_type;
    EventType cascade_type;
    int call_count = 0;

    CascadingListener(EventBus& b, EventType trigger, EventType cascade)
        : bus(b), trigger_type(trigger), cascade_type(cascade) {}

    void on_event(const Event& e) override {
        call_count++;
        if (e.type == trigger_type) {
            Event cascaded{};
            cascaded.type = cascade_type;
            bus.publish(cascaded);
        }
    }
};

TEST(EventBusTest, CascadedPublishDuringCallback) {
    EventBus bus;
    CascadingListener l1(bus, EventType::TOUCH_EVENT, EventType::MOOD_CHANGED);
    MockListener l2;

    bus.subscribe(EventType::TOUCH_EVENT, &l1);
    bus.subscribe(EventType::MOOD_CHANGED, &l2);

    Event initial{};
    initial.type = EventType::TOUCH_EVENT;
    bus.publish(initial);

    EXPECT_EQ(l1.call_count, 1);
    EXPECT_EQ(l2.call_count, 1);
    EXPECT_EQ(l2.last_event.type, EventType::MOOD_CHANGED);
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
