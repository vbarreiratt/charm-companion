# WASM Digital Twin Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Compile the same production `Canvas`/`App`/`Scene`/`EventBus` C++ source (not a reimplementation) to WebAssembly, driving a browser `<canvas>` the user can watch update live and click on — replacing the failed 2026-09-04 esp32-badge approach (a hand-ported TypeScript UI that drifted from the C++ firmware) with a twin that cannot drift, because it's the same binary source compiled to a second target.

**Architecture:** One new C++ file (`twin_main.cpp`) exposing a small `extern "C"` API (select target, inject touch, tick one frame, read the framebuffer), compiled with Emscripten against the existing `src/` tree; one static HTML+JS page that drives it via `requestAnimationFrame` and forwards mouse clicks.

**Tech Stack:** Emscripten (`emcc`, installed via Homebrew), the existing C++17 `src/` tree (no changes to it), vanilla JS/HTML (no build step, no npm — deliberately avoiding the prior project's Vite/TypeScript pipeline).

**Spec:** `docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md`

**Depends on:** `docs/superpowers/plans/2026-09-06-hardware-iteration-harness.md` only for the `.gitignore` entry it adds (`tools/hw/webtwin/dist/`) — otherwise independent.

## Global Constraints

- Emscripten is not installed on this machine as of 2026-09-06; Task 1 installs it via Homebrew (`brew install emscripten`), confirmed available as a bottled formula.
- No `ALLOW_MEMORY_GROWTH` — the framebuffer is a small fixed allocation (466×466×2 bytes ≈ 434 KB); avoiding memory growth avoids the well-known Emscripten pitfall where JS typed-array views become detached after the heap grows.
- No npm/Vite/bundler — this is deliberately a single static HTML file plus the Emscripten-generated `.js`/`.wasm`, so there is exactly one toolchain (Emscripten) between "C++ source" and "what's on screen," not two.
- `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` are both `466` (`config/pin_config.h`) — the canvas element must match.

---

## File Structure

```
tools/hw/webtwin/
├── twin_main.cpp     # extern "C" API wrapping HomeApp/PlanetScene/EyeScene
├── build.sh           # the emcc invocation (documented once, run repeatably)
├── shell.html          # the browser page: canvas, buttons, JS glue
└── dist/               # emcc output (twin.js, twin.wasm) - gitignored
```

---

### Task 1: Install and verify Emscripten

**Files:** none.

**Interfaces:** produces a working `emcc` on `PATH` for Task 2 onward.

- [x] **Step 1: Install via Homebrew**

Run: `brew install emscripten`
Expected: completes successfully (it's a bottled formula — confirmed available via `brew info emscripten` on 2026-09-06).

- [x] **Step 2: Verify the compiler runs**

Run: `emcc --version`
Expected: prints an `emcc (Emscripten gcc/clang-like replacement)` version line.

- [x] **Step 3: Compile and run a trivial smoke program**

```bash
cat > /tmp/twin_smoke.cpp << 'EOF'
#include <cstdio>
int main() { printf("emscripten ok\n"); return 0; }
EOF
emcc /tmp/twin_smoke.cpp -o /tmp/twin_smoke.js
node /tmp/twin_smoke.js
```
Expected: prints `emscripten ok`. (This confirms both the C++→WASM compile step and that the generated JS runs standalone under `node`, before we add any of our own code.)

- [x] **Step 4: No commit for this task** (nothing in the repo changed — this only verifies the local toolchain).

---

### Task 2: `twin_main.cpp` — compile the production source to WASM

**Files:**
- Create: `tools/hw/webtwin/twin_main.cpp`
- Create: `tools/hw/webtwin/build.sh`

**Interfaces:**
- Consumes: `HomeApp`, `PlanetScene`, `EyeScene` (`src/apps/...`), `g_personality_api` (`src/spicy/personality_api.h`), `Canvas` (`src/utils/canvas_wrapper.h`), `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` (`config/pin_config.h`) — all existing, unchanged.
- Produces (as `extern "C"` functions, called from JS in Task 3): `twin_select(const char* name)`, `twin_touch(uint16_t x, uint16_t y)`, `twin_tick(uint32_t dt_ms)`, `twin_buffer() -> uint16_t*`, `twin_buffer_size() -> int`, `twin_width() -> int`, `twin_height() -> int`.

- [x] **Step 1: Write `tools/hw/webtwin/twin_main.cpp`**

```cpp
// tools/hw/webtwin/twin_main.cpp
//
// WASM digital twin: the SAME production App/Scene/Canvas code as the
// ESP32 firmware, compiled to WebAssembly instead of Xtensa, driven by
// the browser page in shell.html. This exists specifically because a
// prior project's hand-ported TypeScript "simulator" drifted from its
// C++ firmware and was hard to keep in sync — see
// docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md.

#include "apps/home/home_app.h"
#include "apps/scenes/planet_scene.h"
#include "apps/scenes/eye_scene.h"
#include "spicy/personality_api.h"
#include "utils/canvas_wrapper.h"
#include "config/pin_config.h"

#include <cstring>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace {

Canvas g_canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT);
HomeApp g_home;
PlanetScene g_planet;
EyeScene g_eye;

enum class Target { HOME, PLANET, EYE };
Target g_target = Target::EYE;
bool g_entered_home = false;
bool g_entered_planet = false;
bool g_entered_eye = false;

void ensure_entered() {
    if (g_target == Target::HOME && !g_entered_home) {
        g_home.on_enter();
        g_entered_home = true;
    } else if (g_target == Target::PLANET && !g_entered_planet) {
        g_planet.on_enter();
        g_entered_planet = true;
    } else if (g_target == Target::EYE && !g_entered_eye) {
        g_eye.on_enter();
        g_entered_eye = true;
    }
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
uint16_t* twin_buffer() { return g_canvas.get_buffer(); }

EMSCRIPTEN_KEEPALIVE
int twin_buffer_size() { return static_cast<int>(g_canvas.buffer_size_bytes()); }

EMSCRIPTEN_KEEPALIVE
int twin_width() { return DISPLAY_WIDTH; }

EMSCRIPTEN_KEEPALIVE
int twin_height() { return DISPLAY_HEIGHT; }

EMSCRIPTEN_KEEPALIVE
void twin_select(const char* name) {
    if (strcmp(name, "home") == 0) {
        g_target = Target::HOME;
    } else if (strcmp(name, "planet") == 0) {
        g_target = Target::PLANET;
    } else if (strcmp(name, "eye") == 0) {
        g_target = Target::EYE;
    }
    ensure_entered();
}

EMSCRIPTEN_KEEPALIVE
void twin_touch(uint16_t x, uint16_t y) {
    ensure_entered();
    TouchEvent te{x, y, 0, 255};
    switch (g_target) {
        case Target::HOME: g_home.on_touch(te); break;
        case Target::PLANET: g_planet.on_touch(te); break;
        case Target::EYE: g_eye.on_touch(te); break;
    }
}

EMSCRIPTEN_KEEPALIVE
void twin_tick(uint32_t dt_ms) {
    ensure_entered();
    auto ctx = g_personality_api.get_context();
    switch (g_target) {
        case Target::HOME:
            g_home.update(dt_ms);
            g_home.render(&g_canvas);
            break;
        case Target::PLANET:
            g_planet.update(dt_ms);
            g_planet.render(&g_canvas, ctx);
            break;
        case Target::EYE:
            g_eye.update(dt_ms);
            g_eye.render(&g_canvas, ctx);
            break;
    }
}

}  // extern "C"

int main() {
    ensure_entered();
    return 0;
}
```

- [x] **Step 2: Write `tools/hw/webtwin/build.sh`**

```bash
#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
REPO_ROOT="$(cd ../../.. && pwd)"
mkdir -p dist

SOURCES=$(find "$REPO_ROOT/src" -name '*.cpp' ! -name 'main.cpp')

emcc -std=c++17 -O1 \
  -I "$REPO_ROOT/src" \
  -I "$REPO_ROOT/config" \
  -I "$REPO_ROOT" \
  $SOURCES \
  twin_main.cpp \
  -o dist/twin.js \
  -s EXPORTED_FUNCTIONS='["_twin_buffer","_twin_buffer_size","_twin_width","_twin_height","_twin_select","_twin_touch","_twin_tick","_main"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s EXPORT_ES6=0 \
  -s MODULARIZE=0

echo "built dist/twin.js + dist/twin.wasm"
```

```bash
chmod +x tools/hw/webtwin/build.sh
```

- [x] **Step 3: Run the build**

Run: `tools/hw/webtwin/build.sh`
Expected: `built dist/twin.js + dist/twin.wasm`, and both files exist under `tools/hw/webtwin/dist/`.

- [x] **Step 4: Smoke-test the compiled module under `node`, without a browser yet**

```bash
cd tools/hw/webtwin
node -e "
const Module = require('./dist/twin.js');
Module.onRuntimeInitialized = () => {
  const select = Module.cwrap('twin_select', null, ['string']);
  const tick = Module.cwrap('twin_tick', null, ['number']);
  const bufPtr = Module.cwrap('twin_buffer', 'number', []);
  select('eye');
  tick(16);
  const ptr = bufPtr();
  const view = new Uint16Array(Module.HEAPU16.buffer, ptr, 466 * 466);
  // (233,233) is the eye's pupil center, must be COLOR_BG_BLACK (0x0000).
  const idx = 233 * 466 + 233;
  console.log('pupil value:', view[idx]);
  if (view[idx] !== 0) { console.error('FAIL: expected 0x0000'); process.exit(1); }
  console.log('OK');
};
"
```
Expected: prints `pupil value: 0` then `OK`. If `Module.HEAPU16` is undefined, add `"HEAPU16"` to `EXPORTED_RUNTIME_METHODS` in `build.sh` and rebuild (Step 3) before retrying — this is exactly the kind of thing to fix at this step rather than guess about upfront, since it depends on the exact Emscripten version installed in Task 1.

- [x] **Step 5: Commit**

```bash
git add tools/hw/webtwin/twin_main.cpp tools/hw/webtwin/build.sh
git commit -m "feat(webtwin): compile production render code to WebAssembly

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```

---

### Task 3: `shell.html` — the browser page

**Files:**
- Create: `tools/hw/webtwin/shell.html`

**Interfaces:**
- Consumes: `dist/twin.js`/`dist/twin.wasm` (Task 2).
- Produces: a page a human opens directly (`file://.../shell.html`) or via `python3 -m http.server` from `tools/hw/webtwin/`.

- [x] **Step 1: Write `tools/hw/webtwin/shell.html`**

```html
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Charm Companion Digital Twin</title>
<style>
  body { margin: 0; background: #111; display: flex; flex-direction: column;
         align-items: center; font-family: sans-serif; color: #eee; padding-top: 12px; }
  button { margin: 0 4px; padding: 6px 12px; }
  canvas { border-radius: 50%; border: 2px solid #333; margin-top: 12px; cursor: pointer; }
</style>
</head>
<body>
  <div>
    <button onclick="selectTarget('home')">home</button>
    <button onclick="selectTarget('planet')">planet</button>
    <button onclick="selectTarget('eye')">eye</button>
  </div>
  <canvas id="screen" width="466" height="466"></canvas>
  <script src="dist/twin.js"></script>
  <script>
    let twinTouch, twinTick, twinSelect, twinBuffer, twinWidth, twinHeight;
    const canvas = document.getElementById('screen');
    const ctx2d = canvas.getContext('2d');
    let lastTime = performance.now();

    function frame(now) {
      const dt = Math.min(now - lastTime, 100);
      lastTime = now;
      twinTick(Math.round(dt));

      const ptr = twinBuffer();
      const w = twinWidth();
      const h = twinHeight();
      const words = new Uint16Array(Module.HEAPU16.buffer, ptr, w * h);
      const img = ctx2d.createImageData(w, h);
      for (let i = 0; i < w * h; i++) {
        const v = words[i];
        const r5 = (v >> 11) & 0x1f;
        const g6 = (v >> 5) & 0x3f;
        const b5 = v & 0x1f;
        const o = i * 4;
        img.data[o] = Math.round((r5 * 255) / 31);
        img.data[o + 1] = Math.round((g6 * 255) / 63);
        img.data[o + 2] = Math.round((b5 * 255) / 31);
        img.data[o + 3] = 255;
      }
      ctx2d.putImageData(img, 0, 0);
      requestAnimationFrame(frame);
    }

    canvas.addEventListener('click', (e) => {
      const rect = canvas.getBoundingClientRect();
      const x = Math.round((e.clientX - rect.left) * (canvas.width / rect.width));
      const y = Math.round((e.clientY - rect.top) * (canvas.height / rect.height));
      twinTouch(x, y);
    });

    function selectTarget(name) { twinSelect(name); }

    Module.onRuntimeInitialized = () => {
      twinTouch = Module.cwrap('twin_touch', null, ['number', 'number']);
      twinTick = Module.cwrap('twin_tick', null, ['number']);
      twinSelect = Module.cwrap('twin_select', null, ['string']);
      twinBuffer = Module.cwrap('twin_buffer', 'number', []);
      twinWidth = Module.cwrap('twin_width', 'number', []);
      twinHeight = Module.cwrap('twin_height', 'number', []);
      requestAnimationFrame(frame);
    };
  </script>
</body>
</html>
```

- [x] **Step 2: Automated smoke test via headless Chrome**

```bash
"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" \
  --headless=new --disable-gpu --screenshot=/tmp/webtwin_smoke.png \
  --window-size=500,600 \
  --virtual-time-budget=1000 \
  "file://$(pwd)/tools/hw/webtwin/shell.html"
```

Then read `/tmp/webtwin_smoke.png`.

Expected: the canvas shows a circular eye (white sclera, colored iris, black pupil) — not a blank/black square. If it's blank, re-run with `--enable-logging=stderr --v=1` added and check for a JS error (most likely cause: an `EXPORTED_FUNCTIONS`/`cwrap` name mismatch from Task 2 — fix `build.sh`, rebuild, and retry this step).

- [x] **Step 3: Commit**

```bash
git add tools/hw/webtwin/shell.html
git commit -m "feat(webtwin): add browser page driving the WASM digital twin

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```

---

### Task 4: Verify the click-to-blink interaction end-to-end

**Files:** none — this exercises Tasks 2-3 together.

**Interfaces:** consumes everything from Tasks 2-3; produces a recorded pass/fail for the twin's core interactive loop (click triggers the same `on_touch()` behavior a physical/simulated touch does).

- [x] **Step 1: Screenshot immediately after a simulated click**

Headless Chrome can't easily simulate a mid-page click via the `--screenshot` CLI flag alone, so drive it with a tiny Node script using Chrome's remote debugging protocol is more than this needs — instead, verify the same code path `twin_touch` exercises directly, the way Task 2 Step 4 already did under `node`, but through the full blink timeline:

```bash
cd tools/hw/webtwin
node -e "
const Module = require('./dist/twin.js');
Module.onRuntimeInitialized = () => {
  const select = Module.cwrap('twin_select', null, ['string']);
  const touch = Module.cwrap('twin_touch', null, ['number', 'number']);
  const tick = Module.cwrap('twin_tick', null, ['number']);
  const bufPtr = Module.cwrap('twin_buffer', 'number', []);
  const read = (x, y) => {
    const ptr = bufPtr();
    const view = new Uint16Array(Module.HEAPU16.buffer, ptr, 466 * 466);
    return view[y * 466 + x];
  };

  select('eye');
  tick(16);
  const openIris = read(263, 233);

  touch(233, 233);
  for (let i = 0; i < 10; i++) tick(16); // 160ms, past blink_duration (150ms)
  const closedEyelid = read(263, 233);

  console.log('open iris value:', openIris, 'closed eyelid value:', closedEyelid);
  if (openIris === closedEyelid) { console.error('FAIL: click did not change render state'); process.exit(1); }
  console.log('OK: click-triggered blink changed the framebuffer as expected');
};
"
```
Expected: `open iris value:` and `closed eyelid value:` differ, and the script prints `OK`. This confirms `twin_touch` reaches the exact same `EyeScene::on_touch`/blink state machine already covered by `tests/test_eye_scene.cpp` — the WASM build didn't silently change behavior.

- [x] **Step 2: No commit for this task** (verification only; nothing changed unless Step 1 failed and required a fix in Task 2/3's files, in which case amend those commits' follow-up with a new commit there instead).

---

### Task 5: Extend `hardware-iterate` SKILL.md with the visual-refinement loop

**Files:**
- Modify: `.claude/skills/hardware-iterate/SKILL.md` (created by `docs/superpowers/plans/2026-09-06-hardware-iteration-harness.md` Task 5)

**Interfaces:**
- Consumes: `tools/hw/webtwin/shell.html`, `tools/hw/webtwin/build.sh` (Tasks 2-3).
- Produces: the completed skill, now covering both loops described in the spec.

- [x] **Step 1: Replace the "Visual-refinement missions" section**

Find this block in `.claude/skills/hardware-iterate/SKILL.md`:

```markdown
## Visual-refinement missions

*(v1 scope: this skill currently only covers the loop above. A mission
that is primarily about visual/design refinement — not yet wired to a
digital twin the user can watch live — should still use
`render_reference.py` to check work, but expect the WASM digital twin
described in `docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md`
to add a proper pre-hardware co-iteration loop here in a follow-up plan.)*
```

Replace it with:

```markdown
## Visual-refinement missions

When a mission is primarily about visual/design refinement, iterate on
the digital twin *before* touching hardware:

1. `tools/hw/webtwin/build.sh` (only needed after changing `src/` render
   code or `twin_main.cpp`).
2. Open `tools/hw/webtwin/shell.html` directly in a browser (`file://` is
   fine — no server needed) and tell the user it's ready to look at.
3. Iterate: edit `src/` render code → re-run `build.sh` → user refreshes
   the page → look again. This is the same production C++ source the
   firmware uses, so what the user approves here cannot drift from what
   ships (unlike the TypeScript-reimplementation approach a prior project
   tried and had trouble keeping in sync).
4. Once the user approves the visual, capture it as the pixel-perfect
   reference: `tools/hw/render_reference.py <target> --out <path>.png`
   with whatever `--touch`/`--update-ms` reproduces the approved state.
5. That PNG is now the reference for the autonomous hardware loop above —
   proceed there, diffing the real board's camera capture against it.
```

- [x] **Step 2: Commit**

```bash
git add .claude/skills/hardware-iterate/SKILL.md
git commit -m "feat: add visual-refinement loop to hardware-iterate skill

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01KK4vBa8VgAcevptQhV87Fr"
```
