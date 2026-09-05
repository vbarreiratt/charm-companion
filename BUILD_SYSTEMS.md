# Build Systems & Toolchain Setup

## Supported Frameworks

### ESP-IDF (Recommended for Production)
- **Versions:** v5.5.5, v6.0.2 (Waveshare CI validates both)
- **Language:** C
- **Build:** CMake-based, `idf.py` commands
- **Components:** Managed BSP `waveshare/esp32_s3_touch_amoled_1_75c` (auto-fetched)
- **LVGL:** Official support via component registry
- **Advantages:**
  - First-party Espressif support
  - Full access to ESP-IDF features (partitions, fota, security, etc.)
  - LVGL integration mature
  - Managed BSP keeps hardware abstraction current

### Arduino (Easier Prototyping)
- **Framework:** Arduino-ESP32 3.3.11
- **Language:** C++ sketches
- **Build:** PlatformIO or Arduino IDE
- **Display library:** Arduino_GFX (https://github.com/moononournation/Arduino_GFX.git)
- **Advantages:**
  - Simpler setup for beginners
  - Large ecosystem of libraries
  - Faster iteration for GUI-focused projects
- **Disadvantages:**
  - Less control over partitions/resources
  - LVGL requires manual wrapper code

---

## Toolchain Installation

### ESP-IDF Setup (macOS / Linux)

```bash
# 1. Clone ESP-IDF repository
git clone -b v5.5.5 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf

# 2. Run install script (creates Python venv)
./install.sh esp32s3

# 3. Activate environment (add to ~/.zshrc or ~/.bash_profile for persistence)
source ~/esp/esp-idf/export.sh

# 4. Verify
idf.py --version
```

### Arduino Setup (PlatformIO)

```bash
# Install PlatformIO CLI
pip install platformio

# Or use VS Code extension: PlatformIO IDE

# Check installation
pio --version
```

---

## Build Configuration

### ESP-IDF sdkconfig.defaults

Common configuration for this board (excerpt):

```ini
CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_ESP32S3_INSTRUCTION_CACHE_32KB=y
CONFIG_ESP32S3_DATA_CACHE_64KB=y
CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y
CONFIG_PARTITION_TABLE_TWO_OTA=y
```

**Key settings:**
- `SPIRAM` + `SPIRAM_MODE_OCT` → Enable 8MB PSRAM at 80 MHz
- `FLASHSIZE_32MB` → Declare full 32MB flash
- OTA partition table → Over-the-air update support

### PlatformIO platformio.ini (Arduino)

```ini
[env:waveshare-amoled-175c]
board = esp32-s3-devkitc-1
board_upload.flash_size = 32MB
build_flags =
  -DBOARD_HAS_PSRAM=1
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DWAVESHARE_AMOLED_175C=1
  -DUSE_ARDUINO_GFX=1
  -DTFT_DATABUS_N=1
  -DTFT_DISPLAY_DRIVER_N=41
  -DTFT_ROTATION=0           # CRITICAL: CO5300 does not support rotation
  -DTFT_USE_CANVAS=1         # PSRAM framebuffer (required for 1px window fix)
  -DROTATION=3               # Touch axis mapping
lib_deps =
  https://github.com/moononournation/Arduino_GFX.git
```

**Critical flags:**
- `TFT_ROTATION=0` → DO NOT CHANGE (CO5300 limitation)
- `TFT_USE_CANVAS=1` → Enable PSRAM framebuffer (mandatory for 1px primitives)
- `TFT_DISPLAY_DRIVER_N=41` → CO5300 chip ID (not a standard driver; only Arduino_GFX + vendor reference support it)

---

## Building from Command Line

### ESP-IDF

```bash
# 1. Activate environment
source ~/esp/esp-idf/export.sh

# 2. Navigate to project directory
cd my-esp-idf-project

# 3. Configure board
idf.py set-target esp32s3

# 4. Build
idf.py build

# 5. Flash (device on /dev/cu.usbmodem21201 or similar)
idf.py -p /dev/cu.usbmodem* flash

# 6. Monitor serial output
idf.py -p /dev/cu.usbmodem* monitor

# All-in-one: build, flash, monitor
idf.py -p /dev/cu.usbmodem* flash monitor
```

### Arduino / PlatformIO

```bash
# 1. Navigate to project directory
cd my-platformio-project

# 2. Build
pio run -e waveshare-amoled-175c

# 3. Upload (auto-detects USB port)
pio run -e waveshare-amoled-175c -t upload

# 4. Monitor
pio device monitor

# All-in-one
pio run -e waveshare-amoled-175c -t upload && pio device monitor
```

---

## Flashing Pre-built Binaries

### Using esptool.py

```bash
# Install
pip install esptool

# Flash combined image (offset 0x0)
esptool.py -p /dev/cu.usbmodem* write_flash 0x0 firmware-combined.bin

# Erase + flash (clean slate)
esptool.py -p /dev/cu.usbmodem* erase_flash
esptool.py -p /dev/cu.usbmodem* write_flash 0x0 firmware-combined.bin
```

### Factory Recovery

Waveshare provides a factory recovery image:
- **File:** `Firmware/ESP32-S3-Touch-AMOLED-1.75C-FactoryOnly-260114.bin`
- **Purpose:** Restore device to factory state (demo firmware)
- **When to use:** Only when recovering from a bad flash; for development, use release packages

```bash
esptool.py -p /dev/cu.usbmodem* erase_flash
esptool.py -p /dev/cu.usbmodem* write_flash 0x0 ESP32-S3-Touch-AMOLED-1.75C-FactoryOnly-260114.bin
```

---

## Project Structure Patterns

### ESP-IDF Pattern

```
my-project/
├── CMakeLists.txt           (top-level)
├── main/
│   ├── CMakeLists.txt
│   ├── main.c               (app_main)
│   └── ...
├── components/
│   ├── my_ui/               (custom component)
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   └── ...
│   └── ...
├── sdkconfig.defaults       (ESP-IDF config overlay)
└── ...
```

### Arduino Pattern (PlatformIO)

```
my-project/
├── platformio.ini           (PlatformIO config)
├── src/
│   └── main.cpp             (sketch entry)
├── lib/
│   └── MyLibrary/           (custom libraries)
│       ├── src/
│       └── ...
├── include/                 (header search path)
└── ...
```

---

## Common Build Issues

### ESP-IDF: Python Venv Not Found

**Error:**
```
ERROR: ESP-IDF Python virtual environment "/path/to/idf5.5_py3.11_env/bin/python" not found.
```

**Fix:**
```bash
cd ~/esp/esp-idf
./install.sh esp32s3
source ~/esp/esp-idf/export.sh
```

### Arduino_GFX: CO5300 Driver Not Available

**Error:** Compilation fails with "CO5300 not found" or undefined references.

**Fix:**
1. Ensure `lib_deps` includes the exact Arduino_GFX git URL (not Arduino library registry version, which may be stale)
2. Rebuild cleanly: `pio run --target clean`
3. Re-fetch dependencies: `pio run --target cleanall`

### Display Banding or 1px Primitives Fail

**Cause:** `TFT_USE_CANVAS=1` not set, or PSRAM allocation failed.

**Fix:**
1. Add `-DTFT_USE_CANVAS=1` to build flags
2. Verify PSRAM enabled in sdkconfig: `CONFIG_SPIRAM=y`
3. Check heap logs during boot (serial monitor)

---

## Debugging & Monitoring

### Serial Monitor Output

```bash
# ESP-IDF
idf.py -p /dev/cu.usbmodem* monitor

# PlatformIO
pio device monitor

# Raw serial (macOS)
screen /dev/cu.usbmodem* 115200
# (Exit: Ctrl+A, then Ctrl+\ or Ctrl+D)
```

### Log Levels

- `I (info)`: Normal operation
- `W (warn)`: Potentially problematic conditions
- `E (error)`: Error conditions
- `D (debug)`: Detailed diagnostic (verbose, may impact performance)

**Change log level (ESP-IDF config):**
```
CONFIG_LOG_DEFAULT_LEVEL=INFO    # Default: INFO
CONFIG_LOG_DEFAULT_LEVEL=DEBUG   # For debugging
```

### GDB Debugging

**ESP-IDF:**
```bash
idf.py -p /dev/cu.usbmodem* gdb
```

**Requires:** OpenOCD debugger and compatible adapter (not always available on dev boards)

---

## Continuous Integration (Waveshare CI)

Waveshare's GitHub Actions pipeline:
1. Builds all examples on every PR / commit
2. Validates against ESP-IDF 5.5.5 and 6.0.2, Arduino-ESP32 3.3.11
3. Publishes combined firmware packages to GitHub Releases
4. CI: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/actions

**Release packages:** Pre-built binaries available at https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/releases

---

**Last updated:** 2026-09-05  
**Reference:** Waveshare official + ESP32-S3 development experience
