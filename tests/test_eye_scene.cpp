#include <gtest/gtest.h>
#include "apps/scenes/eye_scene.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"
#include "utils/color_utils.h"

TEST(EyeSceneTest, RenderDrawsScleraAtCenter) {
    EyeScene scene;
    scene.on_enter();
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(233, 150), COLOR_MONO_NEUTRAL);  // top of the 100px-radius sclera
}

TEST(EyeSceneTest, IrisVisibleWhenNotBlinking) {
    EyeScene scene;
    scene.on_enter();
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(263, 233), ctx.theme.primary_color);  // within iris ring, outside pupil
    EXPECT_EQ(canvas.get_pixel(233, 233), COLOR_BG_BLACK);           // pupil at dead center
}

TEST(EyeSceneTest, MidBlinkShrinksIris) {
    EyeScene scene;
    scene.on_enter();
    TouchEvent te{0, 0, 0, 0};
    scene.on_touch(te);
    scene.update(75);  // halfway through the 150ms blink

    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();
    scene.render(&canvas, ctx);

    // Iris radius has shrunk from 50 to ~25 — a point 30px from center that
    // showed iris color when fully open now shows sclera instead.
    EXPECT_EQ(canvas.get_pixel(263, 233), COLOR_MONO_NEUTRAL);
}
