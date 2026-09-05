// src/apps/home/home_ui.cpp
#include "apps/home/home_ui.h"

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

void HomeUI::render_home_screen(Canvas* canvas, const PersonalityContext& ctx, 
                                float motion_intensity, uint32_t anim_frame) {
    // TODO: Implement full home screen layout
    render_spicy_eyes(canvas, ctx, motion_intensity, anim_frame);
    render_mood_display(canvas, ctx);
    render_app_launcher(canvas);
}

void HomeUI::render_spicy_eyes(Canvas* canvas, const PersonalityContext& ctx, 
                               float motion_intensity, uint32_t anim_frame) {
    (void)canvas;
    // TODO: Draw animated eyes based on mood + motion
#if defined(ARDUINO)
    Serial.printf("Rendering Spicy eyes (mood=%d, motion=%f, frame=%u)\n", 
                  static_cast<int>(ctx.mood), motion_intensity, static_cast<unsigned int>(anim_frame));
#else
    printf("Rendering Spicy eyes (mood=%d, motion=%f, frame=%u)\n", 
           static_cast<int>(ctx.mood), motion_intensity, static_cast<unsigned int>(anim_frame));
#endif
}

void HomeUI::render_mood_display(Canvas* canvas, const PersonalityContext& ctx) {
    (void)canvas;
    (void)ctx;
    // TODO: Draw current mood indicator
#if defined(ARDUINO)
    Serial.println("Rendering mood display");
#else
    printf("Rendering mood display\n");
#endif
}

void HomeUI::render_app_launcher(Canvas* canvas) {
    (void)canvas;
    // TODO: Draw app launcher grid
#if defined(ARDUINO)
    Serial.println("Rendering app launcher");
#else
    printf("Rendering app launcher\n");
#endif
}
