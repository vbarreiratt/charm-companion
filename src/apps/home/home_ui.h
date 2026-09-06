#ifndef APPS_HOME_HOME_UI_H
#define APPS_HOME_HOME_UI_H

#include "spicy/personality_types.h"

#include "utils/canvas_wrapper.h"

class HomeUI {
public:
    void render_home_screen(Canvas* canvas, const PersonalityContext& ctx,
                            float motion_intensity, uint32_t anim_frame);
private:
    void render_spicy_eyes(Canvas* canvas, const PersonalityContext& ctx,
                           float motion_intensity, uint32_t anim_frame);
    void render_mood_display(Canvas* canvas, const PersonalityContext& ctx);
    void render_app_launcher(Canvas* canvas);
};

#endif // APPS_HOME_HOME_UI_H
