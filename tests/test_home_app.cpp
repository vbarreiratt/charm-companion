#include <gtest/gtest.h>
#include "apps/home/home_app.h"
#include "shell/event_bus.h"
#include "shell/event_types.h"
#include "utils/canvas_wrapper.h"

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
    Canvas canvas(466, 466);
    for (int i = 0; i < 5; ++i) {
        app.update(16);
        app.render(&canvas);
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

TEST(HomeAppTest, ReceivesTouchEventPublishedOnGlobalBus) {
    // Regression: HomeApp previously never subscribed to TOUCH_EVENT, so
    // Shell::poll_sensors()'s g_event_bus.publish() never reached on_touch()
    // in production, regardless of real touch hardware.
    HomeApp app;

    Event event;
    event.type = EventType::TOUCH_EVENT;
    event.data.touch.x = 100;
    event.data.touch.y = 200;
    event.data.touch.duration_ms = 50;
    event.data.touch.intensity = 128;
    g_event_bus.publish(event);

    EXPECT_EQ(app.get_touch_count(), 1);
}

TEST(HomeAppTest, ReceivesMotionEventPublishedOnGlobalBus) {
    // Same regression as above, for MOTION_EVENT.
    HomeApp app;

    Event event;
    event.type = EventType::MOTION_EVENT;
    event.data.motion.intensity = 0.75f;
    event.data.motion.accel_x = 0.0f;
    event.data.motion.accel_y = 0.0f;
    event.data.motion.accel_z = 1.0f;
    event.data.motion.gyro_x = 0.0f;
    event.data.motion.gyro_y = 0.0f;
    event.data.motion.gyro_z = 0.0f;
    event.data.motion.direction = 0;
    g_event_bus.publish(event);

    EXPECT_FLOAT_EQ(app.get_motion_intensity(), 0.75f);
}

TEST(HomeAppTest, TouchAndMotionSubscriptionsCleanedUpOnDestroy) {
    {
        HomeApp app;
    }

    Event touch_event;
    touch_event.type = EventType::TOUCH_EVENT;
    touch_event.data.touch = {1, 2, 3, 4};
    g_event_bus.publish(touch_event);

    Event motion_event;
    motion_event.type = EventType::MOTION_EVENT;
    motion_event.data.motion = {0, 0, 0, 0, 0, 0, 0, 0};
    g_event_bus.publish(motion_event);
}
