// src/shell/hal/touch_hal.h
#ifndef SHELL_HAL_TOUCH_HAL_H
#define SHELL_HAL_TOUCH_HAL_H

#include <cstdint>

#if defined(ARDUINO)
class TouchDrvCST92xx;
#endif

class TouchHAL {
public:
    static TouchHAL& instance();

    bool init();
    bool get_touch_point(uint16_t* x, uint16_t* y);

    // Exposed for native unit testing -- parses a raw 10-byte CST9217 data
    // register read into a single touch point. Matches the protocol used by
    // waveshareteam/esp32-badge's cached waveshare__esp_lcd_touch_cst9217
    // component (esp_lcd_touch_cst9217_read_data()), the driver confirmed
    // to read this exact chip reliably.
    static bool parse_touch_data(const uint8_t data[10], uint16_t* x, uint16_t* y);

private:
    TouchHAL() = default;
    ~TouchHAL() = default;

    TouchHAL(const TouchHAL&) = delete;
    TouchHAL& operator=(const TouchHAL&) = delete;

#if defined(ARDUINO)
    TouchDrvCST92xx* touch_ = nullptr;
#endif
};

#endif
