// src/apps/home/home_app.cpp
#include "apps/home/home_app.h"
#include "spicy/personality_api.h"
#include "shell/event_bus.h"

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

HomeApp::HomeApp() : ui() {
    subscribe_to(EventType::MOOD_CHANGED);
}

HomeApp::~HomeApp() {
    unsubscribe_from(EventType::MOOD_CHANGED);
}

void HomeApp::on_enter() {
#if defined(ARDUINO)
    Serial.println("HomeApp::on_enter()");
#else
    printf("HomeApp::on_enter()\n");
#endif
    frame_counter = 0;
}

void HomeApp::on_exit() {
#if defined(ARDUINO)
    Serial.println("HomeApp::on_exit()");
#else
    printf("HomeApp::on_exit()\n");
#endif
}

void HomeApp::on_touch(const TouchEvent& e) {
    // TODO: Handle mood selector tap, app launcher tap
#if defined(ARDUINO)
    Serial.printf("HomeApp::on_touch(x=%u, y=%u)\n", e.x, e.y);
#else
    printf("HomeApp::on_touch(x=%u, y=%u)\n", e.x, e.y);
#endif
}

void HomeApp::on_motion(const MotionEvent& e) {
    // Motion affects Spicy's reaction
    motion_intensity = e.intensity;
}

void HomeApp::update(uint32_t dt) {
    (void)dt;
    frame_counter++;
    motion_intensity *= 0.95f;  // Decay motion
}

void HomeApp::render(Canvas* canvas) {
    auto ctx = g_personality_api.get_context();
    auto anim_frame = frame_counter % 60;  // 60-frame animation cycle
    ui.render_home_screen(canvas, ctx, motion_intensity, anim_frame);
}
