# Waveshare ESP32-S3-Touch-AMOLED-1.75C - Pinouts & Hardware Reference

## Hardware Overview

| Component | Spec | Interface |
|-----------|------|-----------|
| **MCU** | ESP32-S3 (dual-core, 240 MHz) | — |
| **Display** | 1.75" 466×466px AMOLED (CO5300 driver) | QSPI (GPIO4-7, GPIO38, GPIO12, GPIO1) |
| **Touch** | CST9217 capacitive controller | I2C (GPIO14=SCL, GPIO15=SDA) |
| **Power** | AXP2101 PMU | I2C (GPIO14=SCL, GPIO15=SDA) |
| **IMU** | QMI8658 (6-axis) | I2C (GPIO14=SCL, GPIO15=SDA) |
| **Audio Input** | ES7210 ADC (2× digital microphones) | I2S/PDM |
| **Audio Output** | ES8311 codec | I2S |
| **Storage** | 32 MB Flash (QIO), 8 MB PSRAM (OCT mode) | QSPI bus (shared with display) |

---

## GPIO Pin Mapping

### Display (QSPI) — CO5300 AMOLED Driver
- **D0-D3:** GPIO 4-7 (4-bit data)
- **CLK:** GPIO 38
- **CS:** GPIO 12
- **RESET:** GPIO 1 (Display reset, NOT shared with touch reset)
- **Brightness:** Via I2C command (0x51), not GPIO

### Touch Input — CST9217 Controller
- **SDA:** GPIO 15
- **SCL:** GPIO 14
- **INT (interrupt):** GPIO 11
- **RESET:** GPIO 2 (**NOT** GPIO 1; separate from display reset)

### Shared I2C Bus (GPIO 14/15)
- **AXP2101** (Power Management) @ I2C addr 0x34
- **QMI8658** (IMU) @ I2C addr 0x6B
- **CST9217** (Touch) @ I2C addr 0x5A

### Audio (I2S/PDM)
- **ES7210 Microphone ADC:**
  - BCLK: GPIO 9
  - LRCK: GPIO 45
  - DIN: GPIO 10
- **ES8311 Audio Codec:**
  - MCLK: GPIO 16
  - DOUT: GPIO 8
  - PA (power amplifier): GPIO 46

### User Input
- **BOOT Button:** GPIO 0 (input pullup) — can be remapped for menu navigation

---

## Display Bus (QSPI) Architecture

### CO5300 AMOLED Driver Characteristics
- **Resolution:** 466×466px circular
- **Color:** RGB565 (16-bit)
- **Bus speed:** 40 MHz (default, tested stable)
  - Can attempt 80 MHz but not validated for long-term stability
- **Frame buffer:** 434 KB (466×466×2 bytes)
- **Self-emissive:** No backlight; brightness controlled via CO5300 register 0x51
- **Rotation support:** NO — CO5300 does not support hardware rotation; use `TFT_ROTATION=0` only

### QSPI Bus Shared Constraints
- Display flush (full-frame) and touch reset/PMU commands compete for the same SPI transaction struct
- **Mutex guard required** between:
  - Framebuffer flush task (core 0, continuous)
  - `_setBrightness()` command (any task)
  - Any other SPI peripheral (future expansion)

---

## Power Rails & Battery

### AXP2101 Management
- **Battery:** 3300–4150 mV nominal
- **ADC channels:** Battery voltage, VBUS, system voltage
- **I2C address:** 0x34 (standard for this chip)

**Battery percentage calculation** (from bruce-src):
```c
int mv = PMU.getBattVoltage();
int percent = (mv - 3300) * 100 / (4150 - 3300);
percent = constrain(percent, 1, 100);
```

---

## Touch Calibration

### CST9217 I2C Controller
- **Default address:** 0x5A
- **Interrupt:** GPIO 11 (active low)
- **Rotation handling:** Separate from display rotation
  - `TFT_ROTATION=0` (display, non-negotiable)
  - `ROTATION=3` flag (touch axis mapping; **NOT verified after TFT_ROTATION fix**)

### Axis Mapping Modes (from bruce-src/boards/waveshare-amoled-175c/interface.cpp)
```c
if (bruceConfigPins.rotation == 3) {
    touch.setMaxCoordinates(466, 466);
    touch.setSwapXY(false);
    touch.setMirrorXY(false, true);
} else if (bruceConfigPins.rotation == 0) {
    touch.setMaxCoordinates(466, 466);
    touch.setSwapXY(false);
    touch.setMirrorXY(false, false);
}
```

**Status:** ROTATION=3 was calibrated for `TFT_ROTATION=3` (old, broken display). After switching to `TFT_ROTATION=0`, touch axis mapping should be re-verified with real-finger tapping (synthetic serial injection hides misalignment).

---

## I2C Bus Topology

All I2C devices on GPIO 14 (SCL) / GPIO 15 (SDA):

| Device | Address | Purpose | Notes |
|--------|---------|---------|-------|
| AXP2101 | 0x34 | Power management, battery ADC | Managed by XPowersLib |
| CST9217 | 0x5A | Capacitive touch controller | Managed by TouchDrvCSTXXX |
| QMI8658 | 0x6B | 6-axis IMU (accel + gyro) | Optional; used by IMU examples |

**Mutex guard:** I2C bus access is serialized via `i2cMutex` (recursive mutex) in bruce-src to prevent task contention.

---

## Memory Layout

- **Flash:** 32 MB QIO mode
  - Typical firmware binary: 4–5 MB
  - Partitions: bootloader, partition table, app, OTA space, SPIFFS/FATFS as needed
- **Internal RAM:** ~300 KB total
  - After TFT init: ~167 KB free heap
- **PSRAM:** 8 MB OCT mode (80 MHz)
  - Framebuffer allocation: 434 KB
  - Remaining available: ~7.5 MB

---

## Build System Examples

### Supported Toolchains (per Waveshare CI)
- **ESP-IDF:** v5.5.5, v6.0.2 (both validated)
- **Arduino-ESP32:** 3.3.11

### Managed BSP Component
- **Name:** `waveshare/esp32_s3_touch_amoled_1_75c`
- **Version:** ^3.0.0 (from ESP-IDF component registry)
- Includes pinout definitions, board-level initialization, BSP helpers

### Arduino_GFX Library (for Arduino)
- **Source:** https://github.com/moononournation/Arduino_GFX.git
- **Driver:** CO5300 (#41)
- **Bus:** Arduino_ESP32QSPI (QSPI, not regular SPI peripheral)

---

## Known Limitations

1. **CO5300 1px window discard:** Any QSPI address window with width=1 OR height=1 is silently discarded. Workaround: PSRAM framebuffer (Arduino_Canvas) + full-frame flush.

2. **Rotation:** CO5300 does NOT support true 90° rotation. `TFT_ROTATION=0` only; compensation in software.

3. **AMOLED self-emission:** Dim RGB565 values (#3a3a3a, etc.) are still visible on self-emissive display, unlike backlit LCDs. Photography auto-exposure artifacts; use RGB565 constants, not camera verification.

4. **Touch calibration:** After TFT_ROTATION fix (changed from 3→0), touch axis mapping (`ROTATION` flag) has not been re-verified with real-finger input.

5. **FPS ceiling:** With PSRAM framebuffer workaround, full-frame flush ~56.7ms → ~17 fps max (bottleneck is byte-swapping during PSRAM read, not SPI link).

---

## Schematic & Reference Files

- **Official schematic:** https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/Schematic/
- **Example code (Arduino):** https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/examples/arduino/
- **Example code (ESP-IDF):** https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/examples/esp-idf/
- **Managed BSP:** Espressif component registry (auto-fetched by ESP-IDF)

---

**Last updated:** 2026-09-05  
**Source:** Waveshare official repository + ESP32-S3-Touch-AMOLED-1.75C development experience
