#ifndef APPS_HOME_HOME_APP_H
#define APPS_HOME_HOME_APP_H

#include "apps/app_base.h"
#include "apps/home/home_ui.h"

class HomeApp : public App {
public:
    HomeApp();
    ~HomeApp() override;
    
    void on_enter() override;
    void on_exit() override;
    void on_touch(const TouchEvent& e) override;
    void on_motion(const MotionEvent& e) override;
    void update(uint32_t dt) override;
    void render(Canvas* canvas) override;

    // Test accessors
    uint32_t get_frame_counter() const { return frame_counter; }
    float get_motion_intensity() const { return motion_intensity; }
    int get_touch_count() const { return touch_count; }

protected:
    uint32_t frame_counter = 0;
    float motion_intensity = 0.0f;
    int touch_count = 0;
    HomeUI ui;
};

#endif // APPS_HOME_HOME_APP_H
