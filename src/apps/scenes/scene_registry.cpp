// src/apps/scenes/scene_registry.cpp
#include "apps/scenes/scene_registry.h"
#include <iterator>

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#endif

SceneRegistry& SceneRegistry::instance() {
    static SceneRegistry reg;
    return reg;
}

void SceneRegistry::register_scene(const char* name, SceneFactory factory) {
    if (!name) return;
    scenes[std::string(name)] = std::move(factory);
#if defined(ARDUINO)
    Serial.printf("Scene registered: %s\n", name);
#else
    printf("Scene registered: %s\n", name);
#endif
}

Scene* SceneRegistry::load_scene(const char* name) {
    if (!name) return nullptr;
    auto it = scenes.find(std::string(name));
    if (it != scenes.end()) {
        return it->second();  // Call factory
    }
    return nullptr;
}

int SceneRegistry::count() const {
    return static_cast<int>(scenes.size());
}

const char* SceneRegistry::scene_at(int index) const {
    if (index < 0 || index >= static_cast<int>(scenes.size())) {
        return nullptr;
    }
    auto it = scenes.begin();
    std::advance(it, index);
    return it->first.c_str();
}

void SceneRegistry::clear() {
    scenes.clear();
}
