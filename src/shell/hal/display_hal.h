// src/shell/hal/display_hal.h
#ifndef SHELL_HAL_DISPLAY_HAL_H
#define SHELL_HAL_DISPLAY_HAL_H

#include <cstdint>

class DisplayHAL {
public:
    static DisplayHAL& instance();

    bool init();
    void flush(uint8_t* frame_buffer);  // 434KB PSRAM buffer
    void set_brightness(uint8_t percent);  // 0-100

private:
    DisplayHAL() = default;
    ~DisplayHAL() = default;

    // Prevent copy/move
    DisplayHAL(const DisplayHAL&) = delete;
    DisplayHAL& operator=(const DisplayHAL&) = delete;
};

#endif
