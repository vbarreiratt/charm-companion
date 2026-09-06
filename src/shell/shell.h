#ifndef SHELL_SHELL_H
#define SHELL_SHELL_H

#include "shell/event_bus.h"
#include "shell/swipe_detector.h"
#include "utils/canvas_wrapper.h"
#include <cstdint>
#include <cstddef>

class App;

class Shell {
public:
    static Shell& instance();

    bool init();
    void run();  // Main loop
    void tick(uint32_t dt); // Single step for testing/deterministic execution
    void switch_app(App* app, const char* name = nullptr);
    void switch_app(const char* app_name);
    void switch_app(std::nullptr_t) { switch_app(static_cast<App*>(nullptr), nullptr); }
    
    App* get_current_app() const { return current_app; }
    const char* get_current_app_name() const { return current_app_name; }
    
    // Allow tests or main loop to poll sensors explicitly
    void poll_sensors();

    // Physical back-button navigation: asks the current app to handle it
    // (e.g. ScenesApp moving to a previous scene); if it can't, exits to
    // Home. Public so it can be driven by ButtonHAL in poll_sensors() or
    // called directly by tests.
    void handle_boot_press();

    // Feeds a synthetic touch sample through the same swipe-detection /
    // TOUCH_EVENT-publishing path poll_sensors() uses for real hardware.
    // Lets tests (and the serial debug console) simulate a swipe without
    // real touch hardware.
    void debug_inject_touch(uint16_t x, uint16_t y, bool touched);

private:
    Shell();
    ~Shell();
    
    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;
    
    App* current_app = nullptr;
    App* next_app = nullptr;
    bool app_transition_pending = false;
    const char* current_app_name = nullptr;
    const char* next_app_name = nullptr;
    Canvas screen_canvas;
    SwipeDetector swipe_detector;

    void update_active_app(uint32_t dt);
    void render_and_flush();
    void handle_app_transition();
    void process_touch(bool touched, uint16_t x, uint16_t y);
};

#endif
