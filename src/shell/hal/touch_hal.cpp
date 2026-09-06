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
// Read-path rewrite (2026-09-06): SensorLib's TouchDrvCST92xx::getPoint()
// (used previously) has no retry or recovery -- a single failed I2C
// transaction permanently reports "no touch" until something unrelated
// happens to unstick it. This was the root cause of touch going silent for
// 30+ seconds at a time on real hardware, confirmed by comparing against
// waveshareteam/esp32-badge's cached waveshare__esp_lcd_touch_cst9217
// component (a proven-reliable ESP-IDF driver for this exact chip, from
// the same board's official factory firmware): its read function retries
// up to 5 times with delays between the register-address write and the
// data read, and hardware-resets the chip if all retries fail. init()
// still uses SensorLib's begin() (chip detection has never failed here);
// only the per-tick data read is now a direct, from-scratch reimplementation
// of that proven protocol via Wire, bypassing SensorLib's getPoint().

namespace {
constexpr uint8_t CST9217_I2C_ADDR = 0x5A;
constexpr uint16_t CST9217_DATA_REG = 0xD000;
constexpr uint8_t CST9217_ACK_VALUE = 0xAB;
constexpr int CST9217_MAX_RETRIES = 5;

#if defined(ARDUINO)
// Reads `len` bytes from a 16-bit register address, retrying on I2C failure
// and hardware-resetting the chip if every retry is exhausted -- matching
// waveshare__esp_lcd_touch_cst9217's cst9217_read_reg().
bool cst9217_read_reg(uint16_t reg, uint8_t* data, uint8_t len) {
    for (int retry = 0; retry < CST9217_MAX_RETRIES; ++retry) {
        Wire.beginTransmission(CST9217_I2C_ADDR);
        Wire.write(static_cast<uint8_t>(reg >> 8));
        Wire.write(static_cast<uint8_t>(reg & 0xFF));
        if (Wire.endTransmission(true) != 0) {
            delay(3);
            continue;
        }

        delay(2);

        uint8_t received = Wire.requestFrom(static_cast<uint8_t>(CST9217_I2C_ADDR), len);
        if (received == len) {
            for (uint8_t i = 0; i < len; ++i) {
                data[i] = Wire.read();
            }
            return true;
        }
        delay(3);
    }

    Serial.println("[touch_hal] read failed after retries, resetting chip");
    digitalWrite(TOUCH_RESET_PIN, LOW);
    delay(10);
    digitalWrite(TOUCH_RESET_PIN, HIGH);
    delay(100);
    return false;
}
#endif

}  // namespace

bool TouchHAL::parse_touch_data(const uint8_t data[10], uint16_t* x, uint16_t* y) {
    if (data[6] != CST9217_ACK_VALUE) return false;

    uint8_t points = data[5] & 0x7F;
    if (points == 0) return false;

    // Single-touch HAL: only the first reported point is used.
    uint8_t status = data[0] & 0x0F;
    if (status != 0x06) return false;

    if (x) *x = static_cast<uint16_t>((data[1] << 4) | (data[3] >> 4));
    if (y) *y = static_cast<uint16_t>((data[2] << 4) | (data[3] & 0x0F));
    return true;
}

TouchHAL& TouchHAL::instance() {
    static TouchHAL inst;
    return inst;
}

bool TouchHAL::init() {
#if defined(ARDUINO)
    Serial.println("TouchHAL::init() — CST9217 initialization");
    pinMode(TOUCH_RESET_PIN, OUTPUT);
    digitalWrite(TOUCH_RESET_PIN, HIGH);
    touch_ = new TouchDrvCST92xx();
    touch_->setPins(TOUCH_RESET_PIN, TOUCH_INT_PIN);
    if (!touch_->begin(Wire, CST9217_I2C_ADDR, TOUCH_SDA_PIN, TOUCH_SCL_PIN)) {
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
    uint8_t data[10] = {0};
    if (!cst9217_read_reg(CST9217_DATA_REG, data, sizeof(data))) {
        return false;
    }
    return parse_touch_data(data, x, y);
#else
    (void)x;
    (void)y;
    return false;  // No touch active
#endif
}
