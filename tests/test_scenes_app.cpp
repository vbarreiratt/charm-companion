#include <gtest/gtest.h>
#include "apps/scenes/scenes_app.h"
#include "apps/scenes/scene_registry.h"
#include "apps/scenes/scene_base.h"
#include "shell/event_bus.h"
#include "shell/event_types.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"

class MockScene : public Scene {
public:
    int update_count = 0;
    int touch_count = 0;
    int motion_count = 0;
    int render_count = 0;
    bool entered = false;
    bool exited = false;
    std::string scene_name;

    MockScene(const char* n = "mock") : scene_name(n) {}

    const char* name() const override { return scene_name.c_str(); }
    void on_enter() override { entered = true; }
    void on_exit() override { exited = true; }
    void on_touch(const TouchEvent& e) override { touch_count++; }
    void on_motion(const MotionEvent& e) override { motion_count++; }
    void update(uint32_t dt) override { update_count++; }
    void render(Canvas* canvas, const PersonalityContext& ctx) override { render_count++; }
};

TEST(ScenesAppTest, RegisterAndLoadScene) {
    auto& registry = SceneRegistry::instance();
    registry.register_scene("test_scene", []() { return new MockScene("mock"); });

    Scene* scene = registry.load_scene("test_scene");
    ASSERT_NE(scene, nullptr);
    ASSERT_STREQ(scene->name(), "mock");

    delete scene;
}

TEST(ScenesAppTest, SceneTransition) {
    // Register two scenes
    auto& registry = SceneRegistry::instance();
    registry.register_scene("scene1", []() { return new MockScene("scene1"); });
    registry.register_scene("scene2", []() { return new MockScene("scene2"); });

    ScenesApp app;
    app.on_enter();
    ASSERT_NE(app.get_current_scene(), nullptr);

    app.next_scene();
    ASSERT_NE(app.get_current_scene(), nullptr);

    app.on_exit();
    EXPECT_EQ(app.get_current_scene(), nullptr);
}

TEST(ScenesAppTest, CountAndSceneAt) {
    auto& registry = SceneRegistry::instance();
    registry.clear();
    registry.register_scene("count_scene_a", []() { return new MockScene("count_scene_a"); });
    registry.register_scene("count_scene_b", []() { return new MockScene("count_scene_b"); });

    EXPECT_EQ(registry.count(), 2);
    EXPECT_NE(registry.scene_at(0), nullptr);
    EXPECT_STREQ(registry.scene_at(0), "count_scene_a");
    EXPECT_STREQ(registry.scene_at(1), "count_scene_b");
    EXPECT_EQ(registry.scene_at(2), nullptr);
    EXPECT_EQ(registry.scene_at(-1), nullptr);
}

TEST(ScenesAppTest, PrevSceneAndWrapAround) {
    auto& registry = SceneRegistry::instance();
    registry.clear();
    registry.register_scene("scene_0", []() { return new MockScene("scene_0"); });
    registry.register_scene("scene_1", []() { return new MockScene("scene_1"); });
    registry.register_scene("scene_2", []() { return new MockScene("scene_2"); });

    ScenesApp app;
    app.on_enter();
    EXPECT_EQ(app.get_current_scene_index(), 0);
    EXPECT_STREQ(app.get_current_scene()->name(), "scene_0");

    // prev wraps to last
    app.prev_scene();
    EXPECT_EQ(app.get_current_scene_index(), 2);
    EXPECT_STREQ(app.get_current_scene()->name(), "scene_2");

    // next wraps back to 0
    app.next_scene();
    EXPECT_EQ(app.get_current_scene_index(), 0);
    EXPECT_STREQ(app.get_current_scene()->name(), "scene_0");

    app.on_exit();
}

TEST(ScenesAppTest, UpdateAndRenderDelegation) {
    auto& registry = SceneRegistry::instance();
    registry.clear();
    registry.register_scene("mock_active", []() { return new MockScene("mock_active"); });

    ScenesApp app;
    app.on_enter();
    auto* mock = dynamic_cast<MockScene*>(app.get_current_scene());
    ASSERT_NE(mock, nullptr);
    EXPECT_TRUE(mock->entered);

    app.update(16);
    EXPECT_EQ(mock->update_count, 1);

    Canvas canvas(466, 466);
    app.render(&canvas);
    EXPECT_EQ(mock->render_count, 1);

    TouchEvent te{10, 20, 100, 255};
    app.on_touch(te);
    EXPECT_EQ(mock->touch_count, 1);

    MotionEvent me{0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0};
    app.on_motion(me);
    EXPECT_EQ(mock->motion_count, 1);

    app.on_exit();
}

TEST(ScenesAppTest, EmptyRegistryDoesNotCrash) {
    auto& registry = SceneRegistry::instance();
    registry.clear();

    ScenesApp app;
    app.on_enter();
    EXPECT_EQ(app.get_current_scene(), nullptr);

    app.next_scene();
    app.prev_scene();
    app.update(16);
    Canvas canvas(466, 466);
    app.render(&canvas);
    app.on_exit();
}

TEST(ScenesAppTest, SubscribesAndUnsubscribesFromEventBus) {
    {
        ScenesApp app;
    }
    // Publishing after destruction must not crash
    Event mood_event;
    mood_event.type = EventType::MOOD_CHANGED;
    mood_event.data.mood.personality_context = nullptr;
    g_event_bus.publish(mood_event);
}

TEST(ScenesAppTest, ReceivesTouchAndMotionEventsPublishedOnGlobalBus) {
    // Regression: ScenesApp previously never subscribed to TOUCH_EVENT or
    // MOTION_EVENT, so Shell::poll_sensors()'s g_event_bus.publish() never
    // reached the active scene in production.
    auto& registry = SceneRegistry::instance();
    registry.clear();
    registry.register_scene("bus_mock", []() { return new MockScene("bus_mock"); });

    ScenesApp app;
    app.on_enter();
    auto* mock = dynamic_cast<MockScene*>(app.get_current_scene());
    ASSERT_NE(mock, nullptr);

    Event touch_event;
    touch_event.type = EventType::TOUCH_EVENT;
    touch_event.data.touch = {10, 20, 100, 255};
    g_event_bus.publish(touch_event);
    EXPECT_EQ(mock->touch_count, 1);

    Event motion_event;
    motion_event.type = EventType::MOTION_EVENT;
    motion_event.data.motion = {0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0};
    g_event_bus.publish(motion_event);
    EXPECT_EQ(mock->motion_count, 1);

    app.on_exit();
}
