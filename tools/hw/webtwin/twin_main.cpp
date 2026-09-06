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
