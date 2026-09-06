#include <gtest/gtest.h>
#include "apps/home/home_ui.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"
#include "utils/color_utils.h"

TEST(HomeUITest, RenderHomeScreenFillsBackground) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(0, 0), ctx.theme.bg_color);
}

TEST(HomeUITest, RenderHomeScreenDrawsEyeIrises) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(170, 190), ctx.theme.primary_color);  // left iris center
    EXPECT_EQ(canvas.get_pixel(296, 190), ctx.theme.primary_color);  // right iris center
}

TEST(HomeUITest, RenderHomeScreenDrawsMoodBar) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(200, 285), ctx.theme.primary_color);  // inside the mood bar
}

TEST(HomeUITest, RenderHomeScreenDrawsLauncherButton) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(233, 410), COLOR_TEXT_SECONDARY);  // launcher icon center
}
