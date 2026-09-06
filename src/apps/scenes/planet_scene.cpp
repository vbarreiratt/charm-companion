#include "apps/scenes/planet_scene.h"
#include "spicy/personality_api.h"
#include <cmath>

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

PlanetScene::PlanetScene() : rotation_angle(0.0f), touch_speed_boost(0.0f) {}

void PlanetScene::on_enter() {
#if defined(ARDUINO)
    Serial.println("PlanetScene::on_enter()");
#else
    printf("PlanetScene::on_enter()\n");
#endif
    rotation_angle = 0.0f;
}

void PlanetScene::on_exit() {
#if defined(ARDUINO)
    Serial.println("PlanetScene::on_exit()");
#else
    printf("PlanetScene::on_exit()\n");
#endif
}

void PlanetScene::on_touch(const TouchEvent& e) {
    (void)e;
    touch_speed_boost = 2.0f;  // tap speeds up rotation
}

void PlanetScene::on_motion(const MotionEvent& e) {
    (void)e;
    // Motion can affect planet (phase 2)
}

void PlanetScene::update(uint32_t dt) {
    auto ctx = g_personality_api.get_context();
    float base_speed = 0.5f;
    float speed = base_speed * ctx.theme.animation_speed_factor;
    speed *= (ctx.emotional_state.playfulness / 100.0f);
    speed += touch_speed_boost;
    touch_speed_boost *= 0.95f;  // decay
    rotation_angle += speed * (dt / 1000.0f) * 360.0f;
    rotation_angle = fmodf(rotation_angle, 360.0f);
}

void PlanetScene::render(Canvas* canvas, const PersonalityContext& ctx) {
    // Clear background
    // TODO: canvas->fillScreen(ctx.theme.bg_color);
    int cx = 233;
    int cy = 233;
    int radius = 80;
    float glow = ctx.emotional_state.happiness / 100.0f;
    draw_planet(canvas, cx, cy, radius, ctx.theme.primary_color, rotation_angle, glow);
}

void PlanetScene::draw_planet(Canvas* canvas, int cx, int cy, int radius,
                             uint16_t color, float rotation, float glow) {
    (void)canvas;
    (void)cx;
    (void)cy;
    (void)radius;
    (void)color;
#if defined(ARDUINO)
    Serial.printf("Drawing planet (rotation=%f, glow=%f)\n", rotation, glow);
#else
    printf("Drawing planet (rotation=%f, glow=%f)\n", rotation, glow);
#endif
}
