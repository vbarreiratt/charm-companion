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
//   render_reference <home|planet|eye> --out <path.raw> [--touch]
//                     [--touch-x N] [--touch-y N] [--update-ms N]

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
        fprintf(stderr, "usage: render_reference <home|planet|eye> --out <path> [--touch]"
                        " [--touch-x N] [--touch-y N] [--update-ms N]\n");
        return 1;
    }

    const char* target = argv[1];
    const char* out_path = nullptr;
    bool touch = false;
    uint32_t update_ms = 0;
    uint16_t touch_x = 0;
    uint16_t touch_y = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(argv[i], "--touch") == 0) {
            touch = true;
        } else if (strcmp(argv[i], "--touch-x") == 0 && i + 1 < argc) {
            touch_x = static_cast<uint16_t>(atoi(argv[++i]));
        } else if (strcmp(argv[i], "--touch-y") == 0 && i + 1 < argc) {
            touch_y = static_cast<uint16_t>(atoi(argv[++i]));
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
    TouchEvent te{touch_x, touch_y, 0, 0};
    if (touch) {
        fprintf(stderr, "render_reference: touch x=%u y=%u\n", touch_x, touch_y);
    }

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
