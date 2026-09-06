# Phase 1 Hardware Completion — Sensor Drivers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the Phase 1 Touch/IMU/Power HAL stubs with real I2C drivers for the CST9217 touch controller, QMI8658 IMU, and AXP2101 PMU, using the same maintained libraries Waveshare's own reference firmware for this exact board uses.

**Architecture:** No interface changes — `TouchHAL`, `IMUHAL`, `PowerHAL` keep their existing public method signatures (`init()`, `get_touch_point()`, `read_motion()`, `get_battery_percent()`). Each gains an `ARDUINO`-only pointer member to a real driver object (forward-declared in the header, defined in the `.cpp`), matching the pattern already established by `DisplayHAL` in the display-completion plan. All three chips share one I2C bus (`Wire.begin(15, 14)`), already wired identically today.

**Tech Stack:**
- **Touch:** `SensorLib`'s `TouchDrvCST92xx` (CST9217 is a member of the CST92xx family)
- **IMU:** `SensorLib`'s `SensorQMI8658`
- **Power:** `XPowersLib`'s `XPowersAXP2101`
- Both libraries are maintained by lewisxhe and are the exact libraries vendored in Waveshare's own official reference repo for this board (`github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C`, under `examples/arduino/libraries/`)

**Spec:** `docs/superpowers/specs/2026-09-05-charm-companion-architecture.md`

## Global Constraints

- Shared I2C bus pins (from `config/pin_config.h`): SDA=15, SCL=14 (already `Wire.begin()`-compatible; all three peripherals share this one bus).
- I2C addresses (confirmed against Waveshare's own reference repo's constants and board-specific example sketches): Touch (CST9217) = `0x5A`, IMU (QMI8658) = `0x6B`, PMU (AXP2101) = `0x34`.
- Touch reset/interrupt pins: `TOUCH_RESET_PIN=2`, `TOUCH_INT_PIN=11` (from `config/pin_config.h`).
- Touch axis mapping is passed through **raw, unmodified** from the driver. Do not add axis swap/invert/rotation logic — the existing project comment (carried forward) documents that this requires physical-hardware finger verification, which is out of scope without a device on hand.
- None of this plan's new code paths are exercised by `pio test -e native` (no I2C bus exists on host) — native behavior for all three HALs is **unchanged** from the Phase 1 stub (same return values: `get_touch_point` false, `read_motion` zeros/true, `get_battery_percent` 50). Verification for the `ARDUINO`-only code is compilation success on the real target (`pio run -e waveshare-amoled-175c`), not a host-side test.
- Naming: PascalCase for classes, snake_case for functions, UPPER_CASE for constants.

---

## Task 1: Real TouchHAL (CST9217 via SensorLib)

**Files:**
- Modify: `src/shell/hal/touch_hal.h`
- Modify: `src/shell/hal/touch_hal.cpp`
- Modify: `platformio.ini` (add `SensorLib` to `lib_deps`)

**Interfaces:**
- Consumes: nothing from other tasks in this plan.
- Produces: same public API as before (`init()`, `get_touch_point(uint16_t*, uint16_t*)`) — behavior now real on `ARDUINO`, unchanged stub on native.

- [ ] **Step 1: Add `SensorLib` to `platformio.ini`**

In `[env:waveshare-amoled-175c]`, change:
```ini
lib_deps =
    https://github.com/moononournation/Arduino_GFX.git#v1.6.4
```
to:
```ini
lib_deps =
    https://github.com/moononournation/Arduino_GFX.git#v1.6.4
    https://github.com/lewisxhe/SensorsLib.git
```
(Note: the upstream git repo is named `SensorsLib` — plural — while the Arduino library name is `SensorLib` — singular. Use the git URL directly, exactly as above, to avoid the ambiguity.)

- [ ] **Step 2: Modify `src/shell/hal/touch_hal.h`**

```cpp
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
```

- [ ] **Step 3: Modify `src/shell/hal/touch_hal.cpp`**

```cpp
// src/shell/hal/touch_hal.cpp
#include "shell/hal/touch_hal.h"
#include "config/pin_config.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <Wire.h>
#include <touch/TouchDrvCST92xx.h>
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
```

- [ ] **Step 4: Run the native test suite to confirm behavior is unchanged**

Run: `pio test -e native`
Expected: all tests PASS (this task's `ARDUINO`-only code path is not exercised on native).

- [ ] **Step 5: Verify the real target compiles**

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS.

- [ ] **Step 6: Commit**

```bash
git add src/shell/hal/touch_hal.h src/shell/hal/touch_hal.cpp platformio.ini
git commit -m "feat(touch_hal): implement real CST9217 driver via SensorLib"
```

---

## Task 2: Real IMUHAL (QMI8658 via SensorLib)

**Files:**
- Modify: `src/shell/hal/imu_hal.h`
- Modify: `src/shell/hal/imu_hal.cpp`

**Interfaces:**
- Consumes: `SensorLib` (added to `lib_deps` in Task 1 — no further `platformio.ini` change needed here).
- Produces: same public API as before (`init()`, `read_motion(float*, float*, float*, float*, float*, float*)`) — behavior now real on `ARDUINO`, unchanged stub on native.

- [ ] **Step 1: Modify `src/shell/hal/imu_hal.h`**

```cpp
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
```

- [ ] **Step 2: Modify `src/shell/hal/imu_hal.cpp`**

```cpp
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
```

- [ ] **Step 3: Run the native test suite to confirm behavior is unchanged**

Run: `pio test -e native`
Expected: all tests PASS.

- [ ] **Step 4: Verify the real target compiles**

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS.

- [ ] **Step 5: Commit**

```bash
git add src/shell/hal/imu_hal.h src/shell/hal/imu_hal.cpp
git commit -m "feat(imu_hal): implement real QMI8658 driver via SensorLib"
```

---

## Task 3: Real PowerHAL (AXP2101 via XPowersLib)

**Files:**
- Modify: `src/shell/hal/power_hal.h`
- Modify: `src/shell/hal/power_hal.cpp`
- Modify: `platformio.ini` (add `XPowersLib` to `lib_deps` and `-DXPOWERS_CHIP_AXP2101` to `build_flags`)

**Interfaces:**
- Consumes: nothing from other tasks in this plan.
- Produces: same public API as before (`init()`, `get_battery_percent()`) — behavior now real on `ARDUINO`, unchanged stub on native.

- [ ] **Step 1: Modify `platformio.ini`**

In `[env:waveshare-amoled-175c]`, change:
```ini
build_flags =
    -I.
    -Iconfig
    -Isrc
    -DBOARD_HAS_PSRAM=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DWAVESHARE_AMOLED_175C=1
    -DHAS_SCREEN=1
    -DUSE_ARDUINO_GFX=1
    -DTFT_DATABUS_N=1
    -DTFT_DISPLAY_DRIVER_N=41
    -DTFT_ROTATION=0
    -DTFT_USE_CANVAS=1
    -DROTATION=3
lib_deps =
    https://github.com/moononournation/Arduino_GFX.git#v1.6.4
    https://github.com/lewisxhe/SensorsLib.git
```
to:
```ini
build_flags =
    -I.
    -Iconfig
    -Isrc
    -DBOARD_HAS_PSRAM=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DWAVESHARE_AMOLED_175C=1
    -DHAS_SCREEN=1
    -DUSE_ARDUINO_GFX=1
    -DTFT_DATABUS_N=1
    -DTFT_DISPLAY_DRIVER_N=41
    -DTFT_ROTATION=0
    -DTFT_USE_CANVAS=1
    -DROTATION=3
    -DXPOWERS_CHIP_AXP2101
lib_deps =
    https://github.com/moononournation/Arduino_GFX.git#v1.6.4
    https://github.com/lewisxhe/SensorsLib.git
    https://github.com/lewisxhe/XPowersLib.git
```

- [ ] **Step 2: Modify `src/shell/hal/power_hal.h`**

```cpp
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
```

- [ ] **Step 3: Modify `src/shell/hal/power_hal.cpp`**

```cpp
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
```

- [ ] **Step 4: Run the native test suite to confirm behavior is unchanged**

Run: `pio test -e native`
Expected: all tests PASS.

- [ ] **Step 5: Verify the real target compiles**

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS.

- [ ] **Step 6: Commit**

```bash
git add src/shell/hal/power_hal.h src/shell/hal/power_hal.cpp platformio.ini
git commit -m "feat(power_hal): implement real AXP2101 driver via XPowersLib"
```

---

## Deferred / Out of Scope (tracked, not fixed here)

- **Touch axis calibration on real hardware.** This plan passes CST9217 coordinates through unmodified. Whether they need swapping/inverting for `TFT_ROTATION=0` can only be confirmed by touching the physical device — flagged for hardware bring-up, not fixed here.
- **QMI8658 accelerometer/gyroscope range and ODR tuning.** Task 2 configures `ACC_RANGE_4G`/`ACC_ODR_1000Hz` and `GYR_RANGE_64DPS`/`GYR_ODR_896_8Hz` as reasonable defaults matching SensorLib's own example; whether these ranges suit "shake to react" motion detection (per the architecture spec) should be tuned once real motion data is available on hardware.
- **Battery percent curve accuracy.** `XPowersAXP2101::getBatteryPercent()` is used as-is; no custom voltage-to-percent curve is implemented since the library's own conversion is authoritative and Waveshare's own reference firmware uses it directly.
