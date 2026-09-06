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

TEST(EyeSceneTest, BlinkFullyClosesEyelid) {
    EyeScene scene;
    scene.on_enter();
    TouchEvent te{0, 0, 0, 0};
    scene.on_touch(te);
    for (int i = 0; i < 10; ++i) scene.update(16);  // past blink_duration (150ms)

    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();
    scene.render(&canvas, ctx);

    // These pixels differ between the open and closed states, so they actually
    // discriminate the bug (unlike (233,233)/(233,150), which are the same
    // color whether the eye is open or closed and would pass even if
    // update() incorrectly reset back to the open state on this frame).
    EXPECT_EQ(canvas.get_pixel(263, 233), COLOR_BG_BLACK);     // within the eyelid bar (would be iris color if open)
    EXPECT_EQ(canvas.get_pixel(233, 263), COLOR_MONO_NEUTRAL); // below the bar, sclera (would be iris color if open)
}
