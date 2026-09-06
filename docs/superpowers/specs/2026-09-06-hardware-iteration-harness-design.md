# Hardware Iteration Harness: Design

**Date:** 2026-09-06
**Status:** Design Phase
**Depends on:** 2026-09-05-charm-companion-architecture.md (Shell/EventBus/Canvas), Phase 1 hardware completion plans

---

## Executive Summary

Phase 1 hardware completion (2026-09-05) took ~4h of session time for three plans + subagent-driven-development + hardware flashing, and still left visual correctness unverified — the camera couldn't be reached that session, and the only "proof" was a boot log. The following session (2026-09-06) found and fixed a real bug (touch/motion events never reached any app because `HomeApp`/`ScenesApp` never subscribed to those event types) and built an ad-hoc serial command console and one-off Python scripts to validate on real hardware. That worked, but every tool was invented fresh, mid-task, with no reusable shape.

This spec designs a **reusable harness + a project Skill** so that future sessions working on this hardware:
1. Don't re-invent serial/camera plumbing each time.
2. Get a **pixel-perfect visual reference** to iterate against, generated from the *same C++ source* that ships to the ESP32 — not a hand-ported reimplementation in another language (see "Lesson from esp32-badge" below).
3. Can iterate autonomously (build → flash → simulate input → observe → compare → fix → repeat) once a mission is agreed, checking in only at natural checkpoints or genuine blockers.

### Lesson from esp32-badge

A prior project (`/Users/vitor/Desktop/esp32-badge`) built a "Digital Twin Simulator" as a **separate TypeScript/Vite reimplementation** of the firmware's UI (`dial_menu.ts`, `bruce_menu.ts` mirroring `main_menu.cpp`). The two implementations had to be kept in sync by hand, and the project's own handover notes record difficulty getting the simulator to visually match the real board. This is the single biggest thing this design must avoid: **the digital twin must run the actual production C++ source**, compiled to a second target (WebAssembly), not a parallel rewrite.

This repo is already structured to make that possible: `Canvas`, `Scene`, `App`, `EventBus`, and all scene/app logic are plain C++17 with `#if defined(ARDUINO)` guards isolating the only hardware-specific code. This is exactly why the native PlatformIO test environment already works today (60/60 tests, no Arduino framework involved).

---

## Architecture

### New components

```
tools/hw/
├── serial_console.py       # persistent pyserial connection + command API
├── camera_probe.py         # OpenCV camera opened directly (no MCP round-trip)
│                            #   + serial_console, for time-synchronized
│                            #   "inject touch, burst-capture frames" runs
├── render_reference.cpp    # host-side: instantiate a real App/Scene, call
│                            #   render() on a real Canvas, dump the RGB565
│                            #   framebuffer
└── render_reference.py     # numpy/PIL: RGB565 dump -> PNG

tools/hw/webtwin/            # Emscripten build of the same source, for the
├── (build glue)             #   pre-hardware visual-iteration loop (see below)
└── shell.html

docs/hardware/
└── camera-quirks.md         # catalog of camera artifacts that are NOT code bugs

.claude/skills/hardware-iterate/
└── SKILL.md                 # the playbook tying all of the above together
```

Existing pieces this builds on, unchanged:
- `src/main.cpp`'s serial command console (`touch <x> <y>`, `app <name>`, `next`/`prev`) — already built and flashed today.
- `Shell::poll_sensors()`'s raw touch-point logging — already built today.
- `pio test -e native` (60 tests today) — unchanged; still the first gate before anything touches hardware.

### Component responsibilities

**`serial_console.py`** — one persistent `pyserial` connection (`dsrdtr=False`, matching what worked today; opening once and reusing avoids the ESP32-S3's CDC reset-on-reopen risk noted by the advisor this session). Exposes `send(cmd)` and `tail(seconds)`. Every other script imports this instead of hand-rolling `serial.Serial(...)` each time.

**`render_reference.{cpp,py}`** — a small host-side program, compiled the same way `pio test -e native` compiles (`-Isrc -Iconfig -std=c++17`, no Arduino), that takes a scene/app name (and optional simulated input state, e.g. "mid-blink") on the command line, constructs the real object, calls the real `render()` against a real `Canvas`, and writes the raw RGB565 buffer to disk. A thin Python step (numpy + PIL, both already present on this machine) converts that to a PNG. This PNG is the reference — "what the code says should be on screen" — generated from the exact same source as firmware, not redescribed by hand.

**`camera_probe.py`** — opens the camera directly via `cv2.VideoCapture(0)` (confirmed working today, bypassing the MCP videocapture tool's network round-trip entirely) and holds `serial_console`'s connection open at the same time. On trigger, sends a serial command (e.g. `touch 233 233`) and immediately captures a burst of frames at short intervals (e.g. every ~20-30ms for ~1-2s), each saved to disk with a timestamp. This is what makes it possible to actually catch something like `EyeScene`'s 150ms blink — the MCP tool's round-trip today was too slow for that, purely a latency problem, not a design problem with the blink itself.

**`docs/hardware/camera-quirks.md`** — a living list of "the photo looks wrong but the code is fine" cases, seeded with what was already observed:
- Image may appear mirrored (camera default, not a display rotation bug)
- A small blue reflection point can appear (the camera module's own indicator LED reflecting off the AMOLED glass) — not a rendering artifact
- Frames can be too dark or blown out depending on ambient light and the camera's auto-exposure — before concluding a color is wrong, check exposure first
This file is consulted *before* a camera/reference mismatch is treated as a code bug.

**`tools/hw/webtwin/`** — an Emscripten build of the same `Canvas`/`Scene`/`App`/`EventBus` sources (same include paths as the native test env), driving an HTML `<canvas>` in a browser tab. A small JS shim blits the RGB565 buffer to the canvas each frame and forwards mouse clicks as `TouchEvent`s into the same `on_touch()` path physical touch uses. This is the live, interactive surface for visual-refinement work — the user watches it update in the browser as code changes, the same way they watched the old Vite simulator, but this time it cannot drift from the firmware because it *is* the firmware's rendering code. Requires installing an Emscripten toolchain (`emcc`), not currently present on this machine — a one-time setup step in the implementation plan.

**`.claude/skills/hardware-iterate/SKILL.md`** — the playbook (see Data Flow below). References the other tools by path/command rather than re-describing them.

---

## Data Flow: the iteration loop

```
Mission (visual refs / prompt / briefing)
  → I understand it, ask a small number of targeted questions (lightweight brainstorm)
  → I present 2-3 approaches, you pick
  → I write a short spec for the record (not a gate — I don't wait for you to review it)
  → [only for visual-refinement missions] webtwin loop:
        build WASM target → open in browser → you watch it update as I iterate
        → you approve the visual → THAT approved state becomes the pixel-perfect
          reference (captured via render_reference into a PNG)
  → autonomous hardware loop, repeated until the mission's objective is met:
        1. pio test -e native            (must stay green)
        2. pio run -e waveshare-amoled-175c -t upload   (single command, real board)
        3. render_reference → expected PNG for the current step
        4. camera_probe: inject input via serial_console, burst-capture frames
        5. compare captured frame(s) to the reference PNG, filtering known
           camera-quirks.md artifacts first
        6. match → advance to the next step; real mismatch → fix and go to 1
  → checkpoint at each completed milestone: local commit + a short ai-memory
    handoff note (see Guardrails)
  → I message you again only at: full completion, a genuine blocker only you
    can resolve, or a checkpoint that flags the task is running long
```

For missions that are purely logic/behavior (no visual refinement — e.g. today's touch-routing bug), the webtwin step is skipped; the loop starts directly at the autonomous hardware loop, same as it ran today (minus the ad-hoc scripts, using the reusable tools instead).

---

## Guardrails

- **Pre-authorized within this loop:** running native tests, building and flashing firmware to the known device (`/dev/cu.usbmodem21201` or whatever is currently attached), local git commits. These are reversible/local, matching how today's session already operated.
- **Never pre-authorized:** `git push`, opening PRs, anything touching a remote — stays a manual ask, same as the project's general working agreement.
- **Camera/reference mismatch that isn't a cataloged quirk:** stop and ask. Don't guess between "my code is wrong" and "unknown camera behavior" — that's exactly the ambiguity a human should resolve, and it's also a chance to grow `camera-quirks.md`.
- **Token/context budget:** there is no direct token-counter tool available. The safeguard is procedural: checkpoint (commit + a short ai-memory handoff) after each completed milestone, and if a mission is running long, proactively message the user with a heads-up *before* starting another expensive iteration round rather than silently continuing until forced to stop mid-thought.

---

## Testing / validating the harness itself

- `render_reference` is exercised the same way the app code already is: it links the exact same `.cpp` files `pio test -e native` builds, so any test already covering `Canvas`/scenes covers it transitively. No separate test suite needed beyond a smoke check that it produces a non-empty PNG for each registered scene/app.
- `serial_console.py` / `camera_probe.py` are dev tooling, not shipped firmware — validated by using them for a real task, not by a formal test suite.
- The harness as a whole is considered validated by dogfooding: the next real feature request (touch-based navigation between Home and Scenes, already requested and pending) is the first mission run through it end-to-end. If that mission completes through the full loop without falling back to one-off scripts, the harness has done its job.

---

## Implementation sequencing

This design bundles two separable capabilities. Recommend two implementation plans, built in this order:

1. **Hardware-in-the-loop harness** — `serial_console.py`, `camera_probe.py`, `render_reference.{cpp,py}`, `camera-quirks.md`, and a first version of `SKILL.md` covering the non-visual autonomous loop. This is immediately useful on its own (it's what today's ad-hoc scripts already proved out, just made reusable) and is what the pending touch-navigation feature can dogfood right away.
2. **WASM digital twin** — Emscripten setup, `webtwin/` build, browser shim, and the `SKILL.md` extension adding the pre-hardware visual-iteration loop. Depends on nothing from plan 1 except that `render_reference`'s PNG format becomes the twin's approved-state export target.

Building 1 before 2 means the touch-navigation feature doesn't have to wait on an unfamiliar toolchain (Emscripten) landing successfully.

## Open implementation dependencies

- Emscripten toolchain (`emcc`) is not installed on this machine; installing it is part of the implementation plan, not this design.
- `render_reference`'s exact CLI shape (how a scene/app + an input state like "mid-blink at 75ms" is specified) is left to the implementation plan.
- Where exactly `webtwin`'s browser shim lives (a plain local HTML file opened directly, or served) is an implementation detail, not a design constraint — either works since no capability beyond `<canvas>` + mouse events is needed.
