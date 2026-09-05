// src/apps/scenes/scene_registry.h
#ifndef APPS_SCENES_SCENE_REGISTRY_H
#define APPS_SCENES_SCENE_REGISTRY_H

#include "apps/scenes/scene_base.h"
#include <map>
#include <string>
#include <functional>

class SceneRegistry {
public:
    using SceneFactory = std::function<Scene*()>;
    
    static SceneRegistry& instance();
    
    void register_scene(const char* name, SceneFactory factory);
    Scene* load_scene(const char* name);
    
    int count() const;
    const char* scene_at(int index) const;
    void clear();
    
private:
    SceneRegistry() = default;
    ~SceneRegistry() = default;
    
    std::map<std::string, SceneFactory> scenes;
};

#endif // APPS_SCENES_SCENE_REGISTRY_H
