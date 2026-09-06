#ifndef SHELL_SHELL_H
#define SHELL_SHELL_H

#include "shell/event_bus.h"
#include <cstdint>
#include <cstddef>

class App;
class Canvas;

class Shell {
public:
    static Shell& instance();
    
    bool init();
    void run();  // Main loop
    void tick(uint32_t dt); // Single step for testing/deterministic execution
    void switch_app(App* app);
    void switch_app(const char* app_name);
    void switch_app(std::nullptr_t) { switch_app(static_cast<App*>(nullptr)); }
    
    App* get_current_app() const { return current_app; }
    
    // Allow tests or main loop to poll sensors explicitly
    void poll_sensors();

private:
    Shell() = default;
    ~Shell();
    
    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;
    
    App* current_app = nullptr;
    App* next_app = nullptr;
    bool app_transition_pending = false;
    const char* current_app_name = nullptr;
    const char* next_app_name = nullptr;
    
    void update_active_app(uint32_t dt);
    void render_and_flush();
    void handle_app_transition();
};

#endif
