#include <gtest/gtest.h>
#include "shell/shell.h"
#include "shell/event_bus.h"
#include "shell/event_types.h"
#include "spicy/spicy.h"
#include "apps/app_base.h"

class MockTestApp : public App {
public:
    int enter_count = 0;
    int exit_count = 0;
    int update_count = 0;
    uint32_t last_dt = 0;
    int touch_count = 0;
    int motion_count = 0;
    int render_count = 0;

    void on_enter() override { enter_count++; }
    void on_exit() override { exit_count++; }
    void on_touch(const TouchEvent& e) override { (void)e; touch_count++; }
    void on_motion(const MotionEvent& e) override { (void)e; motion_count++; }
    void update(uint32_t dt) override { update_count++; last_dt = dt; }
    void render(Canvas* canvas) override { (void)canvas; render_count++; }
};

class CountingMotionListener : public Listener {
public:
    int received = 0;
    float last_intensity = 0.0f;
    void on_event(const Event& e) override {
        if (e.type == EventType::MOTION_EVENT) {
            received++;
            last_intensity = e.data.motion.intensity;
        }
    }
};

TEST(ShellIntegrationTest, ShellInitializes) {
    Shell& sh = Shell::instance();
    EXPECT_TRUE(sh.init());
    EXPECT_EQ(g_spicy.get_context().mood, Mood::CURIOUS);
}

TEST(ShellIntegrationTest, EventBusPublishesMotionEvent) {
    CountingMotionListener listener;
    g_event_bus.subscribe(EventType::MOTION_EVENT, &listener);

    Shell::instance().poll_sensors();
    EXPECT_GE(listener.received, 1);

    g_event_bus.unsubscribe(EventType::MOTION_EVENT, &listener);
}

TEST(ShellIntegrationTest, AppTransitionFlow) {
    Shell& sh = Shell::instance();
    MockTestApp app1;
    MockTestApp app2;

    sh.switch_app(&app1);
    EXPECT_EQ(app1.enter_count, 0);

    sh.tick(16);
    EXPECT_EQ(sh.get_current_app(), &app1);
    EXPECT_EQ(app1.enter_count, 1);
    EXPECT_EQ(app1.exit_count, 0);

    sh.switch_app(&app2);
    EXPECT_EQ(app2.enter_count, 0);

    sh.tick(16);
    EXPECT_EQ(sh.get_current_app(), &app2);
    EXPECT_EQ(app1.exit_count, 1);
    EXPECT_EQ(app2.enter_count, 1);

    // Clean up current app
    sh.switch_app(nullptr);
    sh.tick(0);
    EXPECT_EQ(sh.get_current_app(), nullptr);
    EXPECT_EQ(app2.exit_count, 1);
}

TEST(ShellIntegrationTest, TickUpdatesActiveApp) {
    Shell& sh = Shell::instance();
    MockTestApp app;

    sh.switch_app(&app);
    sh.tick(16);

    int initial_updates = app.update_count;
    EXPECT_GE(initial_updates, 1);
    EXPECT_EQ(app.last_dt, 16u);

    sh.tick(33);
    EXPECT_EQ(app.update_count, initial_updates + 1);
    EXPECT_EQ(app.last_dt, 33u);

    // Clean up
    sh.switch_app(nullptr);
    sh.tick(0);
    EXPECT_EQ(sh.get_current_app(), nullptr);
}
