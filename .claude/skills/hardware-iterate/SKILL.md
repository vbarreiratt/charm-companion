---
name: hardware-iterate
description: Use when developing or verifying a change on the Charm Companion ESP32-S3 hardware (Waveshare AMOLED 1.75C) - the build, flash, simulate-input, and camera-verify loop for this specific board. Not a general embedded-development skill.
---

# Hardware Iteration (Charm Companion / Waveshare AMOLED 1.75C)

Reusable tools for iterating on this project's real hardware, so a mission
doesn't require re-inventing serial/camera plumbing from scratch. Built
2026-09-06 after that happened once already (see
docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md).

## Tools

Setup: `pip install -r tools/hw/requirements.txt` (pyserial, opencv-python,
numpy, Pillow, pytest). These modules are flat top-level imports (no
package), so run scripts and tests with working directory at `tools/hw/`.
Additionally, `tools/hw/webtwin/build.sh` requires the Emscripten toolchain:
`brew install emscripten` (a Homebrew formula; no other setup needed beyond
that).

- `tools/hw/serial_console.py` — `connect()` for a persistent connection to
  the board's debug command console (`src/main.cpp`'s `handle_serial_command`:
  `touch <x> <y>`, `app <name>`, `next`, `prev`). Open once per session; do
  not reopen per command (CDC reset risk on this board).
  - `console.send(command, settle=0.3)` writes a newline-terminated command
    and returns the lines received within `settle` seconds.
  - `console.tail(seconds)` just collects whatever the board prints for
    that long, without sending anything — use this to watch for
    non-visual behavior (e.g. after a `touch` command) instead of
    "watching the log."
  - `next`/`prev` only work after `app scenes` has been sent first — the
    firmware refuses otherwise ("current app is not ScenesApp — try 'app
    scenes' first").
  - `connect(port=...)` defaults to `/dev/cu.usbmodem21201` but falls back
    to auto-discovering any `/dev/cu.usbmodem*` device if that exact path
    isn't present (macOS renumbers this across reconnects), raising a
    clear error if none is found. The upload command below has no such
    fallback — if the hardcoded port fails, check what's actually
    connected (e.g. `ls /dev/cu.usbmodem*`) and pass `--upload-port`
    explicitly.
- `tools/hw/render_reference.py` — `render(target, out_png, touch=, touch_x=,
  touch_y=, update_ms=)` generates a PNG from the *real* production C++
  render code (`home`, `planet`, or `eye`). This is the reference to diff a
  camera photo against. Note: it calls `on_touch()` directly on the
  app/scene object, bypassing the real event bus `publish()`/`subscribe()`
  path — so a matching reference PNG is not proof that touch-event routing
  through the bus actually works (this project already hit a real bug
  where touch events were never routed to any app); that verification
  stays the job of `pio test -e native`.
- `tools/hw/camera_probe.py` — `open_camera()` + `run_touch_capture(console,
  cam, command, out_dir)` fires a serial command and burst-captures frames
  in the same process, so fast visual events (e.g. EyeScene's 150ms blink)
  are actually catchable.
- `docs/hardware/camera-quirks.md` — check this before calling a
  reference/photo mismatch a code bug.
- `tools/hw/webtwin/` — the WASM digital twin. `build.sh` compiles the
  production render code (the real `src/` sources, not a reimplementation)
  to `dist/twin.js`/`dist/twin.wasm`; `shell.html` is the browser page that
  drives it. See the "Visual-refinement missions" section below for the
  full iteration loop.

## The autonomous hardware loop

For a mission with a concrete objective on this hardware:

1. `pio test -e native` — must stay green before touching hardware.
2. `pio run -e waveshare-amoled-175c -t upload --upload-port /dev/cu.usbmodem21201`
   — single command, builds and flashes.
3. `render_reference.render(...)` for the state the mission expects.
4. `camera_probe.run_touch_capture(...)` (or `console.tail(seconds)`, for
   non-visual behavior) to see what the real board actually does.
5. Compare. A real mismatch (not in camera-quirks.md) → fix, go to 1. A
   match → advance to the mission's next step.
6. After each completed milestone: commit locally, and write a short
   ai-memory handoff note. If the mission is running long, say so to the
   user *before* starting another expensive iteration round, rather than
   silently continuing.

Message the user again only at: full completion, a genuine blocker only
they can resolve, or a checkpoint flagging the task is running long.

## Guardrails

- Pre-authorized within this loop: native tests, building, flashing the
  known device, local commits.
- Never pre-authorized: `git push`, opening PRs — stays a manual ask.
- A camera/reference mismatch that isn't in `camera-quirks.md`: stop and
  ask, don't guess. Add the resolution to `camera-quirks.md` if it turns
  out to be a new camera artifact.

## Visual-refinement missions

When a mission is primarily about visual/design refinement, iterate on
the digital twin *before* touching hardware:

1. `tools/hw/webtwin/build.sh` — MUST be run at least once before
   `shell.html` will work at all: `dist/` is gitignored, so on a fresh
   clone (or if it was never built) there is no `dist/twin.js`/`twin.wasm`
   yet, and `shell.html` hardcodes `<script src="dist/twin.js">` — skipping
   this step produces a silently blank page, no error surfaced. After that
   first build, it only needs to be re-run after changing `src/` render
   code or `twin_main.cpp`.
2. From `tools/hw/webtwin/`, run `python3 -m http.server 8765` and open
   `http://localhost:8765/shell.html` in a browser, then tell the user it's
   ready to look at. (A bare `file://` open does NOT work — Chrome blocks
   the WASM binary's fetch from a file://-origin page.)
3. Iterate: edit `src/` render code → re-run `build.sh` → user refreshes
   the page → look again. This is the same production C++ source the
   firmware uses, so what the user approves here cannot drift from what
   ships *pixel-for-pixel, for a given state* (unlike the
   TypeScript-reimplementation approach a prior project tried and had
   trouble keeping in sync). This guarantee is about pixels, not animation
   timing — see the caveat below.
4. Once the user approves the visual, capture it as the pixel-perfect
   reference: `python3 tools/hw/render_reference.py <target> --out
   <path>.png` with whatever `--touch`/`--update-ms` reproduces the
   approved state.
5. That PNG is now the reference for the autonomous hardware loop above —
   proceed there, diffing the real board's camera capture against it.

**Caveat — pixels match, animation timing does not.** The twin matches the
real board's pixels exactly for a given state, but its `update()` call
cadence does not match the board's: the browser's `requestAnimationFrame`
loop ticks roughly every 16ms, while the real display flush takes ~57-67ms
per frame (`src/shell/hal/display_hal.cpp`), so the board calls `update(dt)`
far less often per unit of wall-clock time. For an animation driven by
*how many `update()` calls have happened* rather than by elapsed wall time,
the twin and the board will visibly disagree mid-animation even though both
run identical code. `home`'s breathing-pulse animation
(`HomeUI::render_spicy_eyes` in `src/apps/home/home_ui.cpp`) is exactly
this case: its phase comes from `frame_counter % 60` (`src/apps/home/home_app.cpp`),
where `frame_counter` increments once per `update()` call, so the twin
cycles it roughly once per second while the real board takes roughly 3.5
seconds for the same 60 calls — an approved `home` screenshot can land on a
different breathing-pulse phase than what the board shows after the same
wall-clock time (or the same `--update-ms` value, which also steps in
fixed 16ms chunks). `planet` and `eye` scenes are less affected since their
timers accumulate real `dt` in milliseconds rather than counting frames,
but that's not a blanket exemption — treat this as a `home`-specific issue
to watch for, not proof that other scenes are immune.
