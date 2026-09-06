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
