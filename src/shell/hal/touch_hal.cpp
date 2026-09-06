// src/shell/hal/touch_hal.cpp
#include "shell/hal/touch_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <Wire.h>
#include <touch/TouchDrvCST92xx.hpp>
#else
#include <cstdio>
#endif

// Touch (CST9217, part of the CST92xx family): shares I2C bus (addr 0x5A) with
// IMU and PMU on GPIO 14 (SCL) / GPIO 15 (SDA).
//
// ROTATION note: Touch axis calibration (ROTATION=3) was originally tuned for
// TFT_ROTATION=3. After the TFT_ROTATION=0 display fix, touch axis mapping needs
// physical finger re-verification on real hardware (synthetic serial injection
// does not expose axis inversion/swap issues) — this driver passes raw x/y
// through unchanged; do not add axis-swap/invert logic without hardware
// confirmation.
//
// IRQ/sleep investigation (2026-09-06): tried gating get_touch_point() on
// TOUCH_INT_PIN via attachInterrupt(), plus a sleep()/reset() wake cycle,
// modeled on waveshareteam/ESP32-S3-Touch-AMOLED-1.75C's reference examples.
// That made things worse (interrupt never fired; sleep() crashed the I2C
// bus with ESP_ERR_INVALID_STATE) because that repo vendors a DIFFERENT,
// incompatible fork of SensorLib than the one actually pinned in this
// project's platformio.ini (lewisxhe/SensorsLib.git@2b9e591f...). That
// exact pinned version's own bundled example
// (.pio/libdeps/waveshare-amoled-175c/SensorLib/examples/touch/
// cst9217_get_point/cst9217_get_point.ino) uses plain unconditional polling
// (touch.getTouchPoints() every 30ms, no interrupt at all) and explicitly
// comments out sleep()/reset() with the warning "Unable to obtain
// coordinates after turning on sleep" -- confirming unconditional polling
// (as below) is this library version's correct usage, not a bug to fix.
// The separate, still-unexplained symptom (touch reporting nothing for
// 30+ seconds at a time, then recovering) predates today's changes and
// remains open -- reverted to this known-if-imperfect baseline rather than
// guessing further against the wrong reference.

TouchHAL& TouchHAL::instance() {
    static TouchHAL inst;
    return inst;
}

bool TouchHAL::init() {
#if defined(ARDUINO)
    Serial.println("TouchHAL::init() — CST9217 initialization");
    touch_ = new TouchDrvCST92xx();
    touch_->setPins(TOUCH_RESET_PIN, TOUCH_INT_PIN);
    if (!touch_->begin(Wire, 0x5A, TOUCH_SDA_PIN, TOUCH_SCL_PIN)) {
        Serial.println("TouchHAL: touch_->begin() failed");
        delete touch_;
        touch_ = nullptr;
        return false;
    }
    return true;
#else
    printf("TouchHAL::init() — CST9217 initialization\n");
    return true;
#endif
}

bool TouchHAL::get_touch_point(uint16_t* x, uint16_t* y) {
#if defined(ARDUINO)
    if (!touch_) return false;
    int16_t xs[1];
    int16_t ys[1];
    // Requests a single point (this HAL's interface is single-touch).
    uint8_t touched = touch_->getPoint(xs, ys, 1);
    if (touched == 0) return false;
    if (x) *x = static_cast<uint16_t>(xs[0]);
    if (y) *y = static_cast<uint16_t>(ys[0]);
    return true;
#else
    (void)x;
    (void)y;
    return false;  // No touch active
#endif
}
