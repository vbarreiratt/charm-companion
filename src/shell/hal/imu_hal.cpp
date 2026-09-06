// src/shell/hal/imu_hal.cpp
#include "shell/hal/imu_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <Wire.h>
#include <SensorQMI8658.hpp>
#else
#include <cstdio>
#endif

// IMU (QMI8658): shares I2C bus (addr 0x6B) with Touch and PMU on GPIO 14 (SCL) / GPIO 15 (SDA).

IMUHAL& IMUHAL::instance() {
    static IMUHAL inst;
    return inst;
}

bool IMUHAL::init() {
#if defined(ARDUINO)
    Serial.println("IMUHAL::init() — QMI8658 initialization");
    qmi_ = new SensorQMI8658();
    if (!qmi_->begin(Wire, 0x6B, IMU_SDA_PIN, IMU_SCL_PIN)) {
        Serial.println("IMUHAL: qmi_->begin() failed");
        return false;
    }
    qmi_->configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz, SensorQMI8658::LPF_MODE_0);
    qmi_->configGyroscope(SensorQMI8658::GYR_RANGE_64DPS, SensorQMI8658::GYR_ODR_896_8Hz, SensorQMI8658::LPF_MODE_3);
    qmi_->enableAccelerometer();
    qmi_->enableGyroscope();
    return true;
#else
    printf("IMUHAL::init() — QMI8658 initialization\n");
    return true;
#endif
}

bool IMUHAL::read_motion(float* ax, float* ay, float* az, float* gx, float* gy, float* gz) {
#if defined(ARDUINO)
    if (!qmi_ || !qmi_->getDataReady()) return false;
    float lax = 0.0f, lay = 0.0f, laz = 0.0f;
    float lgx = 0.0f, lgy = 0.0f, lgz = 0.0f;
    bool got_accel = qmi_->getAccelerometer(lax, lay, laz);
    bool got_gyro = qmi_->getGyroscope(lgx, lgy, lgz);
    if (ax) *ax = lax;
    if (ay) *ay = lay;
    if (az) *az = laz;
    if (gx) *gx = lgx;
    if (gy) *gy = lgy;
    if (gz) *gz = lgz;
    return got_accel || got_gyro;
#else
    if (ax) *ax = 0.0f;
    if (ay) *ay = 0.0f;
    if (az) *az = 0.0f;
    if (gx) *gx = 0.0f;
    if (gy) *gy = 0.0f;
    if (gz) *gz = 0.0f;
    return true;
#endif
}
