# Phase 1 Hardware Completion — Rendering Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the `(void)canvas;` / `printf`-only drawing stubs in Home UI, Planet Scene, and Eye Scene with real pixel drawing using the `Canvas` primitive, so the device actually shows something instead of a blank/garbage screen.

**Architecture:** Each `render()`/`draw_*()` method that currently discards its `Canvas*` argument now calls real `Canvas` primitives (`fill_screen`, `fill_circle`, `fill_rect`, `draw_line`) using the already-computed personality/animation state (mood theme colors, rotation angle, blink progress, motion intensity). No new state is introduced — this plan only fills in drawing code against state these classes already compute. Visual elements that would need text/font rendering (mood *label*, app *names*) are represented as color/shape indicators instead, since `Canvas` has no text primitive — this is a deliberate scope cut, not an oversight (see Deferred section).

**Tech Stack:** No new dependencies. Pure C++ drawing against `Canvas` (from the display-completion plan), fully testable on native/host — no hardware or `ARDUINO`-only code in this plan at all.

**Spec:** `docs/superpowers/specs/2026-09-05-charm-companion-architecture.md`

**Prerequisite:** This plan requires `2026-09-05-phase1-hardware-completion-display-nvs.md` to be complete — it depends on `Canvas` (`src/utils/canvas_wrapper.h`), `Shell` passing a real `&screen_canvas` to `render()`, and `personality_nvs.cpp` existing (every test target that links `spicy.cpp` must also link `personality_nvs.cpp`, per that plan's Task 4).

## Global Constraints

- Display is 466×466px, center at `(233, 233)` — matches the constant already hardcoded in `PlanetScene::render()`/`EyeScene::render()`.
- `Canvas` primitives available (from the display-completion plan): `fill_screen(color)`, `fill_circle(cx, cy, r, color)`, `draw_circle(cx, cy, r, color)`, `fill_rect(x, y, w, h, color)`, `draw_line(x0, y0, x1, y1, color)`, `draw_pixel(x, y, color)`, `get_pixel(x, y) const`, `width()`, `height()`.
- Color constants come from `src/utils/color_utils.h` (`COLOR_BG_BLACK`, `COLOR_MONO_NEUTRAL`, `COLOR_MONO_DIM`, `COLOR_TEXT_SECONDARY`, `COLOR_ACCENT_RED`). Mood-driven colors come from `ctx.theme.primary_color`/`accent_color`/`bg_color`.
- No text/font rendering in this plan — mood and app-launcher indicators are color/shape-only.
- Naming: PascalCase for classes, snake_case for functions, UPPER_CASE for constants.
- Every `render()` entry point clears the frame first with `canvas->fill_screen(ctx.theme.bg_color)` — `Canvas` retains the previous frame's pixels otherwise (no implicit clear between ticks).

---

## Task 1: Home UI drawing (Spicy eyes, mood bar, app launcher)

**Files:**
- Modify: `src/apps/home/home_ui.cpp`
- Modify: `tests/test_home_app.cpp` (existing `RenderDoesNotCrash` test currently passes `nullptr` — must pass a real `Canvas` now that drawing code dereferences it)
- Test: `tests/test_home_ui.cpp` (new)
- Modify: `tests/CMakeLists.txt` (add `canvas_wrapper.cpp` to `test_home_app`; add new `test_home_ui` target)

**Interfaces:**
- Consumes: `Canvas` primitives (see Global Constraints), `PersonalityContext` (existing).
- Produces: no signature changes — `HomeUI::render_home_screen/render_spicy_eyes/render_mood_display/render_app_launcher` keep their existing signatures.

- [ ] **Step 1: Write the failing tests**

Create `tests/test_home_ui.cpp`:

```cpp
#include <gtest/gtest.h>
#include "apps/home/home_ui.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"
#include "utils/color_utils.h"

TEST(HomeUITest, RenderHomeScreenFillsBackground) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(0, 0), ctx.theme.bg_color);
}

TEST(HomeUITest, RenderHomeScreenDrawsEyeIrises) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(170, 190), ctx.theme.primary_color);  // left iris center
    EXPECT_EQ(canvas.get_pixel(296, 190), ctx.theme.primary_color);  // right iris center
}

TEST(HomeUITest, RenderHomeScreenDrawsMoodBar) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(200, 285), ctx.theme.primary_color);  // inside the mood bar
}

TEST(HomeUITest, RenderHomeScreenDrawsLauncherButton) {
    HomeUI ui;
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    ui.render_home_screen(&canvas, ctx, 0.0f, 0);

    EXPECT_EQ(canvas.get_pixel(233, 410), COLOR_TEXT_SECONDARY);  // launcher icon center
}
```

Update `tests/test_home_app.cpp`: replace
```cpp
// Forward-declared Canvas pointer is passed as nullptr in tests
class Canvas;
```
with
```cpp
#include "utils/canvas_wrapper.h"
```
and replace the `RenderDoesNotCrash` test body:
```cpp
TEST(HomeAppTest, RenderDoesNotCrash) {
    HomeApp app;
    app.on_enter();
    for (int i = 0; i < 5; ++i) {
        app.update(16);
        app.render(nullptr);
    }
}
```
with:
```cpp
TEST(HomeAppTest, RenderDoesNotCrash) {
    HomeApp app;
    app.on_enter();
    Canvas canvas(466, 466);
    for (int i = 0; i < 5; ++i) {
        app.update(16);
        app.render(&canvas);
    }
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native -f test_home_ui`
Expected: FAIL (assertions fail — `render_spicy_eyes`/`render_mood_display`/`render_app_launcher` are still `(void)canvas;` stubs, so the canvas stays all-zero except the background fill, and even the background fill is missing today).

- [ ] **Step 3: Implement real drawing in `src/apps/home/home_ui.cpp`**

Replace the entire file contents with:

```cpp
// src/apps/home/home_ui.cpp
#include "apps/home/home_ui.h"
#include "utils/color_utils.h"

void HomeUI::render_home_screen(Canvas* canvas, const PersonalityContext& ctx, 
                                float motion_intensity, uint32_t anim_frame) {
    if (!canvas) return;
    canvas->fill_screen(ctx.theme.bg_color);
    render_spicy_eyes(canvas, ctx, motion_intensity, anim_frame);
    render_mood_display(canvas, ctx);
    render_app_launcher(canvas);
}

void HomeUI::render_spicy_eyes(Canvas* canvas, const PersonalityContext& ctx, 
                               float motion_intensity, uint32_t anim_frame) {
    if (!canvas) return;

    const int16_t left_cx = 170;
    const int16_t right_cx = 296;
    const int16_t eye_cy = 190;
    const int16_t sclera_radius = 40;
    const int16_t iris_radius = 18;

    // Motion lifts both eyes slightly upward ("startled" look), clamped to a small range.
    float lift = motion_intensity * 4.0f;
    if (lift > 12.0f) lift = 12.0f;
    int16_t offset = static_cast<int16_t>(lift);

    // Gentle idle "breathing" pulse over the 60-frame animation cycle.
    int16_t pulse = static_cast<int16_t>((anim_frame < 30) ? (anim_frame / 6) : ((60 - anim_frame) / 6));
    int16_t iris_r = iris_radius + pulse;

    canvas->fill_circle(left_cx, eye_cy - offset, sclera_radius, COLOR_MONO_NEUTRAL);
    canvas->fill_circle(right_cx, eye_cy - offset, sclera_radius, COLOR_MONO_NEUTRAL);
    canvas->fill_circle(left_cx, eye_cy - offset, iris_r, ctx.theme.primary_color);
    canvas->fill_circle(right_cx, eye_cy - offset, iris_r, ctx.theme.primary_color);
}

void HomeUI::render_mood_display(Canvas* canvas, const PersonalityContext& ctx) {
    if (!canvas) return;

    const int16_t bar_x = 133;
    const int16_t bar_y = 280;
    const int16_t bar_w = 200;
    const int16_t bar_h = 16;

    canvas->fill_rect(bar_x, bar_y, bar_w, bar_h, ctx.theme.primary_color);
    canvas->fill_rect(bar_x, bar_y + bar_h, bar_w, 6, ctx.theme.accent_color);
}

void HomeUI::render_app_launcher(Canvas* canvas) {
    if (!canvas) return;

    const int16_t button_x = 166;
    const int16_t button_y = 380;
    const int16_t button_w = 134;
    const int16_t button_h = 60;

    canvas->fill_rect(button_x, button_y, button_w, button_h, COLOR_MONO_DIM);
    canvas->fill_circle(button_x + button_w / 2, button_y + button_h / 2, 16, COLOR_TEXT_SECONDARY);
}
```

- [ ] **Step 4: Update `tests/CMakeLists.txt`**

Add `../src/utils/canvas_wrapper.cpp` to the `test_home_app` target's source list.

Add a new target:
```cmake
add_executable(test_home_ui
    test_home_ui.cpp
    ../src/shell/event_bus.cpp
    ../src/spicy/spicy.cpp
    ../src/spicy/personality_api.cpp
    ../src/spicy/personality_nvs.cpp
    ../src/apps/home/home_ui.cpp
    ../src/utils/canvas_wrapper.cpp
)
target_link_libraries(test_home_ui gtest gtest_main)
gtest_discover_tests(test_home_ui)
```

- [ ] **Step 5: Run tests to verify they pass**

Run: `pio test -e native`
Expected: all tests PASS, including the new `HomeUITest.*` and the updated `HomeAppTest.RenderDoesNotCrash`.

- [ ] **Step 6: Verify the real target still compiles**

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS.

- [ ] **Step 7: Commit**

```bash
git add src/apps/home/home_ui.cpp tests/test_home_app.cpp tests/test_home_ui.cpp tests/CMakeLists.txt
git commit -m "feat(home_ui): draw Spicy eyes, mood bar, and app launcher"
```

---

## Task 2: Planet Scene drawing

**Files:**
- Modify: `src/apps/scenes/planet_scene.cpp`
- Test: `tests/test_planet_scene.cpp` (new)
- Modify: `tests/CMakeLists.txt` (new `test_planet_scene` target)

**Interfaces:**
- Consumes: `Canvas` primitives (Task 1's prerequisite plan), existing `PlanetScene` state (`rotation_angle`, `touch_speed_boost`).
- Produces: no signature changes.

- [ ] **Step 1: Write the failing tests**

Create `tests/test_planet_scene.cpp`:

```cpp
#include <gtest/gtest.h>
#include "apps/scenes/planet_scene.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"

TEST(PlanetSceneTest, RenderDrawsPlanetBodyAtCenter) {
    PlanetScene scene;
    scene.on_enter();
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(233, 233), ctx.theme.primary_color);
    EXPECT_EQ(canvas.get_pixel(0, 0), ctx.theme.bg_color);  // corner untouched by the planet
}

TEST(PlanetSceneTest, RenderAfterTouchStillDrawsPlanetBody) {
    PlanetScene scene;
    scene.on_enter();
    TouchEvent te{0, 0, 0, 0};
    scene.on_touch(te);
    scene.update(16);

    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();
    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(233, 233), ctx.theme.primary_color);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native -f test_planet_scene`
Expected: FAIL (compile error — `test_planet_scene` target does not exist in `tests/CMakeLists.txt` yet; add Step 4 first if your harness requires the target to exist before running, then rerun to confirm the assertions themselves fail against the current stub `draw_planet`).

- [ ] **Step 3: Implement real drawing in `src/apps/scenes/planet_scene.cpp`**

Replace the entire file contents with:

```cpp
#include "apps/scenes/planet_scene.h"
#include "spicy/personality_api.h"
#include "utils/color_utils.h"
#include <cmath>

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

PlanetScene::PlanetScene() : rotation_angle(0.0f), touch_speed_boost(0.0f) {}

void PlanetScene::on_enter() {
#if defined(ARDUINO)
    Serial.println("PlanetScene::on_enter()");
#else
    printf("PlanetScene::on_enter()\n");
#endif
    rotation_angle = 0.0f;
}

void PlanetScene::on_exit() {
#if defined(ARDUINO)
    Serial.println("PlanetScene::on_exit()");
#else
    printf("PlanetScene::on_exit()\n");
#endif
}

void PlanetScene::on_touch(const TouchEvent& e) {
    (void)e;
    touch_speed_boost = 2.0f;  // tap speeds up rotation
}

void PlanetScene::on_motion(const MotionEvent& e) {
    (void)e;
    // Motion can affect planet (phase 2)
}

void PlanetScene::update(uint32_t dt) {
    auto ctx = g_personality_api.get_context();
    float base_speed = 0.5f;
    float speed = base_speed * ctx.theme.animation_speed_factor;
    speed *= (ctx.emotional_state.playfulness / 100.0f);
    speed += touch_speed_boost;
    touch_speed_boost *= 0.95f;  // decay
    rotation_angle += speed * (dt / 1000.0f) * 360.0f;
    rotation_angle = fmodf(rotation_angle, 360.0f);
}

void PlanetScene::render(Canvas* canvas, const PersonalityContext& ctx) {
    if (!canvas) return;
    canvas->fill_screen(ctx.theme.bg_color);
    int cx = 233;
    int cy = 233;
    int radius = 80;
    float glow = ctx.emotional_state.happiness / 100.0f;
    draw_planet(canvas, cx, cy, radius, ctx.theme.primary_color, rotation_angle, glow);
}

void PlanetScene::draw_planet(Canvas* canvas, int cx, int cy, int radius,
                             uint16_t color, float rotation, float glow) {
    if (!canvas) return;

    // Glow halo: a dimmer, larger ring behind the planet body, sized by happiness.
    int16_t glow_radius = static_cast<int16_t>(radius) + static_cast<int16_t>(glow * 20.0f);
    canvas->fill_circle(cx, cy, glow_radius, COLOR_MONO_DIM);

    canvas->fill_circle(cx, cy, radius, color);

    // Rotation band: a diameter line across the planet's face, rotated to show spin.
    float rad = rotation * 3.14159265f / 180.0f;
    int16_t dx = static_cast<int16_t>(radius * cosf(rad));
    int16_t dy = static_cast<int16_t>(radius * sinf(rad));
    canvas->draw_line(cx - dx, cy - dy, cx + dx, cy + dy, COLOR_BG_BLACK);
}
```

- [ ] **Step 4: Add the new test target to `tests/CMakeLists.txt`**

```cmake
add_executable(test_planet_scene
    test_planet_scene.cpp
    ../src/shell/event_bus.cpp
    ../src/spicy/spicy.cpp
    ../src/spicy/personality_api.cpp
    ../src/spicy/personality_nvs.cpp
    ../src/apps/scenes/planet_scene.cpp
    ../src/utils/canvas_wrapper.cpp
)
target_link_libraries(test_planet_scene gtest gtest_main)
gtest_discover_tests(test_planet_scene)
```

- [ ] **Step 5: Run tests to verify they pass**

Run: `pio test -e native -f test_planet_scene`
Expected: PASS (2/2).

- [ ] **Step 6: Run the full native suite and verify the real target compiles**

Run: `pio test -e native`
Expected: all PASS.

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS.

- [ ] **Step 7: Commit**

```bash
git add src/apps/scenes/planet_scene.cpp tests/test_planet_scene.cpp tests/CMakeLists.txt
git commit -m "feat(planet_scene): draw the planet body, glow, and rotation band"
```

---

## Task 3: Eye Scene drawing

**Files:**
- Modify: `src/apps/scenes/eye_scene.cpp`
- Test: `tests/test_eye_scene.cpp` (new)
- Modify: `tests/CMakeLists.txt` (new `test_eye_scene` target)

**Interfaces:**
- Consumes: `Canvas` primitives, existing `EyeScene` state (`blink_timer`, `is_blinking`, `blink_duration`).
- Produces: no signature changes.

- [ ] **Step 1: Write the failing tests**

Create `tests/test_eye_scene.cpp`:

```cpp
#include <gtest/gtest.h>
#include "apps/scenes/eye_scene.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"
#include "utils/color_utils.h"

TEST(EyeSceneTest, RenderDrawsScleraAtCenter) {
    EyeScene scene;
    scene.on_enter();
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(233, 150), COLOR_MONO_NEUTRAL);  // top of the 100px-radius sclera
}

TEST(EyeSceneTest, IrisVisibleWhenNotBlinking) {
    EyeScene scene;
    scene.on_enter();
    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();

    scene.render(&canvas, ctx);

    EXPECT_EQ(canvas.get_pixel(263, 233), ctx.theme.primary_color);  // within iris ring, outside pupil
    EXPECT_EQ(canvas.get_pixel(233, 233), COLOR_BG_BLACK);           // pupil at dead center
}

TEST(EyeSceneTest, MidBlinkShrinksIris) {
    EyeScene scene;
    scene.on_enter();
    TouchEvent te{0, 0, 0, 0};
    scene.on_touch(te);
    scene.update(75);  // halfway through the 150ms blink

    Canvas canvas(466, 466);
    auto ctx = g_personality_api.get_context();
    scene.render(&canvas, ctx);

    // Iris radius has shrunk from 50 to ~25 — a point 30px from center that
    // showed iris color when fully open now shows sclera instead.
    EXPECT_EQ(canvas.get_pixel(263, 233), COLOR_MONO_NEUTRAL);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native -f test_eye_scene`
Expected: FAIL (add the CMake target from Step 4 first so the binary exists, then confirm the assertions fail against the current stub `draw_eye`).

- [ ] **Step 3: Implement real drawing in `src/apps/scenes/eye_scene.cpp`**

Replace the entire file contents with:

```cpp
#include "apps/scenes/eye_scene.h"
#include "spicy/personality_api.h"
#include "utils/color_utils.h"
#include <cmath>

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

EyeScene::EyeScene() = default;

void EyeScene::on_enter() {
#if defined(ARDUINO)
    Serial.println("EyeScene::on_enter()");
#else
    printf("EyeScene::on_enter()\n");
#endif
    blink_timer = 0;
    is_blinking = false;
}

void EyeScene::on_exit() {
#if defined(ARDUINO)
    Serial.println("EyeScene::on_exit()");
#else
    printf("EyeScene::on_exit()\n");
#endif
}

void EyeScene::on_touch(const TouchEvent& e) {
    (void)e;
    is_blinking = true;
    blink_timer = 0;
}

void EyeScene::on_motion(const MotionEvent& e) {
    (void)e;
    // Motion can affect eye gaze (phase 2)
}

void EyeScene::update(uint32_t dt) {
    blink_timer += dt;
    if (blink_timer >= blink_interval && !is_blinking) {
        is_blinking = true;
        blink_timer = 0;
    }
    if (is_blinking && blink_timer >= blink_duration) {
        is_blinking = false;
        blink_timer = 0;
    }
}

void EyeScene::render(Canvas* canvas, const PersonalityContext& ctx) {
    if (!canvas) return;
    canvas->fill_screen(ctx.theme.bg_color);
    float blink_progress = 0.0f;
    if (is_blinking) {
        blink_progress = static_cast<float>(blink_timer) / blink_duration;
    }
    draw_eye(canvas, 233, 233, 100, ctx.theme.primary_color, blink_progress);
}

void EyeScene::draw_eye(Canvas* canvas, int cx, int cy, int size,
                        uint16_t iris_color, float blink_progress) {
    if (!canvas) return;

    canvas->fill_circle(cx, cy, size, COLOR_MONO_NEUTRAL);  // sclera

    if (blink_progress >= 1.0f) {
        canvas->fill_rect(cx - size, cy - 4, size * 2, 8, COLOR_BG_BLACK);  // closed eyelid line
        return;
    }

    int16_t iris_radius = static_cast<int16_t>((size / 2) * (1.0f - blink_progress));
    if (iris_radius < 2) iris_radius = 2;
    canvas->fill_circle(cx, cy, iris_radius, iris_color);
    canvas->fill_circle(cx, cy, iris_radius / 3, COLOR_BG_BLACK);  // pupil
}
```

- [ ] **Step 4: Add the new test target to `tests/CMakeLists.txt`**

```cmake
add_executable(test_eye_scene
    test_eye_scene.cpp
    ../src/shell/event_bus.cpp
    ../src/spicy/spicy.cpp
    ../src/spicy/personality_api.cpp
    ../src/spicy/personality_nvs.cpp
    ../src/apps/scenes/eye_scene.cpp
    ../src/utils/canvas_wrapper.cpp
)
target_link_libraries(test_eye_scene gtest gtest_main)
gtest_discover_tests(test_eye_scene)
```

- [ ] **Step 5: Run tests to verify they pass**

Run: `pio test -e native -f test_eye_scene`
Expected: PASS (3/3).

- [ ] **Step 6: Run the full native suite and verify the real target compiles**

Run: `pio test -e native`
Expected: all PASS.

Run: `pio run -e waveshare-amoled-175c`
Expected: SUCCESS.

- [ ] **Step 7: Commit**

```bash
git add src/apps/scenes/eye_scene.cpp tests/test_eye_scene.cpp tests/CMakeLists.txt
git commit -m "feat(eye_scene): draw the eye, iris, pupil, and blink animation"
```

---

## Deferred / Out of Scope (tracked, not fixed here)

- **No text or icon rendering.** Mood is shown as a color bar, the app launcher as a colored button — not the literal "Mood: 😊 PLAYFUL" text/emoji from the architecture spec's UI sketch. Adding a bitmap font (or an icon set) is a distinct, separately-sized effort (font data table, a `draw_text()` primitive on `Canvas`, layout logic) intentionally left out of this plan.
- **App launcher is a single static button**, since Phase 1 only has one destination (`Scenes`). A real grid/carousel for multiple future apps is out of scope until there is more than one app to launch to.
- **Pixel positions and sizes (eye centers, mood bar, button) are first-pass values**, not validated against the physical circular panel's visible area (a 466×466 square framebuffer maps onto a circular display — content near the corners is clipped by the bezel). Revisit placement once this renders on real hardware.
