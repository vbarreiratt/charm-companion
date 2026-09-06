#include <gtest/gtest.h>
#include "shell/shell.h"
#include "shell/event_bus.h"
#include "shell/event_types.h"
#include "spicy/spicy.h"
#include "spicy/personality_nvs.h"
#include "apps/app_base.h"
#include "config/pin_config.h"

class MockTestApp : public App {
public:
    int enter_count = 0;
    int exit_count = 0;
    int update_count = 0;
    uint32_t last_dt = 0;
    int touch_count = 0;
    int motion_count = 0;
    int render_count = 0;
    Canvas* last_canvas = nullptr;
    int handle_back_calls = 0;
    bool handle_back_return = false;

    void on_enter() override { enter_count++; }
    void on_exit() override { exit_count++; }
    void on_touch(const TouchEvent& e) override { (void)e; touch_count++; }
    void on_motion(const MotionEvent& e) override { (void)e; motion_count++; }
    void update(uint32_t dt) override { update_count++; last_dt = dt; }
    void render(Canvas* canvas) override { last_canvas = canvas; render_count++; }
    bool handle_back() override { handle_back_calls++; return handle_back_return; }
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
    // Asserts the boot-default mood, which is a precondition on the native
    // NVS store being empty. Reset explicitly rather than relying on gtest
    // registration order across the flat native test binary (other test
    // files' calls to set_mood() also write to this shared global store).
    g_personality_nvs.reset_for_testing();

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

TEST(ShellIntegrationTest, HandleBootPressCallsAppHandleBackFirst) {
    Shell& sh = Shell::instance();
    MockTestApp app;
    app.handle_back_return = true;

    sh.switch_app(&app, "custom");
    sh.tick(16);

    sh.handle_boot_press();
    EXPECT_EQ(app.handle_back_calls, 1);
    EXPECT_EQ(sh.get_current_app(), &app);  // stayed: handle_back said it handled it

    sh.switch_app(nullptr);
    sh.tick(0);
}

TEST(ShellIntegrationTest, HandleBootPressSwitchesToHomeWhenAppCannotGoBack) {
    Shell& sh = Shell::instance();
    MockTestApp app;
    app.handle_back_return = false;

    sh.switch_app(&app, "not_home");
    sh.tick(16);

    sh.handle_boot_press();
    sh.tick(16);  // apply the pending "home" transition
    EXPECT_STREQ(sh.get_current_app_name(), "home");

    sh.switch_app(nullptr);
    sh.tick(0);
}

TEST(ShellIntegrationTest, HandleBootPressIsNoOpWhenAlreadyHome) {
    Shell& sh = Shell::instance();
    sh.switch_app("home");
    sh.tick(16);
    ASSERT_STREQ(sh.get_current_app_name(), "home");

    sh.handle_boot_press();
    sh.tick(16);
    EXPECT_STREQ(sh.get_current_app_name(), "home");
}

TEST(ShellIntegrationTest, SwipeUpOnHomeSwitchesToScenes) {
    Shell& sh = Shell::instance();
    sh.switch_app("home");
    sh.tick(16);
    ASSERT_STREQ(sh.get_current_app_name(), "home");

    sh.debug_inject_touch(233, 400, true);
    sh.debug_inject_touch(233, 300, true);
    sh.debug_inject_touch(0, 0, false);  // release -> completes an UP swipe

    sh.tick(16);
    EXPECT_STREQ(sh.get_current_app_name(), "scenes");

    sh.switch_app(nullptr);
    sh.tick(0);
}

TEST(ShellIntegrationTest, SwipeUpIsIgnoredWhenNotOnHome) {
    Shell& sh = Shell::instance();
    MockTestApp app;

    sh.switch_app(&app, "not_home");
    sh.tick(16);

    sh.debug_inject_touch(233, 400, true);
    sh.debug_inject_touch(233, 300, true);
    sh.debug_inject_touch(0, 0, false);

    sh.tick(16);
    EXPECT_EQ(sh.get_current_app(), &app);  // unchanged

    sh.switch_app(nullptr);
    sh.tick(0);
}

TEST(ShellIntegrationTest, RenderPassesRealNonNullCanvas) {
    Shell& sh = Shell::instance();
    MockTestApp app;

    sh.switch_app(&app);
    sh.tick(16);

    ASSERT_NE(app.last_canvas, nullptr);
    EXPECT_EQ(app.last_canvas->width(), DISPLAY_WIDTH);
    EXPECT_EQ(app.last_canvas->height(), DISPLAY_HEIGHT);

    sh.switch_app(nullptr);
    sh.tick(0);
}
