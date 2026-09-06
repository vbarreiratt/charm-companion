#ifndef APPS_APP_BASE_H
#define APPS_APP_BASE_H

#include "shell/event_types.h"
#include "shell/event_bus.h"

#include "utils/canvas_wrapper.h"

class App : public Listener {
public:
    virtual ~App() = default;
    
    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void on_touch(const TouchEvent& e) = 0;
    virtual void on_motion(const MotionEvent& e) = 0;
    virtual void update(uint32_t dt) = 0;
    virtual void render(Canvas* canvas) = 0;

    // Physical back-button navigation. Return true if this app handled it
    // internally (e.g. moved to a previous sub-state); false if there is
    // nothing left to go back to, so the caller (Shell) should navigate
    // to Home instead. Default: nothing to go back to.
    virtual bool handle_back() { return false; }
    
    // Listener interface (receives events from EventBus)
    void on_event(const Event& e) override;
    
protected:
    void subscribe_to(EventType type) {
        g_event_bus.subscribe(type, this);
    }
    
    void unsubscribe_from(EventType type) {
        g_event_bus.unsubscribe(type, this);
    }
};

#endif // APPS_APP_BASE_H
