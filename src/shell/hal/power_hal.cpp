// src/shell/hal/power_hal.cpp
#include "shell/hal/power_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <Wire.h>
#include <XPowersLib.h>
#else
#include <cstdio>
#endif

// PMU (AXP2101): shares I2C bus (addr 0x34) with Touch and IMU on GPIO 14 (SCL) / GPIO 15 (SDA).

PowerHAL& PowerHAL::instance() {
    static PowerHAL inst;
    return inst;
}

bool PowerHAL::init() {
#if defined(ARDUINO)
    Serial.println("PowerHAL::init() — AXP2101 initialization");
    pmu_ = new XPowersAXP2101();
    if (!pmu_->begin(Wire, 0x34, PMU_SDA_PIN, PMU_SCL_PIN)) {
        Serial.println("PowerHAL: pmu_->begin() failed");
        delete pmu_;
        pmu_ = nullptr;
        return false;
    }
    pmu_->enableBattDetection();
    pmu_->enableBattVoltageMeasure();
    return true;
#else
    printf("PowerHAL::init() — AXP2101 initialization\n");
    return true;
#endif
}

uint8_t PowerHAL::get_battery_percent() {
#if defined(ARDUINO)
    if (!pmu_ || !pmu_->isBatteryConnect()) return 50;  // no battery detected; stub default
    int percent = pmu_->getBatteryPercent();
    if (percent < 0) return 50;
    if (percent > 100) return 100;
    return static_cast<uint8_t>(percent);
#else
    return 50;  // Default stub
#endif
}
