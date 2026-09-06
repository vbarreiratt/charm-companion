#ifndef SHELL_EVENT_BUS_H
#define SHELL_EVENT_BUS_H

#include "event_types.h"
#include <vector>
#include <mutex>

class Listener {
public:
    virtual ~Listener() = default;
    virtual void on_event(const Event& e) = 0;
};

class EventBus {
public:
    EventBus();
    ~EventBus();

    void subscribe(EventType type, Listener* listener);
    void unsubscribe(EventType type, Listener* listener);
    void publish(const Event& event);

private:
    static constexpr int MAX_EVENTS = 16;

    std::vector<Listener*> listeners[MAX_EVENTS];
    // std::mutex works across host native tests and ESP-IDF/Arduino-ESP32.
    // publish() snapshots listeners under lock and releases it before invocation,
    // safely supporting re-entrant / cascaded event publishing.
    std::mutex mutex;
};

extern EventBus g_event_bus; // Global event bus instance

#endif // SHELL_EVENT_BUS_H
