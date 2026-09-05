# Waveshare AMOLED 1.75C - Hardware & Driver Deep Dive

## Device Specification

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.75C
- **MCU:** ESP32-S3 (dual-core, 240 MHz)
- **Display:** CO5300 AMOLED driver, 466×466 px, circular 1.75", self-emissive
- **Flash:** 32 MB (QIO mode)
- **PSRAM:** 8 MB (OCT mode, 80 MHz)
- **Bus:** QSPI (40 MHz native, can go to 80 MHz)
- **Touch:** CST9217 capacitive, I2C (15, 14 pins), INT on 11, RST on 2
- **Power:** AXP2101 PMU, I2C

---

## Critical Hardware Bugs & Workarounds

### 1. CO5300 1px Address Window Silent Discard (BLOCKING)

**Bug:** CO5300 silently discards any address window with `width=1` or `height=1`. No error, no warning — pixels simply vanish.

**Impact:**
- All 1px primitives fail: `drawPixel()`, `drawFastHLine()`, `drawFastVLine()`, `drawLine()` (H/V only)
- `drawRect()` edges vanish (outline is 1px wide)
- `fillCircle()`, `fillRoundRect()`, `fillArc()` render as **stripes** — the `writeFillEllipseHelper()` fills with `writeFillRect(..., height=yt-i, ...)` bands; when height=1 (near circle equator), those bands vanish, leaving dark horizontal gaps
- Text rendering affected if transparent/no-background mode uses 1px primitives

**Proof:**
- Isolated repro in vendor's own `01_HelloWorld` code path (zero Bruce code): `fillRect(80,150,300,1)` renders nothing; same rect with height=2 renders perfectly
- Tested coordinate alignment, odd spans, parity — irrelevant; the rule is exact: `w=1 OR h=1 → no output`

**Solution Implemented: PSRAM Framebuffer**
- Use `Arduino_Canvas` (LVGL-style framebuffer in RAM)
- Draw all primitives into RAM framebuffer (never 1px windows needed)
- Full-frame flush every 20ms (one 466×466 full-screen address window only)
- Cost: 56.7ms per flush (~17 fps ceiling), 434 KB PSRAM
- Trade-off: Animation limited to ~17 fps, but all 1px primitives work

**File:** `bruce-src/lib/HAL/display/ardgfx.cpp:147` (Canvas construction), `ardgfx.cpp:179-190` (flush task)

---

### 2. TFT_ROTATION vs ROTATION Mismatch (ROTATION FIXED)

**Initial Symptom:** Circles rendered as flattened ovals, text skewed, geometry wrong.

**Root Cause:** 
- CO5300 driver source comment: "**Does not support rotation**"
- Setting `TFT_ROTATION=3` in Arduino_GFX only applies MADCTL mirror axis-flips, not true 90° transpose
- Vendor's validated reference uses `TFT_ROTATION=0` with offsets `(6,0,0,0)`

**Solution:**
- **Set `TFT_ROTATION=0`** in board INI (`.ini:-DTFT_ROTATION=0`)
- Vendor's own example uses this; circles now perfectly round, text readable
- Orientation compensation happens in software (touch mapping, dial math) via separate `ROTATION=3` flag (for axis-swap/mirror only)

**Status:** FIXED in Task 2. Verified: circles round, WiFi arcs seamless.

**File:** `bruce-src/boards/waveshare-amoled-175c/waveshare-amoled-175c.ini` (line 33)

---

### 3. _setBrightness() Dead (BRIGHTNESS FIXED)

**Bug:** `_setBrightness()` was a no-op. `tft.native()` returned `nullptr`, so brightness command never reached CO5300.

**Solution:** 
- `native()` now returns `_panel` (the physical driver) instead of `nullptr`
- Guards brightness writes with `lockPanel()` / `unlockPanel()` mutex (SPI bus shared with flush task on other core)

**File:** `bruce-src/lib/HAL/display/ardgfx.cpp:559` (native() returns _panel)
**File:** `bruce-src/boards/waveshare-amoled-175c/interface.cpp:119` (lockPanel/unlockPanel guard)

---

### 4. SPI Transaction Race (BOOT LOOP FIXED)

**Bug:** Once brightness started working (native() no longer nullptr), firmware boot-looped:
```
E spi_master: SPI_TRANS_USE_TXDATA only available for txdata transfer <= 32 bits
assert failed: spi_device_polling_end (host->cur_cs == handle->id)
```

**Root Cause:** 
- `Arduino_ESP32QSPI` shares one `spi_transaction_ext_t` globally
- Flush task (core 0) and _setBrightness (any task) raced on same SPI bus → transaction corruption

**Solution:** 
- Wrap brightness write in `lockPanel()` / `unlockPanel()` recursive mutex (same mutex guards all flush operations)
- No more concurrent SPI access

**Verified:** 20s serial capture post-fix shows 0× `Guru Meditation`, 0× assert, 0× reboot.

---

### 5. Touch Axis Calibration (ROTATION=3, NOT RE-VERIFIED)

**Status:** NOT FULLY RE-VERIFIED after TFT_ROTATION=0 fix.

**Setup:**
- `ROTATION=3` flag in board INI (separate from `TFT_ROTATION`) controls touch axis mapping (swapXY, mirrorXY)
- Originally tuned for `TFT_ROTATION=3` display orientation
- Now that display is `TFT_ROTATION=0`, touch axis mapping may need recalibration

**Note:** Task 2 fixed geometry/rotation but noted touch should be verified with real finger (not just serial injection). Serial-injected touches in Tasks 2-8 worked, but synthetic-coordinate quirks can hide real-world misalignment.

**Action for next session:** Tap-test real finger on hardware; if coordinates land wrong (tap top-left, cursor appears elsewhere), recalibrate `ROTATION` or swapXY/mirrorXY flags in `bruce-src/boards/waveshare-amoled-175c/interface.cpp:getRawTouchPoint()`.

---

## Build & Configuration

### PlatformIO Environment
```ini
[env:waveshare-amoled-175c]
board = esp32-s3-devkitc-1
board_upload.flash_size = 16MB
build_flags =
  -DBOARD_HAS_PSRAM=1
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DWAVESHARE_AMOLED_175C=1
  -DPART_16MB=1
  -DHAS_SCREEN=1
  -DUSE_ARDUINO_GFX=1
  -DTFT_DATABUS_N=1          # Arduino_ESP32QSPI
  -DTFT_DISPLAY_DRIVER_N=41  # CO5300
  -DTFT_ROTATION=0           # Critical: CO5300 does not support rotation
  -DTFT_USE_CANVAS=1         # PSRAM framebuffer (required for 1px window fix)
  -DROTATION=3               # Touch axis mapping (separate from display rotation)
lib_deps =
  https://github.com/moononournation/Arduino_GFX.git
```

### Key Define Flags
- `TFT_DATABUS_N=1` → Arduino_ESP32QSPI (not the SPI peripheral)
- `TFT_DISPLAY_DRIVER_N=41` → CO5300 (not a common driver; only Arduino_GFX + vendor reference support it)
- `TFT_ROTATION=0` → Do NOT change; CO5300 ignores other values
- `TFT_USE_CANVAS=1` → Enables PSRAM framebuffer (mandatory workaround for 1px window bug)
- `ROTATION=3` → Touch calibration, separate axis (may need re-tuning if TFT_ROTATION changes)

---

## Performance Characteristics

### Display Refresh
- **Full-frame flush:** 56.7 ms (434 KB PSRAM → SPI @ 40 MHz)
- **Effective FPS ceiling:** ~17 fps
- **Bottleneck:** `Arduino_ESP32QSPI::writePixels` byte-swapping during PSRAM read (not SPI link)

### PSRAM
- **Used:** 434 KB (466×466×2 bytes, RGB565)
- **Available after allocation:** ~7.5 MB (8 MB - 434 KB - reserves)
- **Speed:** 80 MHz OCT mode, confirmed working

### MCU Load
- **Free RAM:** ~167 KB internal heap (after TFT init)
- **Flash:** Typical firmware 4-5 MB

---

## Color Management (RGB565)

**Important:** Waveshare AMOLED is **self-emissive** (no backlight). Dim RGB565 values (e.g., `#3a3a3a` = RGB 58,58,58) are **still visible** against black. Normal LCD behavior (dim values invisible) does NOT apply.

**Practical implication:** On-display contrast is *higher* than on a backlit LCD showing the same RGB565 value. Photography/camera artifacts can make dim colors read as brighter than they are.

**RGB565 Constants (Sample from Bruce):**
```c
#define COLOR_BG_BLACK       0x0000  // #000000
#define COLOR_ACCENT_RED     0xF9A6  // #ff3333 (R=31, G=13, B=6)
#define COLOR_MONO_NEUTRAL   0xE73C  // #e8e8e8 (R=28, G=28, B=28)
#define COLOR_MONO_DIM       0x39C7  // #3a3a3a (R=7, G=7, B=7)
#define COLOR_TEXT_SECONDARY 0x9CF4  // #9e9ea8 (R=19, G=29, B=20)
```

**Derivation:** 
- RGB 8-bit → RGB565: R>>3, G>>2, B>>3 (5/6/5 bit packing)
- Example: `#ff3333` = RGB(255,33,51) → R=31, G=13, B=6 → 0xF9A6

---

## Lessons Learned

1. **Hardware is the constraint, not the library.** CO5300's 1px-window limitation is *by design* or *a chip bug*, not something Arduino_GFX can work around — framebuffer is the only solution.

2. **ESP-IDF vs PlatformIO matters.** This board works in both, but driver availability differs (Arduino_GFX for PlatformIO is simpler; ESP-IDF requires manual LVGL/driver config).

3. **PSRAM is essential.** Without framebuffer, 1px primitives fail. With it, animation FPS is capped ~17 but all graphics work. This is the trade-off.

4. **Touch calibration is display-rotation-dependent.** If you change rotation mode, re-verify touch (real finger, not serial injection).

5. **AMOLED self-emission changes photography perception.** Screenshots with camera auto-exposure can make dim colors look brighter than their RGB565 value suggests. Use a colorimeter or direct RGB565 inspection, not camera photos, for color verification.

6. **Mutex guards cross-core SPI access.** If multiple tasks touch the SPI bus (flush task + brightness command), they must be serialized with a recursive mutex.

---

## Files & Locations

| Item | Path |
|------|------|
| Board config | `bruce-src/boards/waveshare-amoled-175c/waveshare-amoled-175c.ini` |
| Display HAL | `bruce-src/lib/HAL/display/ardgfx.cpp` / `ardgfx.h` |
| Touch interface | `bruce-src/boards/waveshare-amoled-175c/interface.cpp` |
| Vendor reference | `ESP32-S3-Touch-AMOLED-1.75C/examples/arduino/examples/01_HelloWorld/` |
| Arduino_GFX | `bruce-src/.pio/libdeps/.../Arduino_GFX.git` |

---

## For Next Session: Fresh Start Checklist

- [ ] Use this CO5300 config as-is; do NOT change `TFT_ROTATION`
- [ ] Include PSRAM framebuffer from day 1 (TFT_USE_CANVAS=1)
- [ ] Add real-finger touch test early (synthetic serial injection hides axis misalignment)
- [ ] Use RGB565 constant table (not camera) for color verification
- [ ] If animation FPS matters, explore: dirty-rect flushing, 80 MHz QSPI, big-endian framebuffer (skip byte-swap)
- [ ] Test brightness control early (was a no-op for a long time; easy to miss)
- [ ] Consider ESP-IDF (main) vs PlatformIO (bruce-src) based on project scope

---

## Known Unknowns (Not Investigated)

- Whether 80 MHz QSPI would work stable (Task 1 reported 40 MHz is default, not tested higher)
- Dirty-rect flushing implementation for Arduino_Canvas (library has no built-in; would need wrapper)
- Whether big-endian framebuffer can skip the byte-swap cost
- LVGL integration on this board (if going ESP-IDF route)
- Concurrent multi-core touch + drawing on this bus (tested brightness only, not full stress)

---

**Document prepared:** 2026-09-05  
**Hardware validated:** Waveshare ESP32-S3-Touch-AMOLED-1.75C, ESP-IDF & PlatformIO compatible  
**Status:** Ready for fresh-start project
