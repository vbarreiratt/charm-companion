#include "apps/scenes/eye_scene.h"
#include "spicy/personality_api.h"
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
    // Auto-blink every blink_interval ms
    if (blink_timer >= blink_interval && !is_blinking) {
        is_blinking = true;
        blink_timer = 0;
    }
    // End blink after blink_duration ms
    if (is_blinking && blink_timer >= blink_duration) {
        is_blinking = false;
        blink_timer = 0;
    }
}

void EyeScene::render(Canvas* canvas, const PersonalityContext& ctx) {
    // Clear background
    // TODO: canvas->fillScreen(ctx.theme.bg_color);
    float blink_progress = 0.0f;
    if (is_blinking) {
        blink_progress = static_cast<float>(blink_timer) / blink_duration;
    }
    draw_eye(canvas, 233, 233, 100, ctx.theme.primary_color, blink_progress);
}

void EyeScene::draw_eye(Canvas* canvas, int cx, int cy, int size,
                        uint16_t iris_color, float blink_progress) {
    (void)canvas;
    (void)cx;
    (void)cy;
    (void)size;
    (void)iris_color;
#if defined(ARDUINO)
    Serial.printf("Drawing eye (blink_progress=%f)\n", blink_progress);
#else
    printf("Drawing eye (blink_progress=%f)\n", blink_progress);
#endif
}
