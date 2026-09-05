// src/apps/scenes/scenes_app.h
#ifndef APPS_SCENES_SCENES_APP_H
#define APPS_SCENES_SCENES_APP_H

#include "apps/app_base.h"
#include "apps/scenes/scene_base.h"

class ScenesApp : public App {
public:
    ScenesApp();
    ~ScenesApp() override;
    
    void on_enter() override;
    void on_exit() override;
    void on_touch(const TouchEvent& e) override;
    void on_motion(const MotionEvent& e) override;
    void update(uint32_t dt) override;
    void render(Canvas* canvas) override;
    
    void next_scene();
    void prev_scene();
    
    // Test accessors
    Scene* get_current_scene() const { return current_scene; }
    int get_current_scene_index() const { return current_scene_index; }
    
private:
    Scene* current_scene = nullptr;
    int current_scene_index = 0;
};

#endif // APPS_SCENES_SCENES_APP_H
