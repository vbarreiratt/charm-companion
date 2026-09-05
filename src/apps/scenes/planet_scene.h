#ifndef APPS_SCENES_PLANET_SCENE_H
#define APPS_SCENES_PLANET_SCENE_H

#include "apps/scenes/scene_base.h"

class PlanetScene : public Scene {
public:
    PlanetScene();
    const char* name() const override { return "planet"; }
    void on_enter() override;
    void on_exit() override;
    void on_touch(const TouchEvent& e) override;
    void on_motion(const MotionEvent& e) override;
    void update(uint32_t dt) override;
    void render(Canvas* canvas, const PersonalityContext& ctx) override;
private:
    float rotation_angle = 0.0f;
    float touch_speed_boost = 0.0f;
    void draw_planet(Canvas* canvas, int cx, int cy, int radius,
                     uint16_t color, float rotation, float glow);
};

#endif // APPS_SCENES_PLANET_SCENE_H
