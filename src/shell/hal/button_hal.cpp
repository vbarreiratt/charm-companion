// src/shell/hal/button_hal.cpp
#include "shell/hal/button_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

namespace {
constexpr uint32_t DEBOUNCE_MS = 50;
}

ButtonHAL& ButtonHAL::instance() {
    static ButtonHAL inst;
    return inst;
}

bool ButtonHAL::init() {
#if defined(ARDUINO)
    Serial.println("ButtonHAL::init() — BOOT button (GPIO 0)");
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    last_raw_pressed_ = (digitalRead(BOOT_BUTTON_PIN) == LOW);
    last_change_ms_ = millis();
    return true;
#else
    printf("ButtonHAL::init() — BOOT button (GPIO 0)\n");
    return true;
#endif
}

bool ButtonHAL::was_pressed() {
#if defined(ARDUINO)
    bool raw_pressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);
    uint32_t now = millis();

    if (raw_pressed == last_raw_pressed_) {
        return false;
    }
    if (now - last_change_ms_ < DEBOUNCE_MS) {
        return false;  // bounce within the debounce window, ignore
    }

    last_change_ms_ = now;
    bool rising_edge_to_pressed = raw_pressed && !last_raw_pressed_;
    last_raw_pressed_ = raw_pressed;
    return rising_edge_to_pressed;
#else
    return false;
#endif
}
