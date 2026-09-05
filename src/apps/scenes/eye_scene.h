#ifndef APPS_SCENES_EYE_SCENE_H
#define APPS_SCENES_EYE_SCENE_H

#include "apps/scenes/scene_base.h"

class EyeScene : public Scene {
public:
    EyeScene();
    const char* name() const override { return "eye"; }
    void on_enter() override;
    void on_exit() override;
    void on_touch(const TouchEvent& e) override;
    void on_motion(const MotionEvent& e) override;
    void update(uint32_t dt) override;
    void render(Canvas* canvas, const PersonalityContext& ctx) override;
private:
    uint32_t blink_timer = 0;
    uint32_t blink_interval = 3000;  // ms between auto-blinks
    bool is_blinking = false;
    uint32_t blink_duration = 150;   // ms per blink
    void draw_eye(Canvas* canvas, int cx, int cy, int size,
                  uint16_t iris_color, float blink_progress);
};

#endif // APPS_SCENES_EYE_SCENE_H
