#include "apps/scenes/planet_scene.h"
#include "spicy/personality_api.h"
#include "utils/color_utils.h"
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
    if (!canvas) return;
    canvas->fill_screen(ctx.theme.bg_color);
    int cx = 233;
    int cy = 233;
    int radius = 80;
    float glow = ctx.emotional_state.happiness / 100.0f;
    draw_planet(canvas, cx, cy, radius, ctx.theme.primary_color, rotation_angle, glow);
}

void PlanetScene::draw_planet(Canvas* canvas, int cx, int cy, int radius,
                             uint16_t color, float rotation, float glow) {
    if (!canvas) return;

    // Glow halo: a dimmer, larger ring behind the planet body, sized by happiness.
    int16_t glow_radius = static_cast<int16_t>(radius) + static_cast<int16_t>(glow * 20.0f);
    canvas->fill_circle(cx, cy, glow_radius, COLOR_MONO_DIM);

    canvas->fill_circle(cx, cy, radius, color);

    // Rotation band: a diameter line across the planet's face, rotated to show spin.
    float rad = rotation * 3.14159265f / 180.0f;
    int16_t dx = static_cast<int16_t>(radius * cosf(rad));
    int16_t dy = static_cast<int16_t>(radius * sinf(rad));
    canvas->draw_line(cx - dx, cy - dy, cx + dx, cy + dy, COLOR_BG_BLACK);
}
