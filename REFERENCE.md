# Official Reference & Project Structure

## Waveshare Official Repository

**URL:** https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C

**Key sections:**
- [Schematic files](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/tree/main/Schematic)
- [Arduino examples](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/tree/main/examples/arduino/examples)
- [ESP-IDF examples](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/tree/main/examples/esp-idf)
- [Documentation](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/tree/main/docs)
- [Firmware releases](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/releases)

---

## Example Code Overview

### Arduino Examples

| Example | Path | Purpose |
|---------|------|---------|
| **01_HelloWorld** | `examples/arduino/examples/01_HelloWorld/` | Display bring-up & basic drawing primitives |
| **02_GFX_AsciiTable** | `examples/arduino/examples/02_GFX_AsciiTable/` | GFX text rendering & fonts |
| **03_LVGL_AXP2101_ADC_Data** | `examples/arduino/examples/03_LVGL_AXP2101_ADC_Data/` | LVGL UI + power management |
| **04_LVGL_QMI8658_ui** | `examples/arduino/examples/04_LVGL_QMI8658_ui/` | LVGL UI + IMU data |
| **05_LVGL_Widgets** | `examples/arduino/examples/05_LVGL_Widgets/` | LVGL widget catalog & touch interaction |
| **06_ES7210** | `examples/arduino/examples/06_ES7210/` | Microphone input (analog) |
| **07_ES8311** | `examples/arduino/examples/07_ES8311/` | Audio codec output |

**Notes:**
- All use **Arduino_GFX** for display driving (CO5300 driver)
- LVGL examples include bundled Arduino libraries (see `examples/arduino/libraries/`)
- Entry point: `examples/arduino/examples/[NAME]/[NAME].ino`

### ESP-IDF Examples

| Example | Path | Purpose |
|---------|------|---------|
| **01_AXP2101** | `examples/esp-idf/01_AXP2101/` | Power management & battery telemetry |
| **02_lvgl_demo_v9** | `examples/esp-idf/02_lvgl_demo_v9/` | LVGL 9.0 display demo |
| **03_esp-brookesia** | `examples/esp-idf/03_esp-brookesia/` | ESP-Brookesia UI framework demo |
| **04_Immersive_block** | `examples/esp-idf/04_Immersive_block/` | Motion-driven LVGL block demo (QMI8658 + LVGL) |
| **05_Spec_Analyzer** | `examples/esp-idf/05_Spec_Analyzer/` | Microphone spectrum analyzer (ES7210 + FFT) |

**Notes:**
- All use **ESP-IDF** build system (CMake)
- LVGL comes from Espressif component registry (auto-fetched)
- Entry point: `examples/esp-idf/[NAME]/main/main.c`
- Managed BSP: `waveshare/esp32_s3_touch_amoled_1_75c` (^3.0.0)

---

## Repository Directory Layout

```
ESP32-S3-Touch-AMOLED-1.75C/
├── README.md                           (Overview)
├── CONTRIBUTING.md                     (Contribution guide)
├── LICENSE                             (Apache 2.0)
│
├── Schematic/
│   └── ESP32-S3-Touch-AMOLED-1.75C-schematic.pdf
│
├── Firmware/
│   └── ESP32-S3-Touch-AMOLED-1.75C-FactoryOnly-260114.bin (Factory recovery)
│
├── docs/
│   ├── README.md
│   ├── components.md                   (BSP & pinouts)
│   ├── ci.md                           (CI workflow)
│   ├── firmware.md                     (Release packages)
│   ├── repository-structure.md
│   ├── brookesia.md                    (Brookesia framework notes)
│   └── images/                         (Documentation images)
│
├── examples/
│   ├── arduino/
│   │   ├── examples/                   (7 Arduino sketches)
│   │   │   ├── 01_HelloWorld/
│   │   │   ├── 02_GFX_AsciiTable/
│   │   │   ├── 03_LVGL_AXP2101_ADC_Data/
│   │   │   ├── 04_LVGL_QMI8658_ui/
│   │   │   ├── 05_LVGL_Widgets/
│   │   │   ├── 06_ES7210/
│   │   │   └── 07_ES8311/
│   │   └── libraries/                  (Bundled Arduino libraries)
│   │       ├── Arduino_GFX/
│   │       ├── XPowersLib/
│   │       ├── SensorLib/
│   │       ├── LVGL/
│   │       └── ...
│   └── esp-idf/
│       ├── 01_AXP2101/
│       ├── 02_lvgl_demo_v9/
│       ├── 03_esp-brookesia/
│       ├── 04_Immersive_block/
│       └── 05_Spec_Analyzer/
│
├── config/
│   └── README.md                       (Shared sdkconfig overlays)
│
├── releases/
│   ├── README.md                       (Release packaging tools)
│   └── ...
│
├── tests/                              (CI test suites)
└── scripts/                            (Helper scripts)
```

---

## Key Dependencies

### Arduino Ecosystem
- **Arduino_GFX:** https://github.com/moononournation/Arduino_GFX.git
  - Required for CO5300 display driving
  - Must use master branch (stable) or pinned commit
- **XPowersLib:** Power management (AXP2101)
- **TouchDrvCSTXXX:** Touch controller (CST9217)
- **SensorLib:** IMU, audio codecs
- **LVGL:** UI framework (bundled or from Arduino registry)

### ESP-IDF Ecosystem
- **Managed BSP:** `waveshare/esp32_s3_touch_amoled_1_75c` (from component registry)
- **LVGL:** Auto-fetched as component (native ESP-IDF integration)
- **esp-idf:** v5.5.5 or v6.0.2

---

## Firmware Release Structure

Pre-built binaries available at: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/releases

Each release includes:
- `*-combined.zip` — Pre-built firmware package (recommended for users)
  - `*-combined.bin` → Single combined binary (flash at offset 0x0)
  - `flash_combined.sh` / `flash_combined.bat` → Helper scripts
  - `manifest-combined-assets.json` → Checksums & metadata
- Split binaries (bootloader, partition table, app, OTA) — for advanced users
- Factory recovery image — for hard reset only

**Flash procedure:**
```bash
unzip example-combined.zip
./flash_combined.sh /dev/cu.usbmodem*
```

---

## Documentation Files

| Document | Purpose |
|----------|---------|
| **components.md** | Hardware cross-check, pinouts, BSP component info |
| **firmware.md** | Firmware artifact structure, flash layout, release process |
| **ci.md** | CI workflow, matrix (ESP-IDF + Arduino versions), build triggers |
| **repository-structure.md** | Directory layout overview |
| **brookesia.md** | ESP-Brookesia UI framework integration notes |

All docs are in English + Simplified Chinese (`*_ZH.md`).

---

## Product Page

**Waveshare store:** https://www.waveshare.com/esp32-s3-touch-amoled-1.75c.htm

- Product details
- Datasheet links
- Purchase options
- Support contact

---

## Common References for Charm Companion

### When You Need...
- **Hardware pinouts:** See [PINOUTS.md](./PINOUTS.md) (local) or `docs/components.md` (Waveshare repo)
- **Display driver info:** CO5300 datasheet (check Waveshare schematic or example code)
- **Touch calibration:** `examples/arduino/examples/05_LVGL_Widgets/` (real-world touch handling)
- **Power management:** `examples/arduino/examples/03_LVGL_AXP2101_ADC_Data/`
- **Audio:** `examples/arduino/examples/06_ES7210/` (input) or `07_ES8311/` (output)
- **LVGL integration:** `examples/esp-idf/02_lvgl_demo_v9/` (ESP-IDF) or `examples/arduino/libraries/LVGL/` (Arduino)
- **Build setup:** See [BUILD_SYSTEMS.md](./BUILD_SYSTEMS.md) (local)

---

## Contributing Back to Waveshare

If you fix bugs or improve support for this board:
1. Fork: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C
2. Create feature branch: `git checkout -b fix/your-fix`
3. Follow [CONTRIBUTING.md](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/blob/main/CONTRIBUTING.md)
4. Submit PR with reproduction steps & validation

---

**Last updated:** 2026-09-05  
**Charm Companion purpose:** Consolidate hardware & toolchain knowledge for standalone development
