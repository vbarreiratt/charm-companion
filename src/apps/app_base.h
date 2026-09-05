#ifndef APPS_APP_BASE_H
#define APPS_APP_BASE_H

#include "shell/event_types.h"
#include "shell/event_bus.h"

class Canvas;  // Forward declare (display driver provides this)

class App : public Listener {
public:
    virtual ~App() = default;
    
    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void on_touch(const TouchEvent& e) = 0;
    virtual void on_motion(const MotionEvent& e) = 0;
    virtual void update(uint32_t dt) = 0;
    virtual void render(Canvas* canvas) = 0;
    
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
