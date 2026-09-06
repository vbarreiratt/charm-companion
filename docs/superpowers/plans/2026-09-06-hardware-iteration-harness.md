# Hardware Iteration Harness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Replace the ad-hoc Python scripts written during the 2026-09-06 session with reusable tools — a persistent serial console, a same-source-as-firmware reference renderer, and a synchronized touch+camera capture tool — plus a project Skill that ties them into a repeatable build→flash→simulate→observe→compare loop.

**Architecture:** Three standalone Python/C++ tools under `tools/hw/`, none of which change production firmware behavior, plus a documentation file cataloging non-code camera artifacts, plus a `.claude/skills/` playbook that references all of them by path.

**Tech Stack:** Python 3 (pyserial, opencv-python, numpy, Pillow, pytest — all already present on this machine), C++17 compiled with the system's `g++`/clang (Apple clang, confirmed present), no new PlatformIO environment.

**Spec:** `docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md`

## Global Constraints

- Python packages used: `pyserial`, `opencv-python` (`cv2`), `numpy`, `Pillow` (`PIL`), `pytest` — all confirmed already installed on this machine; no virtualenv is created, but `tools/hw/requirements.txt` documents them for the record.
- The render tool compiles against the exact same `.cpp` files under `src/` that `pio test -e native` builds (everything except `main.cpp`), with the same include paths (`-Isrc -Iconfig -I<repo root>`), so it can never silently diverge from what the native test suite already verifies.
- Real hardware device path: `/dev/cu.usbmodem21201`, baud `115200`, opened with `dsrdtr=False` (confirmed working this session; opening with default DTR/RTS handling risks a reset on this board's `ARDUINO_USB_CDC_ON_BOOT=1` config).
- `pio test -e native` (currently 60/60) must stay green throughout — it is not touched by this plan, only relied upon.

---

## File Structure

```
tools/hw/
├── requirements.txt          # documents the Python deps this plan uses
├── serial_console.py         # SerialConsole class: persistent connection, send()/tail()
├── test_serial_console.py    # tests SerialConsole against pyserial's loop:// virtual port
├── render_reference.cpp      # host-side CLI: real App/Scene -> real Canvas -> raw RGB565 dump
├── render_reference.py       # build/run wrapper + RGB565->PNG conversion
├── test_render_reference.py  # tests render_reference.py end-to-end, asserting known pixel colors
├── camera_probe.py           # OpenCV camera + SerialConsole: synchronized touch+capture burst
└── test_camera_probe.py      # tests the pure scheduling/naming logic (no real camera/serial needed)

docs/hardware/
└── camera-quirks.md          # catalog of camera artifacts that are not code bugs

.claude/skills/hardware-iterate/
└── SKILL.md                  # the playbook referencing the above tools
```

---

### Task 1: `serial_console.py` — persistent serial connection

**Files:**
- Create: `tools/hw/requirements.txt`
- Create: `tools/hw/serial_console.py`
- Test: `tools/hw/test_serial_console.py`

**Interfaces:**
- Produces: `SerialConsole(serial_obj, read_timeout=0.05)` with methods `.send(command, settle=0.3) -> list[str]`, `.tail(seconds) -> list[str]`, `.close()`, and context-manager support. Also `connect(port="/dev/cu.usbmodem21201", baud=115200) -> SerialConsole` for real hardware use.
- Consumes: nothing from earlier tasks (this is the first task).

- [x] **Step 1: Create `tools/hw/requirements.txt`**

```
pyserial
opencv-python
numpy
Pillow
pytest
```

- [x] **Step 2: Write the failing tests**

Create `tools/hw/test_serial_console.py`:

```python
import serial

from serial_console import SerialConsole


def test_send_writes_newline_terminated_command_and_returns_echo():
    ser = serial.serial_for_url("loop://", timeout=0.05)
    console = SerialConsole(ser)

    lines = console.send("touch 100 200", settle=0.05)

    assert lines == ["touch 100 200"]


def test_tail_collects_multiple_pending_lines():
    ser = serial.serial_for_url("loop://", timeout=0.05)
    console = SerialConsole(ser)
    ser.write(b"line one\nline two\n")
    ser.flush()

    lines = console.tail(0.05)

    assert lines == ["line one", "line two"]


def test_context_manager_closes_underlying_serial():
    ser = serial.serial_for_url("loop://", timeout=0.05)
    with SerialConsole(ser) as console:
        console.send("ping", settle=0.02)

    assert not ser.is_open
```

- [x] **Step 2: Run tests to verify they fail**

Run: `cd tools/hw && python3 -m pytest test_serial_console.py -v`
Expected: FAIL with `ModuleNotFoundError: No module named 'serial_console'` (the module doesn't exist yet).

- [x] **Step 3: Implement `tools/hw/serial_console.py`**

```python
"""Persistent serial connection to the ESP32-S3's debug command console
(see src/main.cpp's handle_serial_command: "touch <x> <y>", "app <name>",
"next", "prev").

One connection is opened and reused for a whole session instead of being
reopened per command. This board is configured with
ARDUINO_USB_CDC_ON_BOOT=1; reopening the serial port risks toggling
DTR/RTS and resetting the board mid-session, which happened during manual
testing on 2026-09-06. connect() below opens with dsrdtr=False to avoid
that, matching what was confirmed to work that session.
"""
import time


class SerialConsole:
    def __init__(self, serial_obj, read_timeout=0.05):
        self._ser = serial_obj
        self._ser.timeout = read_timeout

    def send(self, command, settle=0.3):
        self._ser.write((command + "\n").encode())
        self._ser.flush()
        return self.tail(settle)

    def tail(self, seconds):
        end = time.time() + seconds
        lines = []
        while time.time() < end:
            line = self._ser.readline()
            if line:
                lines.append(line.decode(errors="replace").rstrip())
        return lines

    def close(self):
        self._ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()


def connect(port="/dev/cu.usbmodem21201", baud=115200):
    import serial

    ser = serial.Serial(port, baud, timeout=0.05, dsrdtr=False)
    return SerialConsole(ser)
```

- [x] **Step 4: Run tests to verify they pass**

Run: `cd tools/hw && python3 -m pytest test_serial_console.py -v`
Expected: 3 passed.

- [x] **Step 5: Commit**

```bash
git add tools/hw/requirements.txt tools/hw/serial_console.py tools/hw/test_serial_console.py
git commit -m "feat(hw-tools): add persistent serial console harness

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```

---

### Task 2: `render_reference` — same-source digital-twin PNG renderer

**Files:**
- Create: `tools/hw/render_reference.cpp`
- Create: `tools/hw/render_reference.py`
- Test: `tools/hw/test_render_reference.py`

**Interfaces:**
- Consumes: nothing from Task 1.
- Produces: `render_reference.render(target: str, out_png: str, touch=False, update_ms=0, force_build=False) -> str` (returns `out_png`), used by later tasks and by future missions to generate the "expected" reference image. `target` is one of `"home"`, `"planet"`, `"eye"`.

- [x] **Step 1: Write the failing tests**

Create `tools/hw/test_render_reference.py`:

```python
import render_reference as rr


def test_eye_pupil_is_black_when_not_blinking(tmp_path):
    out = tmp_path / "eye_open.png"

    rr.render("eye", str(out))

    img = rr.Image.open(out)
    # Matches tests/test_eye_scene.cpp EyeSceneTest.IrisVisibleWhenNotBlinking:
    # the pupil at dead center (233, 233) is COLOR_BG_BLACK (0x0000).
    assert img.getpixel((233, 233)) == (0, 0, 0)


def test_eye_blink_closes_eyelid_after_touch_and_150ms(tmp_path):
    out = tmp_path / "eye_blink.png"

    rr.render("eye", str(out), touch=True, update_ms=160)

    img = rr.Image.open(out)
    # Matches tests/test_eye_scene.cpp EyeSceneTest.BlinkFullyClosesEyelid:
    # past blink_duration (150ms), (263, 233) is inside the eyelid bar
    # (COLOR_BG_BLACK), not iris color.
    assert img.getpixel((263, 233)) == (0, 0, 0)


def test_home_sclera_matches_mono_neutral_rgb565_conversion(tmp_path):
    out = tmp_path / "home.png"

    rr.render("home", str(out))

    img = rr.Image.open(out)
    # (170, 155) is within HomeUI's left-eye sclera (center 170,190 radius 40)
    # but outside the iris (radius 18), so it's pure COLOR_MONO_NEUTRAL
    # (0xE73C). Converting RGB565->RGB888 by hand: R5=28->230, G6=57->231,
    # B5=28->230. A wrong bit-width split (e.g. swapping the 5/6-bit
    # channels) would produce a different triple here.
    assert img.getpixel((170, 155)) == (230, 231, 230)
```

- [x] **Step 2: Run tests to verify they fail**

Run: `cd tools/hw && python3 -m pytest test_render_reference.py -v`
Expected: FAIL with `ModuleNotFoundError: No module named 'render_reference'`.

- [x] **Step 3: Implement `tools/hw/render_reference.cpp`**

```cpp
// tools/hw/render_reference.cpp
//
// Host-side reference renderer: instantiates the real production App/Scene
// classes and calls their real render() against a real Canvas, then dumps
// the raw RGB565 framebuffer. This produces "what the code says should be
// on screen" from the exact same source that ships to the ESP32 firmware --
// not a hand-ported reimplementation. See docs/superpowers/specs/
// 2026-09-06-hardware-iteration-harness-design.md.
//
// Usage:
//   render_reference <home|planet|eye> --out <path.raw> [--touch] [--update-ms N]

#include "apps/home/home_app.h"
#include "apps/scenes/planet_scene.h"
#include "apps/scenes/eye_scene.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"
#include "config/pin_config.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {

void write_buffer(const char* path, Canvas& canvas) {
    FILE* f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "render_reference: could not open '%s' for writing\n", path);
        exit(2);
    }
    fwrite(canvas.get_buffer(), 1, canvas.buffer_size_bytes(), f);
    fclose(f);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: render_reference <home|planet|eye> --out <path> [--touch] [--update-ms N]\n");
        return 1;
    }

    const char* target = argv[1];
    const char* out_path = nullptr;
    bool touch = false;
    uint32_t update_ms = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(argv[i], "--touch") == 0) {
            touch = true;
        } else if (strcmp(argv[i], "--update-ms") == 0 && i + 1 < argc) {
            update_ms = static_cast<uint32_t>(atoi(argv[++i]));
        } else {
            fprintf(stderr, "render_reference: unrecognized argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (!out_path) {
        fprintf(stderr, "render_reference: --out <path> is required\n");
        return 1;
    }

    Canvas canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    TouchEvent te{0, 0, 0, 0};

    if (strcmp(target, "home") == 0) {
        HomeApp app;
        app.on_enter();
        if (touch) app.on_touch(te);
        uint32_t remaining = update_ms;
        while (remaining > 0) {
            uint32_t step = remaining < 16 ? remaining : 16;
            app.update(step);
            remaining -= step;
        }
        app.render(&canvas);
    } else if (strcmp(target, "planet") == 0) {
        PlanetScene scene;
        scene.on_enter();
        if (touch) scene.on_touch(te);
        uint32_t remaining = update_ms;
        while (remaining > 0) {
            uint32_t step = remaining < 16 ? remaining : 16;
            scene.update(step);
            remaining -= step;
        }
        auto ctx = g_personality_api.get_context();
        scene.render(&canvas, ctx);
    } else if (strcmp(target, "eye") == 0) {
        EyeScene scene;
        scene.on_enter();
        if (touch) scene.on_touch(te);
        uint32_t remaining = update_ms;
        while (remaining > 0) {
            uint32_t step = remaining < 16 ? remaining : 16;
            scene.update(step);
            remaining -= step;
        }
        auto ctx = g_personality_api.get_context();
        scene.render(&canvas, ctx);
    } else {
        fprintf(stderr, "render_reference: unknown target '%s' (expected home|planet|eye)\n", target);
        return 1;
    }

    write_buffer(out_path, canvas);
    return 0;
}
```

- [x] **Step 4: Implement `tools/hw/render_reference.py`**

```python
#!/usr/bin/env python3
"""Build (cached by mtime) and run render_reference.cpp against the real
production C++ source, then convert its raw RGB565 dump to a PNG.

See docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md.
"""
import argparse
import glob
import os
import subprocess
import sys

import numpy as np
from PIL import Image

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOL_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_PATH = os.path.join(TOOL_DIR, "render_reference_bin")

DISPLAY_WIDTH = 466
DISPLAY_HEIGHT = 466


def _source_files():
    files = glob.glob(os.path.join(REPO_ROOT, "src", "**", "*.cpp"), recursive=True)
    return [f for f in files if os.path.basename(f) != "main.cpp"]


def build(force=False):
    sources = _source_files() + [os.path.join(TOOL_DIR, "render_reference.cpp")]
    newest_source = max(os.path.getmtime(f) for f in sources)
    if not force and os.path.exists(BIN_PATH) and os.path.getmtime(BIN_PATH) > newest_source:
        return
    cmd = [
        "g++", "-std=c++17", "-O1",
        "-I", os.path.join(REPO_ROOT, "src"),
        "-I", os.path.join(REPO_ROOT, "config"),
        "-I", REPO_ROOT,
        *sources,
        "-o", BIN_PATH,
    ]
    subprocess.run(cmd, check=True)


def rgb565_to_rgb888(raw_bytes):
    values = np.frombuffer(raw_bytes, dtype="<u2").reshape(DISPLAY_HEIGHT, DISPLAY_WIDTH)
    r5 = (values >> 11) & 0x1F
    g6 = (values >> 5) & 0x3F
    b5 = values & 0x1F
    r8 = (r5 * 255 + 15) // 31
    g8 = (g6 * 255 + 31) // 63
    b8 = (b5 * 255 + 15) // 31
    return np.dstack([r8, g8, b8]).astype(np.uint8)


def render(target, out_png, touch=False, update_ms=0, force_build=False):
    build(force=force_build)
    raw_path = out_png + ".raw"
    cmd = [BIN_PATH, target, "--out", raw_path]
    if touch:
        cmd.append("--touch")
    if update_ms:
        cmd += ["--update-ms", str(update_ms)]
    subprocess.run(cmd, check=True)
    with open(raw_path, "rb") as f:
        raw = f.read()
    os.remove(raw_path)
    rgb = rgb565_to_rgb888(raw)
    Image.fromarray(rgb, mode="RGB").save(out_png)
    return out_png


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", choices=["home", "planet", "eye"])
    parser.add_argument("--out", required=True, help="output PNG path")
    parser.add_argument("--touch", action="store_true")
    parser.add_argument("--update-ms", type=int, default=0)
    parser.add_argument("--force-build", action="store_true")
    args = parser.parse_args()
    path = render(args.target, args.out, touch=args.touch, update_ms=args.update_ms, force_build=args.force_build)
    print(f"wrote {path}")


if __name__ == "__main__":
    sys.exit(main())
```

- [x] **Step 5: Run tests to verify they pass**

Run: `cd tools/hw && python3 -m pytest test_render_reference.py -v`
Expected: 3 passed. (First run compiles `render_reference_bin`, which takes a few seconds; later runs are cached by mtime.)

- [x] **Step 6: Add the compiled binary to `.gitignore`**

Append to the repo's `.gitignore` (create it if it doesn't exist at the repo root, checking first with `cat .gitignore` — if one exists, append rather than overwrite):

```
tools/hw/render_reference_bin
tools/hw/webtwin/dist/
```

- [x] **Step 7: Commit**

```bash
git add tools/hw/render_reference.cpp tools/hw/render_reference.py tools/hw/test_render_reference.py .gitignore
git commit -m "feat(hw-tools): add same-source digital-twin PNG renderer

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```

---

### Task 3: `camera_probe.py` — synchronized touch + camera burst capture

**Files:**
- Create: `tools/hw/camera_probe.py`
- Test: `tools/hw/test_camera_probe.py`

**Interfaces:**
- Consumes: `SerialConsole` from Task 1 (specifically its `._ser` attribute, to write directly without waiting on `tail()`'s settle delay before starting the capture burst).
- Produces: `capture_schedule(duration_s, interval_s) -> list[float]`, `frame_filename(out_dir, index, t) -> str`, `open_camera(device_index=0)`, `run_touch_capture(console, cam, command, out_dir, duration_s=1.5, interval_s=0.03) -> list[str]` (paths of saved frames), used by future missions and by Task 5's end-to-end smoke test.

- [x] **Step 1: Write the failing tests**

Create `tools/hw/test_camera_probe.py`:

```python
from camera_probe import capture_schedule, frame_filename


def test_capture_schedule_spans_duration_at_fixed_interval():
    schedule = capture_schedule(0.1, 0.03)

    assert schedule == [0.0, 0.03, 0.06, 0.09]


def test_frame_filename_encodes_index_and_timestamp():
    path = frame_filename("/tmp/out", 3, 0.075)

    assert path == "/tmp/out/frame_003_t0.075.png"
```

- [x] **Step 2: Run tests to verify they fail**

Run: `cd tools/hw && python3 -m pytest test_camera_probe.py -v`
Expected: FAIL with `ModuleNotFoundError: No module named 'camera_probe'`.

- [x] **Step 3: Implement `tools/hw/camera_probe.py`**

```python
"""Synchronized touch-injection + camera burst capture.

Solves the timing problem from the 2026-09-06 session: the MCP camera
tool's network round-trip (>150ms) was too slow to catch EyeScene's
150ms blink. This opens the camera directly via OpenCV (confirmed
locally to work, bypassing the MCP tool entirely) and holds the serial
connection open at the same time, so the touch command and the first
capture happen back-to-back in one process, not across separate tool
calls.

Before treating a captured frame as evidence of a code bug, check
docs/hardware/camera-quirks.md for known non-code artifacts (mirroring,
the camera's own reflected indicator LED, exposure extremes).
"""
import os
import time

import cv2


def capture_schedule(duration_s, interval_s):
    """Relative timestamps (seconds from t=0) at which to grab a frame."""
    schedule = []
    t = 0.0
    while t < duration_s:
        schedule.append(round(t, 4))
        t += interval_s
    return schedule


def frame_filename(out_dir, index, t):
    return os.path.join(out_dir, f"frame_{index:03d}_t{t:.3f}.png")


def open_camera(device_index=0):
    cam = cv2.VideoCapture(device_index)
    if not cam.isOpened():
        raise RuntimeError(f"could not open camera at index {device_index}")
    return cam


def run_touch_capture(console, cam, command, out_dir, duration_s=1.5, interval_s=0.03):
    """Send `command` over `console` and burst-capture frames from `cam`
    for `duration_s`, roughly every `interval_s`. Returns the saved frame
    paths, in order."""
    os.makedirs(out_dir, exist_ok=True)
    schedule = capture_schedule(duration_s, interval_s)

    start = time.monotonic()
    console._ser.write((command + "\n").encode())
    console._ser.flush()

    paths = []
    for i, target_t in enumerate(schedule):
        while (time.monotonic() - start) < target_t:
            pass
        ok, frame = cam.read()
        if not ok:
            continue
        path = frame_filename(out_dir, i, time.monotonic() - start)
        cv2.imwrite(path, frame)
        paths.append(path)
    return paths
```

- [x] **Step 4: Run tests to verify they pass**

Run: `cd tools/hw && python3 -m pytest test_camera_probe.py -v`
Expected: 2 passed.

- [x] **Step 5: Commit**

```bash
git add tools/hw/camera_probe.py tools/hw/test_camera_probe.py
git commit -m "feat(hw-tools): add synchronized touch+camera burst capture

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```

---

### Task 4: Camera-quirks catalog

**Files:**
- Create: `docs/hardware/camera-quirks.md`

**Interfaces:**
- Consumes: nothing.
- Produces: a document referenced by `camera_probe.py`'s docstring (Task 3) and by `SKILL.md` (Task 5) as the thing to check before calling a visual mismatch a code bug.

- [x] **Step 1: Write `docs/hardware/camera-quirks.md`**

```markdown
# Camera Quirks — Not Code Bugs

Observed artifacts when photographing the ESP32-S3 board with the EMEET
SmartCam C960 4K via OpenCV/MCP videocapture, that are camera behavior,
not firmware behavior. Check this list before concluding a captured
frame shows a rendering bug.

## Mirrored image
The frame may appear horizontally flipped relative to how you're looking
at the board. This is the camera/capture pipeline's default orientation,
not a `TFT_ROTATION`/display bug. If left/right positions look swapped,
check this before touching display rotation config.

## Blue reflection point
A small blue point of light can appear reflected in the AMOLED glass.
This is the camera module's own indicator LED reflecting back, not a
pixel the firmware drew. It moves with the camera's position/angle, not
with anything the app is rendering.

## Too dark / too bright / blown-out colors
Auto-exposure varies a lot with ambient light. A color that looks
different from `render_reference`'s output can be an exposure artifact
rather than a wrong color constant — before suspecting `color_utils.h`,
try comparing hue/shape rather than absolute brightness, or take another
frame under more consistent lighting.

## Adding to this list
If a session hits a new camera-only artifact (something that reproduces
regardless of firmware state), add it here with what it looks like and
how to tell it apart from a real bug, so the next session doesn't
re-diagnose it from scratch.
```

- [x] **Step 2: Commit**

```bash
git add docs/hardware/camera-quirks.md
git commit -m "docs: catalog known camera artifacts vs real rendering bugs

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```

---

### Task 5: `hardware-iterate` Skill (v1 — non-visual autonomous loop)

**Files:**
- Create: `.claude/skills/hardware-iterate/SKILL.md`

**Interfaces:**
- Consumes: `tools/hw/serial_console.py`, `tools/hw/render_reference.py`, `tools/hw/camera_probe.py` (Tasks 1-3), `docs/hardware/camera-quirks.md` (Task 4).
- Produces: the invokable skill itself. (Plan 2, the WASM digital twin, will extend this file with a visual-refinement pre-hardware loop section — noted inline below so that extension point is explicit, not a placeholder for missing content.)

- [x] **Step 1: Write `.claude/skills/hardware-iterate/SKILL.md`**

```markdown
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

- `tools/hw/serial_console.py` — `connect()` for a persistent connection to
  the board's debug command console (`src/main.cpp`'s `handle_serial_command`:
  `touch <x> <y>`, `app <name>`, `next`, `prev`). Open once per session; do
  not reopen per command (CDC reset risk on this board).
- `tools/hw/render_reference.py` — `render(target, out_png, touch=, update_ms=)`
  generates a PNG from the *real* production C++ render code (`home`,
  `planet`, or `eye`). This is the reference to diff a camera photo against.
- `tools/hw/camera_probe.py` — `open_camera()` + `run_touch_capture(console,
  cam, command, out_dir)` fires a serial command and burst-captures frames
  in the same process, so fast visual events (e.g. EyeScene's 150ms blink)
  are actually catchable.
- `docs/hardware/camera-quirks.md` — check this before calling a
  reference/photo mismatch a code bug.

## The autonomous hardware loop

For a mission with a concrete objective on this hardware:

1. `pio test -e native` — must stay green before touching hardware.
2. `pio run -e waveshare-amoled-175c -t upload --upload-port /dev/cu.usbmodem21201`
   — single command, builds and flashes.
3. `render_reference.render(...)` for the state the mission expects.
4. `camera_probe.run_touch_capture(...)` (or just watch `serial_console`'s
   log, for non-visual behavior) to see what the real board actually does.
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

*(v1 scope: this skill currently only covers the loop above. A mission
that is primarily about visual/design refinement — not yet wired to a
digital twin the user can watch live — should still use
`render_reference.py` to check work, but expect the WASM digital twin
described in `docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md`
to add a proper pre-hardware co-iteration loop here in a follow-up plan.)*
```

- [x] **Step 2: Commit**

```bash
git add .claude/skills/hardware-iterate/SKILL.md
git commit -m "feat: add hardware-iterate skill (v1, non-visual loop)

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```

---

### Task 6: End-to-end hardware smoke test

**Files:**
- None created — this task exercises Tasks 1-5 together against the real board.

**Interfaces:**
- Consumes: everything from Tasks 1-5.
- Produces: a recorded pass/fail result for the harness as a whole (this is the plan's actual "does it work" gate — the earlier tasks' automated tests each cover one piece in isolation, but none of them touch real hardware together).

- [x] **Step 1: Confirm the board is flashed with the current firmware**

Run: `pio run -e waveshare-amoled-175c -t upload --upload-port /dev/cu.usbmodem21201`
Expected: `SUCCESS`. (The debug console in `src/main.cpp` was already added and flashed on 2026-09-06; this just re-confirms it's current.)

- [x] **Step 2: Generate the reference image for the touched, fully-blinked eye**

Run:
```bash
python3 tools/hw/render_reference.py eye --touch --update-ms 160 --out /tmp/ref_eye_blink.png
```
Expected: prints `wrote /tmp/ref_eye_blink.png`. Read the file to confirm it shows a closed eyelid bar (matches `EyeSceneTest.BlinkFullyClosesEyelid`).

- [x] **Step 3: Run the synchronized capture against the real board**

This is a manual verification run, not a new committed tool — run it as a one-off script from `tools/hw/` (so `serial_console`/`camera_probe` import directly):

```bash
cd tools/hw
python3 -c "
import serial_console
import camera_probe

console = serial_console.connect()
print(console.send('app scenes'))
# Registry order is alphabetical ('eye' < 'planet'), so entering scenes
# starts on eye already; 'next'/'prev' as a fallback if the log says otherwise.
lines = console.send('next')
if 'scene now: eye' not in '\n'.join(lines):
    lines = console.send('prev')
print(lines)

cam = camera_probe.open_camera()
paths = camera_probe.run_touch_capture(
    console, cam, 'touch 233 233', '/tmp/eye_burst',
    duration_s=1.0, interval_s=0.03,
)
print('captured frames:')
for p in paths:
    print(' ', p)
"
```

Expected: at least one saved frame's timestamp falls within the ~150ms blink window (i.e. some path's `t` is between roughly 0.0 and 0.2).

- [x] **Step 4: Visually compare a burst frame near the blink window to the reference PNG**

Read (view) the frame closest to `t≈0.075` from Step 3's output alongside `/tmp/ref_eye_blink.png` from Step 2. Confirm they show the same shape (closed/near-closed eyelid), accounting for anything listed in `docs/hardware/camera-quirks.md`.

Expected outcome: a real match, OR a documented mismatch that is either (a) added to `camera-quirks.md` if it's a new camera-only artifact, or (b) a real bug to fix before this task can be considered done (in which case, fix it and re-run from Step 1).

- [x] **Step 5: Record the result**

No code change from this task alone (unless Step 4 found a real bug). If everything matched, note it in the next commit message or handoff; there is nothing to commit for this task by itself.
