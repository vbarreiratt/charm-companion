// src/shell/hal/button_hal.h
#ifndef SHELL_HAL_BUTTON_HAL_H
#define SHELL_HAL_BUTTON_HAL_H

#include <cstdint>

// BOOT button (GPIO 0, active-low, INPUT_PULLUP). Used as the physical
// back-navigation button (see docs/superpowers/specs/
// 2026-09-06-touch-navigation-home-scenes.md).
class ButtonHAL {
public:
    static ButtonHAL& instance();

    bool init();

    // True once per fresh debounced press edge; false otherwise
    // (including while the button is held down).
    bool was_pressed();

private:
    ButtonHAL() = default;
    ~ButtonHAL() = default;

    ButtonHAL(const ButtonHAL&) = delete;
    ButtonHAL& operator=(const ButtonHAL&) = delete;

#if defined(ARDUINO)
    bool last_raw_pressed_ = false;
    uint32_t last_change_ms_ = 0;
#endif
};

#endif
