# Phase 1 Hardware Completion — Display & Persistence Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the Phase 1 display-pipeline stubs with a real, testable rendering surface: a project-owned `Canvas` drawing primitive, a real CO5300 `DisplayHAL`, `Canvas` threaded through Shell/App/Scene so nothing renders into `nullptr` anymore, and NVS-backed mood persistence so Spicy's mood survives reboots.

**Architecture:** `Canvas` (new, `src/utils/canvas_wrapper.h/.cpp`) is a plain project-owned RGB565 pixel buffer with its own software-rasterized primitives (fill/draw circle, rect, line, pixel). It does **not** wrap Arduino_GFX's `Arduino_Canvas` — that combination (CO5300 + Arduino_Canvas + 466×466 circular panel) has no first-party Waveshare reference and would add untested risk. Instead, `DisplayHAL` owns the real `Arduino_ESP32QSPI` bus + `Arduino_CO5300` panel driver, and pushes `Canvas`'s buffer to the physical panel in one shot via Arduino_GFX's inherited `draw16bitRGBBitmap()` — a single synchronous full-frame blit, which also sidesteps the CO5300 1px address-window bug (never writes a <2px window). `Shell` owns one `Canvas` instance and passes `&canvas` to `App::render()` every tick, replacing the `nullptr` placeholder. Mood persistence uses the ESP32 Arduino core's bundled `Preferences` library (no new dependency) with a native in-memory fallback for host tests.

**Tech Stack:**
- **Display:** Arduino_GFX `Arduino_ESP32QSPI` + `Arduino_CO5300` (already vendored via `platformio.ini`'s `lib_deps`)
- **Persistence:** ESP32 Arduino `Preferences` (bundled with `framework = arduino`, no new `lib_deps`)
- **Testing:** GoogleTest via both `pio test -e native` (auto-discovers all `src/**/*.cpp` except `main.cpp`) and the parallel `tests/CMakeLists.txt` per-executable harness — **both must be updated** when new source files are added; the CMakeLists list is not automatic.

**Spec:** `docs/superpowers/specs/2026-09-05-charm-companion-architecture.md` (this plan completes the Phase 1 items that spec explicitly lists as in-scope — real drivers, rendering, and NVS persistence — which Phase 1's own implementation plan deferred to stubs).

## Global Constraints

- Display: 466×466px circular AMOLED, CO5300 driver via QSPI. No hardware rotation; `rotation = 0` everywhere.
- CO5300 panel offsets for **this exact board** (verified against Waveshare's own official reference repo `github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C`, `examples/arduino/examples/01_HelloWorld/01_HelloWorld.ino`, identical across all 4 of that repo's board-specific example sketches): `col_offset1=6, row_offset1=0, col_offset2=0, row_offset2=0`.
- Display pins (from `config/pin_config.h`, confirmed matching the same reference repo's `pin_config.h`): `DISPLAY_CS_PIN=12, DISPLAY_CLK_PIN=38, DISPLAY_DATA0_PIN=4, DISPLAY_DATA1_PIN=5, DISPLAY_DATA2_PIN=6, DISPLAY_DATA3_PIN=7, DISPLAY_RESET_PIN=1`.
- No PMIC/LCD power-enable GPIO step is needed before `panel->begin()` on this board (confirmed absent from all 4 reference `.ino` files' `setup()`).
- Framebuffer: `DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t)` = 466×466×2 = 434,312 bytes (RGB565), allocated via `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)` on target, plain `new[]` on native/host.
- FPS ceiling: ~17fps (full-frame flush ~56.7ms). Every display write is a single full-frame blit — never a partial window smaller than 2px (CO5300 1px address-window bug).
- `Canvas` is project-owned, not `Arduino_Canvas`. Method names are `snake_case` per this project's naming convention (not Arduino_GFX's camelCase).
- Mood persistence: `Preferences` namespace `"spicy"`, key `"mood"`, stored as `uint8_t` (the `Mood` enum's underlying type).
- Naming: PascalCase for classes, snake_case for functions, UPPER_CASE for constants.
- Both native test harnesses (`platformio.ini`'s `env:native` — automatic via `build_src_filter = +<*> -<main.cpp>` — and `tests/CMakeLists.txt` — **manual**, one `add_executable` per test file, explicit source lists) must build and pass after every task.

---

## Task 1: Canvas drawing primitive

**Files:**
- Create: `src/utils/canvas_wrapper.h`
- Create: `src/utils/canvas_wrapper.cpp`
- Test: `tests/test_canvas_wrapper.cpp`
- Modify: `tests/CMakeLists.txt` (add `test_canvas_wrapper` target)

**Interfaces:**
- Produces: `class Canvas` with constructor `Canvas(int16_t w, int16_t h)`, destructor, `width()`, `height()`, `fill_screen(uint16_t)`, `fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color)`, `draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color)`, `fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)`, `draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)`, `draw_pixel(int16_t x, int16_t y, uint16_t color)`, `get_pixel(int16_t x, int16_t y) const -> uint16_t`, `get_buffer() -> uint16_t*`, `buffer_size_bytes() const -> size_t`. Non-copyable.
- Consumes: nothing from earlier tasks (this is the foundation).

- [ ] **Step 1: Write the failing tests**

Create `tests/test_canvas_wrapper.cpp`:

```cpp
#include <gtest/gtest.h>
#include "utils/canvas_wrapper.h"

TEST(CanvasTest, ConstructsWithZeroedBuffer) {
    Canvas c(10, 10);
    EXPECT_EQ(c.width(), 10);
    EXPECT_EQ(c.height(), 10);
    EXPECT_EQ(c.get_pixel(0, 0), 0);
    EXPECT_EQ(c.get_pixel(9, 9), 0);
}

TEST(CanvasTest, FillScreenSetsAllPixels) {
    Canvas c(4, 4);
    c.fill_screen(0xFFFF);
    for (int16_t y = 0; y < 4; ++y) {
        for (int16_t x = 0; x < 4; ++x) {
            EXPECT_EQ(c.get_pixel(x, y), 0xFFFF);
        }
    }
}

TEST(CanvasTest, DrawPixelSetsSinglePixel) {
    Canvas c(5, 5);
    c.draw_pixel(2, 3, 0x1234);
    EXPECT_EQ(c.get_pixel(2, 3), 0x1234);
    EXPECT_EQ(c.get_pixel(2, 2), 0);
}

TEST(CanvasTest, DrawPixelOutOfBoundsDoesNotCrash) {
    Canvas c(5, 5);
    c.draw_pixel(-1, 0, 0x1234);
    c.draw_pixel(0, -1, 0x1234);
    c.draw_pixel(5, 0, 0x1234);
    c.draw_pixel(0, 5, 0x1234);
    for (int16_t y = 0; y < 5; ++y) {
        for (int16_t x = 0; x < 5; ++x) {
            EXPECT_EQ(c.get_pixel(x, y), 0);
        }
    }
}

TEST(CanvasTest, FillCircleSetsPixelsWithinRadius) {
    Canvas c(21, 21);
    c.fill_circle(10, 10, 5, 0xFFFF);
    EXPECT_EQ(c.get_pixel(10, 10), 0xFFFF);   // center
    EXPECT_EQ(c.get_pixel(10, 5), 0xFFFF);    // top edge, distance == radius
    EXPECT_EQ(c.get_pixel(10, 0), 0);         // far outside radius
}

TEST(CanvasTest, DrawCircleSetsEdgePixelsOnly) {
    Canvas c(21, 21);
    c.draw_circle(10, 10, 5, 0xFFFF);
    EXPECT_EQ(c.get_pixel(15, 10), 0xFFFF);  // rightmost edge point (cx+r, cy)
    EXPECT_EQ(c.get_pixel(10, 10), 0);       // center not filled
}

TEST(CanvasTest, FillRectSetsBoundedRegion) {
    Canvas c(10, 10);
    c.fill_rect(2, 2, 3, 3, 0xABCD);
    EXPECT_EQ(c.get_pixel(2, 2), 0xABCD);
    EXPECT_EQ(c.get_pixel(4, 4), 0xABCD);
    EXPECT_EQ(c.get_pixel(5, 5), 0);
    EXPECT_EQ(c.get_pixel(1, 1), 0);
}

TEST(CanvasTest, DrawLineConnectsEndpoints) {
    Canvas c(10, 1);
    c.draw_line(0, 0, 9, 0, 0x5555);
    for (int16_t x = 0; x < 10; ++x) {
        EXPECT_EQ(c.get_pixel(x, 0), 0x5555);
    }
}

TEST(CanvasTest, BufferSizeMatchesDimensions) {
    Canvas c(466, 466);
    EXPECT_EQ(c.buffer_size_bytes(), static_cast<size_t>(466) * 466 * 2);
    EXPECT_NE(c.get_buffer(), nullptr);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native -f test_canvas_wrapper`
Expected: FAIL (compile error — `utils/canvas_wrapper.h` does not exist yet)

- [ ] **Step 3: Create `src/utils/canvas_wrapper.h`**

```cpp
// src/utils/canvas_wrapper.h
#ifndef UTILS_CANVAS_WRAPPER_H
#define UTILS_CANVAS_WRAPPER_H

#include <cstdint>
#include <cstddef>

class Canvas {
public:
    Canvas(int16_t w, int16_t h);
    ~Canvas();

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    int16_t width() const { return w_; }
    int16_t height() const { return h_; }

    void fill_screen(uint16_t color);
    void fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
    void draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void draw_pixel(int16_t x, int16_t y, uint16_t color);
    uint16_t get_pixel(int16_t x, int16_t y) const;

    uint16_t* get_buffer();
    size_t buffer_size_bytes() const;

private:
    int16_t w_;
    int16_t h_;
    uint16_t* buffer_ = nullptr;
};

#endif // UTILS_CANVAS_WRAPPER_H
```

- [ ] **Step 4: Create `src/utils/canvas_wrapper.cpp`**

```cpp
// src/utils/canvas_wrapper.cpp
#include "utils/canvas_wrapper.h"

#if defined(ARDUINO)
#include <esp_heap_caps.h>
#endif

Canvas::Canvas(int16_t w, int16_t h) : w_(w), h_(h) {
#if defined(ARDUINO)
    buffer_ = static_cast<uint16_t*>(heap_caps_malloc(
        static_cast<size_t>(w_) * h_ * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
#else
    buffer_ = new uint16_t[static_cast<size_t>(w_) * h_];
#endif
    fill_screen(0);
}

Canvas::~Canvas() {
#if defined(ARDUINO)
    if (buffer_) heap_caps_free(buffer_);
#else
    delete[] buffer_;
#endif
}

void Canvas::fill_screen(uint16_t color) {
    size_t count = static_cast<size_t>(w_) * h_;
    for (size_t i = 0; i < count; ++i) buffer_[i] = color;
}

void Canvas::draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    buffer_[static_cast<size_t>(y) * w_ + x] = color;
}

uint16_t Canvas::get_pixel(int16_t x, int16_t y) const {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return 0;
    return buffer_[static_cast<size_t>(y) * w_ + x];
}

void Canvas::fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int32_t r2 = static_cast<int32_t>(r) * r;
    for (int16_t dy = -r; dy <= r; ++dy) {
        for (int16_t dx = -r; dx <= r; ++dx) {
            if (static_cast<int32_t>(dx) * dx + static_cast<int32_t>(dy) * dy <= r2) {
                draw_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

void Canvas::draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    while (x >= y) {
        draw_pixel(cx + x, cy + y, color);
        draw_pixel(cx + y, cy + x, color);
        draw_pixel(cx - y, cy + x, color);
        draw_pixel(cx - x, cy + y, color);
        draw_pixel(cx - x, cy - y, color);
        draw_pixel(cx - y, cy - x, color);
        draw_pixel(cx + y, cy - x, color);
        draw_pixel(cx + x, cy - y, color);
        y += 1;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void Canvas::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t j = y; j < y + h; ++j) {
        for (int16_t i = x; i < x + w; ++i) {
            draw_pixel(i, j, color);
        }
    }
}

void Canvas::draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);  // negative magnitude
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;
    while (true) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

uint16_t* Canvas::get_buffer() { return buffer_; }

size_t Canvas::buffer_size_bytes() const {
    return static_cast<size_t>(w_) * h_ * sizeof(uint16_t);
}
```

- [ ] **Step 5: Add the new test target to `tests/CMakeLists.txt`**

Insert (anywhere among the other `add_executable` blocks):

```cmake
add_executable(test_canvas_wrapper
    test_canvas_wrapper.cpp
    ../src/utils/canvas_wrapper.cpp
)
target_link_libraries(test_canvas_wrapper gtest gtest_main)
gtest_discover_tests(test_canvas_wrapper)
```

- [ ] **Step 6: Run tests to verify they pass**

Run: `pio test -e native -f test_canvas_wrapper`
Expected: PASS (9/9)

- [ ] **Step 7: Commit**

```bash
git add src/utils/canvas_wrapper.h src/utils/canvas_wrapper.cpp tests/test_canvas_wrapper.cpp tests/CMakeLists.txt
git commit -m "feat: add project-owned Canvas drawing primitive"
```

---

## Task 2: Real DisplayHAL (CO5300 + QSPI)

**Files:**
- Modify: `src/shell/hal/display_hal.h`
- Modify: `src/shell/hal/display_hal.cpp`
- Modify: `platformio.ini` (pin the GFX library version)

**Interfaces:**
- Consumes: `Canvas::get_buffer() -> uint16_t*`, `Canvas::buffer_size_bytes()` (Task 1) — `DisplayHAL::flush()` is handed the buffer as `uint8_t*` and reinterprets it back to `uint16_t*` internally (matches the already-established `flush(uint8_t* frame_buffer)` signature; RGB565 pixels are the wire format either way).
- Produces: same public API as before (`init()`, `flush(uint8_t*)`, `set_brightness(uint8_t)`) — behavior now real on `ARDUINO`, unchanged stub on native.

- [ ] **Step 1: Pin the GFX library version in `platformio.ini`**

In `[env:waveshare-amoled-175c]`, change:
```ini
lib_deps =
    https://github.com/moononournation/Arduino_GFX.git
```
to:
```ini
lib_deps =
    https://github.com/moononournation/Arduino_GFX.git#v1.6.4
```

- [ ] **Step 2: Modify `src/shell/hal/display_hal.h`**

```cpp
// src/shell/hal/display_hal.h
#ifndef SHELL_HAL_DISPLAY_HAL_H
#define SHELL_HAL_DISPLAY_HAL_H

#include <cstdint>

#if defined(ARDUINO)
class Arduino_DataBus;
class Arduino_CO5300;
#endif

class DisplayHAL {
public:
    static DisplayHAL& instance();

    bool init();
    void flush(uint8_t* frame_buffer);  // 434KB PSRAM buffer, RGB565
    void set_brightness(uint8_t percent);  // 0-100

private:
    DisplayHAL() = default;
    ~DisplayHAL() = default;  // process-lifetime singleton; no cleanup on embedded target

    DisplayHAL(const DisplayHAL&) = delete;
    DisplayHAL& operator=(const DisplayHAL&) = delete;

#if defined(ARDUINO)
    Arduino_DataBus* bus_ = nullptr;
    Arduino_CO5300* panel_ = nullptr;
#endif
};

#endif
```

- [ ] **Step 3: Modify `src/shell/hal/display_hal.cpp`**

```cpp
// src/shell/hal/display_hal.cpp
#include "shell/hal/display_hal.h"
#include "config/pin_config.h"
#include "utils/color_utils.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#else
#include <cstdio>
#endif

DisplayHAL& DisplayHAL::instance() {
    static DisplayHAL inst;
    return inst;
}

bool DisplayHAL::init() {
#if defined(ARDUINO)
    Serial.println("DisplayHAL::init() — CO5300 initialization");
    // CO5300 has a 1px address-window bug where any draw with width=1 or height=1
    // silently discards pixels; flush() always writes the full 466x466 frame in
    // one shot via draw16bitRGBBitmap(), which never hits that path.
    // Display rotation must stay 0 (CO5300 does not support hardware rotation).
    bus_ = new Arduino_ESP32QSPI(
        DISPLAY_CS_PIN, DISPLAY_CLK_PIN, DISPLAY_DATA0_PIN, DISPLAY_DATA1_PIN,
        DISPLAY_DATA2_PIN, DISPLAY_DATA3_PIN);
    panel_ = new Arduino_CO5300(
        bus_, DISPLAY_RESET_PIN, 0 /* rotation */, DISPLAY_WIDTH, DISPLAY_HEIGHT,
        6 /* col_offset1 */, 0 /* row_offset1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);
    if (!panel_->begin()) {
        Serial.println("DisplayHAL: panel_->begin() failed");
        return false;
    }
    panel_->fillScreen(COLOR_BG_BLACK);
    panel_->setBrightness(128);
    return true;
#else
    printf("DisplayHAL::init() — CO5300 initialization\n");
    return true;
#endif
}

void DisplayHAL::flush(uint8_t* frame_buffer) {
#if defined(ARDUINO)
    // Full-frame PSRAM flush cycle takes ~56.7ms (~17 fps ceiling).
    if (panel_ && frame_buffer) {
        panel_->draw16bitRGBBitmap(0, 0, reinterpret_cast<uint16_t*>(frame_buffer),
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }
#else
    (void)frame_buffer;
#endif
}

void DisplayHAL::set_brightness(uint8_t percent) {
#if defined(ARDUINO)
    if (percent > 100) percent = 100;
    if (panel_) {
        panel_->setBrightness(static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255) / 100));
    }
#else
    printf("DisplayHAL::set_brightness(%d%%)\n", percent);
#endif
}
```

- [ ] **Step 4: Verify native build and full test suite still pass**

Run: `pio test -e native`
Expected: all existing tests still PASS (this task's `ARDUINO`-only code path is not exercised on native; behavior there is unchanged from the stub).

- [ ] **Step 5: Verify firmware still compiles for the real target**

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS (this is the first task where real Arduino_GFX symbols are referenced — a compile failure here means a wrong include or constructor signature, not just a stale stub).

- [ ] **Step 6: Commit**

```bash
git add src/shell/hal/display_hal.h src/shell/hal/display_hal.cpp platformio.ini
git commit -m "feat(display_hal): implement real CO5300 driver via QSPI"
```

---

## Task 3: Thread Canvas through Shell, App, and Scene

**Files:**
- Modify: `src/apps/app_base.h`
- Modify: `src/apps/home/home_ui.h`
- Modify: `src/apps/scenes/scene_base.h`
- Modify: `src/shell/shell.h`
- Modify: `src/shell/shell.cpp`
- Modify: `tests/test_shell_integration.cpp`
- Modify: `tests/CMakeLists.txt` (add `canvas_wrapper.cpp` to `test_shell_integration`'s sources)

**Interfaces:**
- Consumes: `Canvas` (Task 1), `DisplayHAL::flush(uint8_t*)` (Task 2).
- Produces: `Shell` owns a `Canvas screen_canvas` member and passes `&screen_canvas` to `App::render()` every tick instead of `nullptr`. This task does **not** touch any drawing routine bodies (`home_ui.cpp`, `planet_scene.cpp`, `eye_scene.cpp` keep their `(void)canvas;` stubs) — it only changes signatures/wiring so a real, non-null `Canvas*` flows through. Later work (a separate plan) fills in the actual drawing calls.

- [ ] **Step 1: Replace forward declarations with real includes**

In `src/apps/app_base.h`, replace:
```cpp
class Canvas;  // Forward declare (display driver provides this)
```
with:
```cpp
#include "utils/canvas_wrapper.h"
```

In `src/apps/home/home_ui.h`, replace:
```cpp
class Canvas;
```
with:
```cpp
#include "utils/canvas_wrapper.h"
```

In `src/apps/scenes/scene_base.h`, replace:
```cpp
class Canvas;
```
with:
```cpp
#include "utils/canvas_wrapper.h"
```

- [ ] **Step 2: Give `Shell` a real `Canvas` member**

In `src/shell/shell.h`, replace:
```cpp
#include "shell/event_bus.h"
#include <cstdint>
#include <cstddef>

class App;
class Canvas;

class Shell {
public:
    static Shell& instance();
    
    bool init();
```
with:
```cpp
#include "shell/event_bus.h"
#include "utils/canvas_wrapper.h"
#include <cstdint>
#include <cstddef>

class App;

class Shell {
public:
    static Shell& instance();
    
    bool init();
```

Further down in the same file, replace:
```cpp
private:
    Shell() = default;
    ~Shell();
```
with:
```cpp
private:
    Shell();
    ~Shell();
```

And replace:
```cpp
    App* current_app = nullptr;
    App* next_app = nullptr;
    bool app_transition_pending = false;
    const char* current_app_name = nullptr;
    const char* next_app_name = nullptr;
    
    void update_active_app(uint32_t dt);
```
with:
```cpp
    App* current_app = nullptr;
    App* next_app = nullptr;
    bool app_transition_pending = false;
    const char* current_app_name = nullptr;
    const char* next_app_name = nullptr;
    Canvas screen_canvas;
    
    void update_active_app(uint32_t dt);
```

- [ ] **Step 3: Wire the constructor and `render_and_flush()` in `src/shell/shell.cpp`**

Add `#include "config/pin_config.h"` near the top (needed for `DISPLAY_WIDTH`/`DISPLAY_HEIGHT`), alongside the existing includes.

Replace:
```cpp
Shell& Shell::instance() {
    static Shell sh;
    return sh;
}

Shell::~Shell() {
```
with:
```cpp
Shell& Shell::instance() {
    static Shell sh;
    return sh;
}

Shell::Shell() : screen_canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT) {}

Shell::~Shell() {
```

Replace:
```cpp
void Shell::render_and_flush() {
    if (current_app) {
        current_app->render(nullptr);
        DisplayHAL::instance().flush(nullptr);
    }
}
```
with:
```cpp
void Shell::render_and_flush() {
    if (current_app) {
        current_app->render(&screen_canvas);
        DisplayHAL::instance().flush(reinterpret_cast<uint8_t*>(screen_canvas.get_buffer()));
    }
}
```

- [ ] **Step 4: Write the failing test for the new wiring**

In `tests/test_shell_integration.cpp`, add `#include "config/pin_config.h"` to the includes, add a capture field to `MockTestApp`, and add a new test.

Change:
```cpp
    int render_count = 0;

    void on_enter() override { enter_count++; }
    void on_exit() override { exit_count++; }
    void on_touch(const TouchEvent& e) override { (void)e; touch_count++; }
    void on_motion(const MotionEvent& e) override { (void)e; motion_count++; }
    void update(uint32_t dt) override { update_count++; last_dt = dt; }
    void render(Canvas* canvas) override { (void)canvas; render_count++; }
```
to:
```cpp
    int render_count = 0;
    Canvas* last_canvas = nullptr;

    void on_enter() override { enter_count++; }
    void on_exit() override { exit_count++; }
    void on_touch(const TouchEvent& e) override { (void)e; touch_count++; }
    void on_motion(const MotionEvent& e) override { (void)e; motion_count++; }
    void update(uint32_t dt) override { update_count++; last_dt = dt; }
    void render(Canvas* canvas) override { last_canvas = canvas; render_count++; }
```

Append at the end of the file:
```cpp
TEST(ShellIntegrationTest, RenderPassesRealNonNullCanvas) {
    Shell& sh = Shell::instance();
    MockTestApp app;

    sh.switch_app(&app);
    sh.tick(16);

    ASSERT_NE(app.last_canvas, nullptr);
    EXPECT_EQ(app.last_canvas->width(), DISPLAY_WIDTH);
    EXPECT_EQ(app.last_canvas->height(), DISPLAY_HEIGHT);

    sh.switch_app(nullptr);
    sh.tick(0);
}
```

- [ ] **Step 5: Run test to verify it fails**

Run: `pio test -e native -f test_shell_integration`
Expected: FAIL to compile (`Canvas` incomplete type / `screen_canvas` not yet declared), until Step 2-3 are also applied — apply steps 1-3 first, then this test should compile and pass immediately since the feature and its test land together. Confirm the failure mode before Step 2-3 by temporarily checking out the test file alone if desired; otherwise proceed directly to Step 6 after Steps 1-3 are in place.

- [ ] **Step 6: Add `canvas_wrapper.cpp` to the `test_shell_integration` target in `tests/CMakeLists.txt`**

Change:
```cmake
add_executable(test_shell_integration
    test_shell_integration.cpp
    ../src/shell/event_bus.cpp
    ../src/shell/shell.cpp
    ../src/shell/hal/display_hal.cpp
```
to:
```cmake
add_executable(test_shell_integration
    test_shell_integration.cpp
    ../src/utils/canvas_wrapper.cpp
    ../src/shell/event_bus.cpp
    ../src/shell/shell.cpp
    ../src/shell/hal/display_hal.cpp
```

- [ ] **Step 7: Run tests to verify they pass**

Run: `pio test -e native`
Expected: all tests PASS, including the new `RenderPassesRealNonNullCanvas`.

- [ ] **Step 8: Verify the real target still compiles**

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS.

- [ ] **Step 9: Commit**

```bash
git add src/apps/app_base.h src/apps/home/home_ui.h src/apps/scenes/scene_base.h \
        src/shell/shell.h src/shell/shell.cpp tests/test_shell_integration.cpp tests/CMakeLists.txt
git commit -m "feat(shell): thread real Canvas through render pipeline instead of nullptr"
```

---

## Task 4: NVS mood persistence

**Files:**
- Create: `src/spicy/personality_nvs.h`
- Create: `src/spicy/personality_nvs.cpp`
- Modify: `src/spicy/spicy.h`
- Modify: `src/spicy/spicy.cpp`
- Modify: `src/shell/shell.cpp`
- Test: `tests/test_personality_nvs.cpp`
- Modify: `tests/test_personality_core.cpp` (persistence-through-Spicy test)
- Modify: `tests/CMakeLists.txt` (new `test_personality_nvs` target; add `personality_nvs.cpp` to every target that links `spicy.cpp`)

**Interfaces:**
- Consumes: `Mood` (from `personality_types.h`, already exists).
- Produces: `PersonalityNVS` with `void init()`, `void save_mood(Mood m)`, `Mood load_mood(Mood default_mood)`, and (native-only) `void reset_for_testing()`; global `extern PersonalityNVS g_personality_nvs;` (same singleton-by-global-instance pattern as `g_spicy` and `g_personality_api`).

- [ ] **Step 1: Write the failing tests for `PersonalityNVS`**

Create `tests/test_personality_nvs.cpp`:

```cpp
#include <gtest/gtest.h>
#include "spicy/personality_nvs.h"

class PersonalityNVSTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_personality_nvs.reset_for_testing();
    }
};

TEST_F(PersonalityNVSTest, LoadReturnsDefaultWhenNeverSaved) {
    Mood m = g_personality_nvs.load_mood(Mood::CALM);
    EXPECT_EQ(m, Mood::CALM);
}

TEST_F(PersonalityNVSTest, SaveThenLoadRoundTrips) {
    g_personality_nvs.save_mood(Mood::PLAYFUL);
    Mood m = g_personality_nvs.load_mood(Mood::CURIOUS);
    EXPECT_EQ(m, Mood::PLAYFUL);
}

TEST_F(PersonalityNVSTest, SavingDifferentMoodOverwritesPrevious) {
    g_personality_nvs.save_mood(Mood::ANGRY);
    g_personality_nvs.save_mood(Mood::SAD);
    Mood m = g_personality_nvs.load_mood(Mood::CURIOUS);
    EXPECT_EQ(m, Mood::SAD);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native -f test_personality_nvs`
Expected: FAIL (compile error — `spicy/personality_nvs.h` does not exist yet)

- [ ] **Step 3: Create `src/spicy/personality_nvs.h`**

```cpp
// src/spicy/personality_nvs.h
#ifndef SPICY_PERSONALITY_NVS_H
#define SPICY_PERSONALITY_NVS_H

#include "personality_types.h"

class PersonalityNVS {
public:
    void init();
    void save_mood(Mood m);
    Mood load_mood(Mood default_mood);

#if !defined(ARDUINO)
    // Test-only: clears the in-memory native store between tests.
    void reset_for_testing();
#endif
};

extern PersonalityNVS g_personality_nvs;

#endif // SPICY_PERSONALITY_NVS_H
```

- [ ] **Step 4: Create `src/spicy/personality_nvs.cpp`**

```cpp
// src/spicy/personality_nvs.cpp
#include "personality_nvs.h"

#if defined(ARDUINO)
#include <Preferences.h>
static Preferences s_prefs;
#else
#include <cstdio>
static bool s_has_saved_mood = false;
static Mood s_saved_mood = Mood::CURIOUS;
#endif

PersonalityNVS g_personality_nvs;

void PersonalityNVS::init() {
#if defined(ARDUINO)
    // Namespace is opened per-operation (begin/end) in save_mood/load_mood
    // rather than held open for the app's lifetime.
#else
    printf("PersonalityNVS::init() (native stub)\n");
#endif
}

void PersonalityNVS::save_mood(Mood m) {
#if defined(ARDUINO)
    s_prefs.begin("spicy", false);
    s_prefs.putUChar("mood", static_cast<uint8_t>(m));
    s_prefs.end();
#else
    s_has_saved_mood = true;
    s_saved_mood = m;
#endif
}

Mood PersonalityNVS::load_mood(Mood default_mood) {
#if defined(ARDUINO)
    s_prefs.begin("spicy", true);
    uint8_t stored = s_prefs.getUChar("mood", static_cast<uint8_t>(default_mood));
    s_prefs.end();
    return static_cast<Mood>(stored);
#else
    return s_has_saved_mood ? s_saved_mood : default_mood;
#endif
}

#if !defined(ARDUINO)
void PersonalityNVS::reset_for_testing() {
    s_has_saved_mood = false;
    s_saved_mood = Mood::CURIOUS;
}
#endif
```

- [ ] **Step 5: Add the new test target and update existing ones in `tests/CMakeLists.txt`**

Add:
```cmake
add_executable(test_personality_nvs
    test_personality_nvs.cpp
    ../src/spicy/personality_nvs.cpp
)
target_link_libraries(test_personality_nvs gtest gtest_main)
gtest_discover_tests(test_personality_nvs)
```

Add `../src/spicy/personality_nvs.cpp` to the source list of every existing target that already lists `../src/spicy/spicy.cpp` (because Step 7 below makes `spicy.cpp` depend on it): `test_personality_core`, `test_personality_api`, `test_home_app`, `test_scenes_app`, `test_shell_integration`.

- [ ] **Step 6: Run tests to verify they pass**

Run: `pio test -e native -f test_personality_nvs`
Expected: PASS (3/3). (The other targets will fail to build until Step 7 adds the include — that's expected at this point; proceed to Step 7 before re-running the full suite.)

- [ ] **Step 7: Wire persistence into `Spicy::set_mood()` and fix the stale deadlock comment**

In `src/spicy/spicy.h`, replace:
```cpp
    // NOTE: set_mood() must NOT be called from within an on_event() callback
    // in Phase 1. EventBus::publish() holds a std::mutex during listener callbacks,
    // so calling set_mood() (which publishes MOOD_CHANGED) from inside an on_event()
    // callback would result in a deadlock.
    void set_mood(Mood m);
```
with:
```cpp
    // set_mood() is safe to call from within an on_event() callback: EventBus::publish()
    // snapshots its listener list under lock and releases the lock before invoking any
    // listener, so a cascaded publish() from inside a callback does not deadlock.
    void set_mood(Mood m);
```

In `src/spicy/spicy.cpp`, add the include:
```cpp
#include "spicy.h"
#include "shell/event_bus.h"
#include "personality_nvs.h"
#include "utils/color_utils.h"
```

Replace:
```cpp
// NOTE: set_mood() must NOT be called from within an on_event() callback
// in Phase 1. EventBus::publish() holds a std::mutex during listener callbacks,
// so calling set_mood() (which publishes MOOD_CHANGED) from inside an on_event()
// callback would result in a deadlock.
void Spicy::set_mood(Mood m) {
    context.mood = m;
    apply_mood_modifiers(m);
    select_theme_for_mood(m);

    // Publish event
    Event e;
    e.type = EventType::MOOD_CHANGED;
    e.data.mood.personality_context = &context;  // safe: synchronous dispatch, g_spicy is global
    g_event_bus.publish(e);
}
```
with:
```cpp
void Spicy::set_mood(Mood m) {
    context.mood = m;
    apply_mood_modifiers(m);
    select_theme_for_mood(m);

    // Publish event
    Event e;
    e.type = EventType::MOOD_CHANGED;
    e.data.mood.personality_context = &context;  // safe: synchronous dispatch, g_spicy is global
    g_event_bus.publish(e);

    g_personality_nvs.save_mood(m);
}
```

- [ ] **Step 8: Restore persisted mood on boot in `src/shell/shell.cpp`**

Add the include:
```cpp
#include "spicy/personality_nvs.h"
```

Replace:
```cpp
    g_spicy.set_mood(Mood::CURIOUS);
```
with:
```cpp
    g_personality_nvs.init();
    g_spicy.set_mood(g_personality_nvs.load_mood(Mood::CURIOUS));
```

- [ ] **Step 9: Add a Spicy-level persistence test**

In `tests/test_personality_core.cpp`, add the include:
```cpp
#include "spicy/personality_nvs.h"
```

Append at the end of the file:
```cpp
TEST(SpicyTest, SetMoodPersistsToNVS) {
    g_personality_nvs.reset_for_testing();

    Spicy spicy;
    spicy.set_mood(Mood::SCARED);

    ASSERT_EQ(g_personality_nvs.load_mood(Mood::CURIOUS), Mood::SCARED);
}
```

- [ ] **Step 10: Run the full native test suite**

Run: `pio test -e native`
Expected: all tests PASS, including the new `PersonalityNVSTest.*` and `SpicyTest.SetMoodPersistsToNVS`.

- [ ] **Step 11: Verify the real target still compiles**

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS (`Preferences.h` is bundled with the ESP32 Arduino core, no new `lib_deps` needed).

- [ ] **Step 12: Commit**

```bash
git add src/spicy/personality_nvs.h src/spicy/personality_nvs.cpp src/spicy/spicy.h src/spicy/spicy.cpp \
        src/shell/shell.cpp tests/test_personality_nvs.cpp tests/test_personality_core.cpp tests/CMakeLists.txt
git commit -m "feat(spicy): persist mood to NVS and restore it on boot"
```

---

## Deferred / Out of Scope (tracked, not fixed here)

- `Shell::run()`'s own `while(1)` loop (in `src/shell/shell.cpp`) is dead code — `src/main.cpp`'s Arduino `loop()` calls `Shell::instance().tick(dt)` directly and never calls `Shell::run()`. Found during the pre-Phase-2 review; not touched by this plan (no task here modifies the main-loop entry point). Worth a follow-up cleanup.
- Real hardware bring-up risk: this plan's `DisplayHAL::init()`/`flush()` code path is `ARDUINO`-only and cannot be exercised by `pio test -e native` — it is only verified by successful compilation (`pio run -e waveshare-amoled-175c`) until it runs on physical hardware. Budget time for on-device debugging of the QSPI/CO5300 bring-up (address-window, offset, brightness).
- Touch/IMU/Power real drivers, and the actual Home/Scenes drawing routines that call into `Canvas`, are out of scope for this plan — tracked in separate plans (`phase1-hardware-completion-sensors`, `phase1-hardware-completion-rendering`).
