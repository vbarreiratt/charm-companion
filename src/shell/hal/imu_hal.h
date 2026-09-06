// src/shell/hal/imu_hal.h
#ifndef SHELL_HAL_IMU_HAL_H
#define SHELL_HAL_IMU_HAL_H

#if defined(ARDUINO)
class SensorQMI8658;
#endif

class IMUHAL {
public:
    static IMUHAL& instance();

    bool init();
    bool read_motion(float* ax, float* ay, float* az, float* gx, float* gy, float* gz);

private:
    IMUHAL() = default;
    ~IMUHAL() = default;

    IMUHAL(const IMUHAL&) = delete;
    IMUHAL& operator=(const IMUHAL&) = delete;

#if defined(ARDUINO)
    SensorQMI8658* qmi_ = nullptr;
#endif
};

#endif
