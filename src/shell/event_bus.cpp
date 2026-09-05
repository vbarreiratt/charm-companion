#include "shell/event_bus.h"
#include <algorithm>

EventBus g_event_bus;

EventBus::EventBus() = default;

EventBus::~EventBus() = default;

void EventBus::subscribe(EventType type, Listener* listener) {
    if (!listener) {
        return;
    }
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= MAX_EVENTS) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    auto& list = listeners[idx];
    if (std::find(list.begin(), list.end(), listener) == list.end()) {
        list.push_back(listener);
    }
}

void EventBus::unsubscribe(EventType type, Listener* listener) {
    if (!listener) {
        return;
    }
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= MAX_EVENTS) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    auto& list = listeners[idx];
    list.erase(
        std::remove(list.begin(), list.end(), listener),
        list.end()
    );
}

void EventBus::publish(const Event& event) {
    int idx = static_cast<int>(event.type);
    if (idx < 0 || idx >= MAX_EVENTS) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    // NOTE: publish() holds mutex while invoking listener callbacks.
    // Re-entrant calls to publish() from within on_event() will cause deadlock.
    // Safe for Phase 1 (synchronous, non-reentrant event dispatch).
    auto& list = listeners[idx];
    for (auto* listener : list) {
        if (listener) {
            listener->on_event(event);
        }
    }
}
