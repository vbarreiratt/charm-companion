// src/apps/scenes/scene_base.h
#ifndef APPS_SCENES_SCENE_BASE_H
#define APPS_SCENES_SCENE_BASE_H

#include "shell/event_types.h"
#include "spicy/personality_types.h"

class Canvas;

class Scene {
public:
    virtual ~Scene() = default;
    
    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void on_touch(const TouchEvent& e) = 0;
    virtual void on_motion(const MotionEvent& e) = 0;
    virtual void update(uint32_t dt) = 0;
    virtual void render(Canvas* canvas, const PersonalityContext& ctx) = 0;
    
    virtual const char* name() const = 0;
};

#endif // APPS_SCENES_SCENE_BASE_H
