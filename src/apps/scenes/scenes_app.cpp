// src/apps/scenes/scenes_app.cpp
#include "apps/scenes/scenes_app.h"
#include "apps/scenes/scene_registry.h"
#include "spicy/personality_api.h"

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

ScenesApp::ScenesApp() : current_scene(nullptr), current_scene_index(0) {
    subscribe_to(EventType::MOOD_CHANGED);
}

ScenesApp::~ScenesApp() {
    unsubscribe_from(EventType::MOOD_CHANGED);
    if (current_scene) {
        current_scene->on_exit();
        delete current_scene;
        current_scene = nullptr;
    }
}

void ScenesApp::load_scene_at_index(int index) {
    if (current_scene) {
        current_scene->on_exit();
        delete current_scene;
        current_scene = nullptr;
    }

    auto& registry = SceneRegistry::instance();
    if (index >= 0 && index < registry.count()) {
        current_scene_index = index;
        const char* name = registry.scene_at(index);
        if (name) {
            current_scene = registry.load_scene(name);
            if (current_scene) {
                current_scene->on_enter();
            }
        }
    }
}

void ScenesApp::on_enter() {
#if defined(ARDUINO)
    Serial.println("ScenesApp::on_enter()");
#else
    printf("ScenesApp::on_enter()\n");
#endif
    load_scene_at_index(0);
}

void ScenesApp::on_exit() {
#if defined(ARDUINO)
    Serial.println("ScenesApp::on_exit()");
#else
    printf("ScenesApp::on_exit()\n");
#endif
    if (current_scene) {
        current_scene->on_exit();
        delete current_scene;
        current_scene = nullptr;
    }
}

void ScenesApp::on_touch(const TouchEvent& e) {
    if (current_scene) {
        current_scene->on_touch(e);
    }
}

void ScenesApp::on_motion(const MotionEvent& e) {
    if (current_scene) {
        current_scene->on_motion(e);
    }
}

void ScenesApp::update(uint32_t dt) {
    if (current_scene) {
        current_scene->update(dt);
    }
}

void ScenesApp::render(Canvas* canvas) {
    if (current_scene) {
        auto ctx = g_personality_api.get_context();
        current_scene->render(canvas, ctx);
    }
}

void ScenesApp::next_scene() {
    auto& registry = SceneRegistry::instance();
    if (registry.count() > 0) {
        load_scene_at_index((current_scene_index + 1) % registry.count());
    }
}

void ScenesApp::prev_scene() {
    auto& registry = SceneRegistry::instance();
    if (registry.count() > 0) {
        load_scene_at_index((current_scene_index - 1 + registry.count()) % registry.count());
    }
}
