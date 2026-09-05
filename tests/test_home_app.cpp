#include <gtest/gtest.h>
#include "apps/home/home_app.h"
#include "shell/event_bus.h"
#include "shell/event_types.h"

// Forward-declared Canvas pointer is passed as nullptr in tests
class Canvas;

TEST(HomeAppTest, OnEnterDoesNotCrash) {
    HomeApp app;
    app.on_enter();
    EXPECT_EQ(app.get_frame_counter(), 0u);
    app.on_exit();
}

TEST(HomeAppTest, UpdateIncrementsFrameCounter) {
    HomeApp app;
    app.on_enter();
    EXPECT_EQ(app.get_frame_counter(), 0u);

    for (int i = 0; i < 5; ++i) {
        app.update(16);
    }
    EXPECT_EQ(app.get_frame_counter(), 5u);
}

TEST(HomeAppTest, OnMotionDecaysIntensity) {
    HomeApp app;
    app.on_enter();

    Event event;
    event.type = EventType::MOTION_EVENT;
    event.data.motion.intensity = 1.0f;
    event.data.motion.accel_x = 0.0f;
    event.data.motion.accel_y = 0.0f;
    event.data.motion.accel_z = 1.0f;
    event.data.motion.gyro_x = 0.0f;
    event.data.motion.gyro_y = 0.0f;
    event.data.motion.gyro_z = 0.0f;
    event.data.motion.direction = 0;

    app.on_event(event);
    EXPECT_FLOAT_EQ(app.get_motion_intensity(), 1.0f);

    // Call update(16) multiple times and verify decay
    for (int i = 0; i < 50; ++i) {
        app.update(16);
    }

    // 1.0 * (0.95^50) ~= 0.0769
    EXPECT_LT(app.get_motion_intensity(), 0.1f);
    EXPECT_GE(app.get_motion_intensity(), 0.0f);
}

TEST(HomeAppTest, RenderDoesNotCrash) {
    HomeApp app;
    app.on_enter();
    for (int i = 0; i < 5; ++i) {
        app.update(16);
        app.render(nullptr);
    }
}

TEST(HomeAppTest, OnTouchDoesNotCrash) {
    HomeApp app;
    app.on_enter();

    Event event;
    event.type = EventType::TOUCH_EVENT;
    event.data.touch.x = 100;
    event.data.touch.y = 200;
    event.data.touch.duration_ms = 50;
    event.data.touch.intensity = 128;

    app.on_event(event);
}

TEST(HomeAppTest, SubscribesAndUnsubscribesFromEventBus) {
    // When HomeApp is created, it subscribes to MOOD_CHANGED.
    // When destroyed, it unsubscribes cleanly so publishing does not crash.
    {
        HomeApp app;
    }

    // Publish MOOD_CHANGED after app is destroyed
    Event mood_event;
    mood_event.type = EventType::MOOD_CHANGED;
    mood_event.data.mood.personality_context = nullptr;
    g_event_bus.publish(mood_event);
}
