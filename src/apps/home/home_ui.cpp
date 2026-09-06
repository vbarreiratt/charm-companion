// src/apps/home/home_ui.cpp
#include "apps/home/home_ui.h"
#include "utils/color_utils.h"

void HomeUI::render_home_screen(Canvas* canvas, const PersonalityContext& ctx,
                                float motion_intensity, uint32_t anim_frame) {
    if (!canvas) return;
    canvas->fill_screen(ctx.theme.bg_color);
    render_spicy_eyes(canvas, ctx, motion_intensity, anim_frame);
    render_mood_display(canvas, ctx);
    render_app_launcher(canvas);
}

void HomeUI::render_spicy_eyes(Canvas* canvas, const PersonalityContext& ctx,
                               float motion_intensity, uint32_t anim_frame) {
    if (!canvas) return;

    const int16_t left_cx = 170;
    const int16_t right_cx = 296;
    const int16_t eye_cy = 190;
    const int16_t sclera_radius = 40;
    const int16_t iris_radius = 18;

    // Motion lifts both eyes slightly upward ("startled" look), clamped to a small range.
    float lift = motion_intensity * 4.0f;
    if (lift > 12.0f) lift = 12.0f;
    int16_t offset = static_cast<int16_t>(lift);

    // Gentle idle "breathing" pulse over the 60-frame animation cycle.
    int16_t pulse = static_cast<int16_t>((anim_frame < 30) ? (anim_frame / 6) : ((60 - anim_frame) / 6));
    int16_t iris_r = iris_radius + pulse;

    canvas->fill_circle(left_cx, eye_cy - offset, sclera_radius, COLOR_MONO_NEUTRAL);
    canvas->fill_circle(right_cx, eye_cy - offset, sclera_radius, COLOR_MONO_NEUTRAL);
    canvas->fill_circle(left_cx, eye_cy - offset, iris_r, ctx.theme.primary_color);
    canvas->fill_circle(right_cx, eye_cy - offset, iris_r, ctx.theme.primary_color);
}

void HomeUI::render_mood_display(Canvas* canvas, const PersonalityContext& ctx) {
    if (!canvas) return;

    const int16_t bar_x = 133;
    const int16_t bar_y = 280;
    const int16_t bar_w = 200;
    const int16_t bar_h = 16;

    canvas->fill_rect(bar_x, bar_y, bar_w, bar_h, ctx.theme.primary_color);
    canvas->fill_rect(bar_x, bar_y + bar_h, bar_w, 6, ctx.theme.accent_color);
}

void HomeUI::render_app_launcher(Canvas* canvas) {
    if (!canvas) return;

    const int16_t button_x = 166;
    const int16_t button_y = 380;
    const int16_t button_w = 134;
    const int16_t button_h = 60;

    canvas->fill_rect(button_x, button_y, button_w, button_h, COLOR_MONO_DIM);
    canvas->fill_circle(button_x + button_w / 2, button_y + button_h / 2, 16, COLOR_TEXT_SECONDARY);
}
