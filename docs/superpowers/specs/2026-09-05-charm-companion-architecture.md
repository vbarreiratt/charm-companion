# Charm Companion: Architecture Design

**Date:** 2026-09-05  
**Status:** Design Phase  
**Phase:** 1 (MVP)

---

## Executive Summary

**Charm Companion (Spicy)** is a portable AMOLED charm device (1.75" circular) that acts as a **personal, expressive companion**. It combines:
- **OS-level personality core** (Spicy) that manages mood, personality traits, and theme
- **Event-driven architecture** where apps (Home, Scenes) subscribe to personality context and respond authentically
- **Motion-reactive behavior** that makes her feel alive (responds to bag movement, touch, gestures)
- **Progressive discovery** as users explore interactions and apps

**Vision:** First interaction evokes wonder, magic, intelligence, beauty. Someone pulls it from a bag and discovers something delightfully alive—curious, playful, with personality that cascades through every interaction.

---

## Architecture Overview

### System Layers

```
┌─────────────────────────────────────────────────────────┐
│                    SPICY (OS Layer)                     │
│  • Personality Core (mood, traits, emotional states)    │
│  • Event Bus (pub/sub for all interactions)             │
│  • Personality API (context for apps)                   │
│  • Theme/Color Management                              │
│  • Sensor Routing (IMU → events)                        │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                  SHELL (System Layer)                   │
│  • Display Driver (CO5300, PSRAM framebuffer)           │
│  • Touch Input (CST9217)                                │
│  • Motion Sensing (QMI8658 IMU)                         │
│  • Power Management (AXP2101)                           │
│  • Main event loop, app lifecycle                       │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                      APPS                               │
│  • Home App (Spicy on-screen)                           │
│  • Scenes App (Planet, Eye, Star, Ghost, ...)           │
│  • Future apps (phase 2+)                               │
└─────────────────────────────────────────────────────────┘
```

### Architecture Pattern: Event Bus + Personality Registry (Approach B)

**Spicy** is NOT a monolithic personality engine. It's:
1. **Event Bus** — centralized pub/sub for all interactions (touch, motion, app transitions)
2. **Personality Registry** — single source of truth for mood, traits, theme, tone rules
3. **State Manager** — persists and syncs personality state (local + WiFi backend, phase 2)

**Apps** (Home, Scenes, future):
- Query Spicy's personality context when needed
- Subscribe to events relevant to them
- Publish events (user taps, app transitions, etc.)
- Render based on personality + their own logic
- Personality is *informed by* Spicy, but expression is *their own*

---

## Core Components

### 1. Spicy: Personality Core

**Responsibility:** Manage personality state, provide personality context to apps, route sensor events.

**Data Structure:**
```cpp
struct Personality {
    mood: enum {
        PLAYFUL,
        CURIOUS,
        CALM,
        ANGRY,
        SCARED,
        SAD,
        LOST
    };
    
    traits: {
        curiosity: 0-100,
        mischief: 0-100,
        patience: 0-100,
        creativity: 0-100,
        fashion_sense: 0-100   // tech-girly aesthetic
    };
    
    emotional_state: {
        happiness: 0-100,
        irritation: 0-100,
        confidence: 0-100,
        playfulness: 0-100
    };
    
    theme: {
        primary_color: RGB565,
        accent_color: RGB565,
        bg_color: RGB565,
        animation_speed_factor: 0.5-2.0
    };
};
```

**Mood Cascade Logic:**
When mood changes (e.g., user sets "playful" for today):
1. Update `Personality.mood`
2. Adjust `traits` and `emotional_state` to reflect mood
3. Select `theme` colors matched to mood
4. Publish `MoodChanged` event with new context
5. Apps re-render with new personality

**Example:**
```
User sets mood = PLAYFUL
  ↓
Spicy updates: curiosity +20, mischief +15, patience -10
  ↓
Spicy selects bright, energetic colors (warm yellows, pinks)
  ↓
Spicy publishes MoodChanged(context={mood, traits, theme})
  ↓
Home app re-renders Spicy with new colors
  ↓
Scenes app uses new animation speed
```

### 2. Event Bus

**Responsibility:** Decoupled communication between Shell, Spicy, and Apps.

**Event Types:**

| Event | Emitted By | Listeners | Payload |
|-------|-----------|-----------|---------|
| `MoodChanged` | Spicy | All apps | `{mood, traits, theme}` |
| `TouchEvent` | Shell | Active app | `{x, y, duration, intensity}` |
| `MotionEvent` | Shell (IMU) | Spicy, active app | `{accel, gyro, intensity, direction}` |
| `AppTransition` | Shell | Spicy, old app, new app | `{from_app, to_app}` |
| `SensorTrick` | App | Spicy | `{trick_type, params}` (e.g., shake → 8 Ball prediction) |
| `TextRequest` | App | Spicy | `{context}` → returns tone-adjusted text |
| `AnimationRequest` | App | Spicy | `{animation_type}` → returns animation speed + style |

**Implementation Pattern:**
```cpp
class EventBus {
    void subscribe(EventType type, Listener* listener);
    void unsubscribe(EventType type, Listener* listener);
    void publish(Event event);  // async, non-blocking
};

// Example: Home app subscribes to MoodChanged
void HomeApp::onMoodChanged(const Event& e) {
    auto context = e.personality_context;
    redraw_with_colors(context.theme);
}
```

### 3. Personality API

**Responsibility:** Apps ask Spicy "how should I express this?"

**Methods:**

```cpp
class PersonalityAPI {
    // Get current personality context
    PersonalityContext getContext();
    
    // Get tone for a specific action/text
    string getTone(TextType type);  // "prediction", "greeting", "reaction"
    
    // Get animation style for personality
    AnimationStyle getAnimationStyle(AnimationType type);  // "idle", "excited", "calm"
    
    // Get color palette for current mood
    ThemeColors getThemeColors();
    
    // Adjust emotional state (apps can "affect" Spicy)
    void recordInteraction(InteractionType type);  // "played_with", "ignored", "poked"
};
```

**Example Usage (Scenes App):**
```cpp
void ScenesApp::renderPlanetScene() {
    auto personality = spicy_api->getContext();
    auto animation_speed = spicy_api->getAnimationStyle(ROTATION).speed;
    auto colors = spicy_api->getThemeColors();
    
    // Planet spins at speed tuned to Spicy's personality
    rotate_planet(animation_speed * personality.mood_energy);
    set_planet_colors(colors.primary, colors.accent);
}
```

### 4. Home App

**Responsibility:** Spicy's visual presence on screen. Idle home screen, mood customization entry point.

**Features:**
- **Idle state:** Spicy animated, responsive to motion (shake → she reacts)
- **Mood selector:** Touch menu to set mood for the day (cascades to all apps)
- **Status display:** Current mood, theme colors, emotional state
- **App launcher:** Grid or carousel to access Scenes app (and future apps)

**Data Flow:**
1. Shell detects motion → publishes `MotionEvent`
2. Home app receives `MotionEvent` → looks up Spicy's reaction based on mood
3. Home app renders Spicy's reaction (animation, color shift, emote)
4. User taps mood selector → updates `Spicy.mood` → triggers `MoodChanged` event
5. All apps (including Home) re-render with new personality

**UI Sketch (conceptual):**
```
┌─────────────────────┐
│   SPICY (eyes)      │  ← motion-reactive, expressive
│   [Happy/Calm/etc]  │
├─────────────────────┤
│   Mood: 😊 PLAYFUL  │  ← user-tappable to change
├─────────────────────┤
│    [Apps Grid]      │  ← access Scenes, future apps
└─────────────────────┘
```

### 5. Scenes App Framework

**Responsibility:** Extensible framework for loading scenes (Planet, Eye, Star, Ghost, etc). Each scene inherits Spicy's personality through tone + visuals.

**Architecture:**

```cpp
class Scene {
    virtual void render(Canvas* canvas) = 0;
    virtual void on_touch(TouchEvent e) = 0;
    virtual void on_motion(MotionEvent e) = 0;
    virtual void update(uint32_t dt) = 0;
};

class ScenesApp {
    vector<Scene*> available_scenes;
    Scene* current_scene;
    
    void load_scene(string scene_name);
    void next_scene();
    void prev_scene();
};
```

**Phase 1 Scenes (example):**
- **Planet:** Spins and rotates. Touch = speed up. Mood colors affect glow.
- **Eye:** Blinks with Spicy's rhythm. Animated lashes. Color-reactive.
- (Future: Star, Ghost, Doodle canvas, etc.)

**Personality Expression in Scenes:**
Each scene queries Spicy's API to adjust behavior:
```cpp
// Planet scene
void PlanetScene::update(uint32_t dt) {
    auto personality = spicy_api->getContext();
    rotation_speed = base_speed * personality.emotional_state.playfulness / 50.0f;
    glow_color = spicy_api->getThemeColors().primary;
}
```

### 6. Shell: System Layer

**Responsibility:** Display driver, input handling, IMU sensor routing, app lifecycle.

**Key Responsibilities:**
- Initialize all hardware (display, touch, IMU, power)
- Main event loop:
  - Poll IMU → publish `MotionEvent`
  - Poll touch → publish `TouchEvent`
  - Update active app
  - Render to PSRAM framebuffer
  - Flush to display (56.7ms cycle)
- App lifecycle (load/unload/switch)
- Persist Spicy's mood state (local storage)

**Main Loop (pseudocode):**
```cpp
void shell_main_loop() {
    while (true) {
        // 1. Sensor input
        if (imu->has_data()) {
            auto motion = imu->read();
            event_bus->publish(MotionEvent{motion});
        }
        if (touch->has_data()) {
            auto tap = touch->read();
            event_bus->publish(TouchEvent{tap});
        }
        
        // 2. Active app update & render
        active_app->update(dt);
        active_app->render(canvas);
        
        // 3. Flush to display
        display->flush(canvas);
        
        // 4. Handle app transitions
        if (app_transition_requested) {
            switch_app(next_app);
        }
        
        delay(16ms);  // ~60 FPS target (though display refresh is ~17 FPS)
    }
}
```

---

## Data Flow: Complete Example

**Scenario:** User carries charm in bag (motion), then taps it to set mood.

### 1. Motion Detection → Spicy Reacts

```
IMU detects shake
  ↓
Shell publishes MotionEvent{accel=high, direction=vertical}
  ↓
Spicy subscribes to MotionEvent
  ↓
Spicy determines reaction: "shake = playful, so she gets excited"
  → Updates emotional_state.playfulness +20
  → Publishes ReactionEvent{animation="excited_eyes", color_pulse=true}
  ↓
Home app receives ReactionEvent
  ↓
Home app renders Spicy with excited animation + pulsing colors
```

### 2. Mood Customization → Cascade

```
User taps "Set Mood" in Home app
  ↓
Home app shows mood selector (PLAYFUL, CALM, CURIOUS, etc.)
  ↓
User selects PLAYFUL
  ↓
Home app calls spicy->set_mood(PLAYFUL)
  ↓
Spicy updates personality state:
  • mood = PLAYFUL
  • curiosity +20, mischief +15, patience -10
  • theme = bright, energetic colors
  ↓
Spicy publishes MoodChanged{new_context}
  ↓
ALL apps (Home, Scenes, future) receive MoodChanged
  ↓
Each app re-renders using new theme & personality context
```

### 3. App Transition

```
User taps "Scenes" in Home app
  ↓
Home app calls shell->switch_app("scenes")
  ↓
Shell publishes AppTransition{from=Home, to=Scenes}
  ↓
Home app unsubscribes from events, pauses rendering
  ↓
Scenes app initializes, subscribes to events
  ↓
Scenes app renders first scene (Planet)
  ↓
Shell sets Scenes as active_app
```

---

## State Persistence & Sync

### Phase 1 (Local Only)

- **Spicy's mood state** saved to NVS (ESP32 non-volatile storage)
- On boot, restore last mood (or default to CURIOUS)
- No WiFi sync yet

**Persistence Point:**
```cpp
void Spicy::set_mood(Mood m) {
    mood = m;
    update_traits_and_theme();
    event_bus->publish(MoodChanged{...});
    
    // Persist
    nvs->set("mood", mood);
    nvs->set("traits", traits);
    nvs->commit();
}
```

### Phase 2 (WiFi + Mobile Dashboard)

- Sync mood state to phone app (BLE or local network API)
- Phone dashboard allows remote mood changes
- Future: cloud sync for backup/restore

---

## Component Interactions Summary

| Component | Publishes | Subscribes | Queries | Mutates |
|-----------|-----------|-----------|---------|---------|
| **Spicy** | MoodChanged, ReactionEvent | MotionEvent, InteractionEvent | — | personality state |
| **Shell** | MotionEvent, TouchEvent, AppTransition | — | personality context (for routing) | — |
| **Home App** | AppTransition | MoodChanged, MotionEvent, ReactionEvent | personality context, tone API | display state |
| **Scenes App** | (as scenes see fit) | MoodChanged, MotionEvent, TouchEvent | personality context, animation style | scene state |

---

## Phase 1 Scope

### In Scope
- ✅ Shell (drivers validated for CO5300, CST9217, QMI8658, AXP2101)
- ✅ Spicy Core (mood, traits, emotional states, theme cascading)
- ✅ Event Bus + Personality API
- ✅ Home App (Spicy on-screen, mood selector, app launcher)
- ✅ Scenes App Framework (structure for extensible scenes)
- ✅ 1-2 example scenes (e.g., Planet, Eye) built and integrated
- ✅ Basic motion reactions (shake → Spicy reacts, but not environment-aware)
- ✅ Local persistence (mood saved to NVS)

### Out of Scope (Phase 2+)
- ❌ Environment-reactive personality (learning, ambient awareness)
- ❌ Sensor tricks (8 Ball, etc.)
- ❌ WiFi sync / mobile dashboard
- ❌ Multi-scene interactions (scenes talking to each other)
- ❌ Voice input / audio output
- ❌ External APIs

---

## File Structure

```
charm-companion/
├── src/
│   ├── main.cpp                        # Shell entry point, main loop
│   ├── shell/
│   │   ├── shell.h / shell.cpp         # App lifecycle, hardware init
│   │   ├── event_bus.h / event_bus.cpp # Event bus implementation
│   │   └── hal/
│   │       ├── display_hal.h/cpp       # CO5300 driver wrapper
│   │       ├── touch_hal.h/cpp         # CST9217 wrapper
│   │       ├── imu_hal.h/cpp           # QMI8658 wrapper
│   │       └── power_hal.h/cpp         # AXP2101 wrapper
│   ├── spicy/
│   │   ├── spicy.h / spicy.cpp         # Personality core
│   │   ├── personality_api.h/cpp       # Query interface
│   │   └── personality_types.h         # Mood, traits, theme structs
│   ├── apps/
│   │   ├── app_base.h                  # Base class for all apps
│   │   ├── home/
│   │   │   ├── home_app.h / home_app.cpp
│   │   │   └── home_ui.h/cpp           # UI components (mood selector, etc)
│   │   └── scenes/
│   │       ├── scenes_app.h / scenes_app.cpp
│   │       ├── scene_base.h            # Scene base class
│   │       ├── scenes/
│   │       │   ├── planet_scene.h/cpp
│   │       │   ├── eye_scene.h/cpp
│   │       │   └── (future scenes)
│   │       └── scene_registry.h/cpp    # Scene loader
│   └── utils/
│       ├── canvas_wrapper.h/cpp        # PSRAM framebuffer
│       ├── color_utils.h/cpp           # RGB565, theme management
│       └── animation.h/cpp             # Animation curves, interpolation
├── config/
│   ├── sdkconfig.defaults              # ESP-IDF config
│   ├── board/waveshare-amoled-175c.ini # PlatformIO board config
│   └── pin_config.h                    # GPIO definitions
├── docs/
│   ├── HARDWARE.md                     # Hardware reference (from esp32-badge)
│   ├── PINOUTS.md                      # Pinouts & I2C topology
│   ├── BUILD_SYSTEMS.md                # Build & toolchain setup
│   ├── REFERENCE.md                    # Waveshare repo reference
│   └── superpowers/specs/
│       └── 2026-09-05-charm-companion-architecture.md (this doc)
├── tests/
│   ├── test_personality_api.cpp
│   ├── test_event_bus.cpp
│   ├── test_scenes_app.cpp
│   └── (unit tests for each component)
├── CMakeLists.txt                      # or platformio.ini (TBD: ESP-IDF vs Arduino)
└── README.md
```

---

## Testing Strategy

### Phase 1 Unit Tests
- **Spicy Core:** mood changes, trait updates, theme generation
- **Event Bus:** publish/subscribe, event ordering, memory
- **Personality API:** context queries, tone generation (mock)
- **Home App:** mood selector, app launcher transitions
- **Scenes App:** scene loading, rendering (mock), animation updates

### Phase 1 Integration Tests
- **Motion → Reaction:** IMU shake → Spicy reacts → Home renders (simulator or real hardware)
- **Mood Cascade:** Set mood → all apps receive update → visual change verified
- **App Transition:** Home → Scenes → Home (state preservation)

### Hardware Validation
- Display: banding test (CO5300 1px window fix), color accuracy
- Touch: tap-detect, drag smoothness, calibration
- IMU: shake detection, motion data quality
- Power: battery percentage accurate, brightness control works

---

## Future Extensibility (Phase 2+)

### Sensor Tricks (8 Ball, etc.)
```cpp
// Example: shake = trigger 8 Ball
void ShellMain::on_shake_detected() {
    event_bus->publish(SensorTrick{type=SHAKE});
}

// 8 Ball app (future) subscribes
void EightBallApp::on_sensor_trick(SensorTrick e) {
    if (e.type == SHAKE) {
        auto tone = spicy_api->getTone("prediction");
        string prediction = generate_prediction_in_tone(tone);
        render_prediction(prediction);
    }
}
```

### Environment-Reactive Personality
```cpp
// Phase 2: expand Spicy to learn interaction patterns
struct InteractionLog {
    timestamp,
    interaction_type,  // "shaken", "touched", "ignored"
    emotional_response,
    app_active
};

// Spicy analyzes logs over time
// Adjusts personality: "she's more playful when you're moving around"
```

### Mobile Dashboard
```cpp
// Phase 2: WiFi API
GET /api/spicy/mood → current mood
POST /api/spicy/mood → set mood
GET /api/spicy/context → personality state
POST /api/spicy/trait → adjust trait (future)
```

---

## Success Criteria (Phase 1)

- [ ] Shell compiles and runs on Waveshare AMOLED 1.75C
- [ ] All drivers validated (display, touch, IMU, power)
- [ ] Spicy mood cascades visually to Home + Scenes
- [ ] Home App displays Spicy with motion reactions
- [ ] Scenes App loads and renders Planet scene with mood-adjusted colors
- [ ] User can set mood and see cascade effect across all apps
- [ ] First interaction creates magic moment (motion → reaction)
- [ ] Battery life acceptable for daily bag use (TBD: target hours between charges)
- [ ] Unit tests pass (core components)

---

## Open Questions for Next Phase

1. **Build System:** ESP-IDF or Arduino? (Recommend ESP-IDF for control + component ecosystem, but Arduino for faster iteration)
2. **Scene Rendering:** Use Arduino_GFX primitives only, or integrate LVGL for layouts?
3. **Tone Generation:** Hardcoded personality text, or parametric generation?
4. **Animation System:** Use existing Arduino_GFX tweening, or custom animation engine?
5. **Mobile Backend:** BLE or WiFi local API for phase 2 dashboard?

---

**Document Status:** Ready for implementation planning phase.

Next step: Invoke `writing-plans` skill to create detailed implementation plan.
