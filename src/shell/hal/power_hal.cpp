// src/shell/hal/power_hal.cpp
#include "shell/hal/power_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

// PMU (AXP2101): Shares I2C bus (addr 0x34) with Touch and IMU on GPIO 14 (SCL) / GPIO 15 (SDA).

PowerHAL& PowerHAL::instance() {
    static PowerHAL inst;
    return inst;
}

bool PowerHAL::init() {
#if defined(ARDUINO)
    Serial.println("PowerHAL::init() — AXP2101 initialization");
#else
    printf("PowerHAL::init() — AXP2101 initialization\n");
#endif
    // TODO: Initialize AXP2101 PMU via I2C (addr 0x34), enable ADC for battery monitoring
    return true;
}

uint8_t PowerHAL::get_battery_percent() {
    // TODO: Read battery voltage via AXP2101 ADC (3300mV - 4150mV nominal) and calculate percentage
    return 50;  // Default stub
}
