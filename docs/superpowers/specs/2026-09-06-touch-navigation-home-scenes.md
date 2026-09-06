# Touch Navigation: Home ↔ Scenes

**Date:** 2026-09-06
**Status:** Implemented and validated on real hardware (2026-09-06) — native suite 74/74; swipe-up (Home→Scenes) and BOOT-press back navigation (scene-by-scene, then exit to Home) confirmed via serial log and camera photo on the physical board.
**Depends on:** 2026-09-06-hardware-iteration-harness-design.md (harness used to verify this)

## Interaction design (confirmed with user)

- **Home → Scenes:** swipe gesture (finger moves up a threshold distance while touching the screen, on the Home screen). Not a tap on the existing launcher button — the button stays purely visual for now.
- **Scenes → Home:** the physical BOOT button (GPIO 0). Single press: go back one step. From any scene after the first, that means the previous scene. From the first scene, that means exit to Home. Long-press is explicitly out of scope (deferred, to be discussed later).
- Touch inside a scene keeps its existing per-scene meaning (blink the eye, speed up the planet) — swipe/back navigation must not interfere with that.

## Architecture

- **`SwipeDetector`** (`src/shell/swipe_detector.h/.cpp`) — pure logic, no hardware dependency. Fed `(touched, x, y)` once per tick; returns a `SwipeDirection` (`NONE`/`UP`/`DOWN`/`LEFT`/`RIGHT`) the instant a touch-release completes a motion past threshold. Extracted as its own class specifically so it's unit-testable without `TouchHAL`, whose native (non-`ARDUINO`) fallback always reports "not touched" — the same reason `render_reference.cpp` calls `on_touch()` directly instead of going through the real touch driver.
- **`ButtonHAL`** (`src/shell/hal/button_hal.h/.cpp`) — new HAL, same singleton/`init()` pattern as `DisplayHAL`/`TouchHAL`/`IMUHAL`/`PowerHAL`. Wraps `BOOT_BUTTON_PIN` (GPIO 0, active-low, `INPUT_PULLUP`) with debounce and edge detection: `was_pressed()` returns true once per fresh press, not continuously while held. Native fallback always returns false (matches `TouchHAL`'s "nothing happening" default).
- **`App::handle_back()`** — new virtual method on the `App` base class, default returns `false` (nothing to go back to). `ScenesApp` overrides: if not on the first scene, moves to the previous scene and returns `true`; if already on the first scene, returns `false` (nothing left to go back to within Scenes — the caller should exit to Home). This avoids needing RTTI (disabled on this build, `-fno-rtti`) or a name-based `static_cast` hack to let `Shell` special-case `ScenesApp`.
- **`Shell::handle_boot_press()`** — new public method (mirrors `poll_sensors()` being public specifically so tests can call it directly without a real `ButtonHAL` press). Calls `current_app->handle_back()`; if that returns false and the current app isn't already "home", switches to Home.
- **`Shell::poll_sensors()`** — gains: feed `SwipeDetector` from the same touch poll already happening (no behavior change to existing `TOUCH_EVENT` publishing); on `UP` while current app is "home", switch to "scenes". Also polls `ButtonHAL::instance().was_pressed()` and calls `handle_boot_press()` if true.

## Testing strategy

- `SwipeDetector`: fully unit-tested in isolation (no HAL, no Arduino) — feed synthetic `(touched, x, y)` sequences, assert the returned direction.
- `ScenesApp::handle_back()`: unit-tested directly (register test scenes, call repeatedly, assert index decrements then returns false at index 0).
- `Shell::handle_boot_press()`: unit-tested directly via a mock `App` overriding `handle_back()`, without touching `ButtonHAL` — same pattern `ShellIntegrationTest` already uses for `MockTestApp`.
- `ButtonHAL`/native GPIO reading: not unit-tested (no native fallback behavior beyond "always false", same as `TouchHAL`) — validated on real hardware via the `hardware-iterate` loop (serial console log + physical button press).
- End-to-end: native suite green → flash → confirm via serial log (`app home` → swipe simulated via injected touch sequence, or physical swipe → scene changes; boot press → scene goes back / exits to Home).
