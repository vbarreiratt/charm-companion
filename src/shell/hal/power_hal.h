// src/shell/hal/power_hal.h
#ifndef SHELL_HAL_POWER_HAL_H
#define SHELL_HAL_POWER_HAL_H

#include <cstdint>

#if defined(ARDUINO)
class XPowersAXP2101;
#endif

class PowerHAL {
public:
    static PowerHAL& instance();

    bool init();
    uint8_t get_battery_percent();  // 0-100

private:
    PowerHAL() = default;
    ~PowerHAL() = default;

    PowerHAL(const PowerHAL&) = delete;
    PowerHAL& operator=(const PowerHAL&) = delete;

#if defined(ARDUINO)
    XPowersAXP2101* pmu_ = nullptr;
#endif
};

#endif
