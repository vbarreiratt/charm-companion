#include "apps/scenes/eye_scene.h"
#include "spicy/personality_api.h"
#include "utils/color_utils.h"
#include <cmath>

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

EyeScene::EyeScene() = default;

void EyeScene::on_enter() {
#if defined(ARDUINO)
    Serial.println("EyeScene::on_enter()");
#else
    printf("EyeScene::on_enter()\n");
#endif
    blink_timer = 0;
    is_blinking = false;
}

void EyeScene::on_exit() {
#if defined(ARDUINO)
    Serial.println("EyeScene::on_exit()");
#else
    printf("EyeScene::on_exit()\n");
#endif
}

void EyeScene::on_touch(const TouchEvent& e) {
    (void)e;
    is_blinking = true;
    blink_timer = 0;
}

void EyeScene::on_motion(const MotionEvent& e) {
    (void)e;
    // Motion can affect eye gaze (phase 2)
}

void EyeScene::update(uint32_t dt) {
    blink_timer += dt;
    if (blink_timer >= blink_interval && !is_blinking) {
        is_blinking = true;
        blink_timer = 0;
    }
    if (is_blinking && blink_timer >= blink_duration) {
        is_blinking = false;
        blink_timer = 0;
    }
}

void EyeScene::render(Canvas* canvas, const PersonalityContext& ctx) {
    if (!canvas) return;
    canvas->fill_screen(ctx.theme.bg_color);
    float blink_progress = 0.0f;
    if (is_blinking) {
        blink_progress = static_cast<float>(blink_timer) / blink_duration;
    }
    draw_eye(canvas, 233, 233, 100, ctx.theme.primary_color, blink_progress);
}

void EyeScene::draw_eye(Canvas* canvas, int cx, int cy, int size,
                        uint16_t iris_color, float blink_progress) {
    if (!canvas) return;

    canvas->fill_circle(cx, cy, size, COLOR_MONO_NEUTRAL);  // sclera

    if (blink_progress >= 1.0f) {
        canvas->fill_rect(cx - size, cy - 4, size * 2, 8, COLOR_BG_BLACK);  // closed eyelid line
        return;
    }

    int16_t iris_radius = static_cast<int16_t>((size / 2) * (1.0f - blink_progress));
    if (iris_radius < 2) iris_radius = 2;
    canvas->fill_circle(cx, cy, iris_radius, iris_color);
    canvas->fill_circle(cx, cy, iris_radius / 3, COLOR_BG_BLACK);  // pupil
}
