// src/shell/hal/touch_hal.cpp
#include "shell/hal/touch_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

// Touch (CST9217): Shares I2C bus (addr 0x5A) with IMU and PMU on GPIO 14 (SCL) / GPIO 15 (SDA).
// Touch INT is on GPIO 11, RST is on GPIO 2 (NOT GPIO 1, which is display reset).
//
// ROTATION note: Touch axis calibration (ROTATION=3) was originally tuned for TFT_ROTATION=3.
// After the TFT_ROTATION=0 display fix, touch axis mapping needs physical finger re-verification
// on real hardware (synthetic serial injection does not expose axis inversion/swap issues).

TouchHAL& TouchHAL::instance() {
    static TouchHAL inst;
    return inst;
}

bool TouchHAL::init() {
#if defined(ARDUINO)
    Serial.println("TouchHAL::init() — CST9217 initialization");
#else
    printf("TouchHAL::init() — CST9217 initialization\n");
#endif
    // TODO: Initialize CST9217 I2C controller (addr 0x5A), configure INT (GPIO 11) & RST (GPIO 2)
    return true;
}

bool TouchHAL::get_touch_point(uint16_t* x, uint16_t* y) {
    // TODO: Read touch coordinates from CST9217 via I2C
    return false;  // No touch active
}
