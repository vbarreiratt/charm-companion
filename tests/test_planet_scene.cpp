#include <gtest/gtest.h>
#include "apps/scenes/planet_scene.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"
#include "utils/color_utils.h"

TEST(PlanetSceneTest, RenderDrawsPlanetBodyAtCenter) {
    PlanetScene scene;
    scene.on_enter();
    Canvas canvas(466, 466);
    canvas.fill_screen(COLOR_ACCENT_RED);  // sentinel; render() must overwrite it
    auto ctx = g_personality_api.get_context();

    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(233, 193), ctx.theme.primary_color);  // body pixel, off-band
    EXPECT_EQ(canvas.get_pixel(0, 0), ctx.theme.bg_color);  // corner untouched by the planet
}

TEST(PlanetSceneTest, RenderAfterTouchStillDrawsPlanetBody) {
    PlanetScene scene;
    scene.on_enter();
    TouchEvent te{0, 0, 0, 0};
    scene.on_touch(te);
    scene.update(16);

    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();
    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(233, 193), ctx.theme.primary_color);  // body pixel, off-band
}
