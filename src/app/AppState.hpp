#pragma once

#include <string>
#include <vector>

namespace aic {

// All GUI screens (spec section 5: GUI pages).
enum class Screen {
    Title,
    ScenarioSelect,
    Main,
    Agents,
    Concepts,
    Civilization,
    AnomalyLab,
    Timeline,
    Logs,
    Settings
};

const char* screenName(Screen s);

// --- Settings (persisted to config/app_settings.json) ----------------------

struct SimulationSettings {
    unsigned int seed = 12345;
    int   maxCycle = 18000;
    float speed = 1.0f;              // simulation speed multiplier
    int   autoSnapshotInterval = 3000; // cycles between auto snapshots; 0 = off
};

struct AudioSettings {
    float bgmVolume = 0.6f;
    float seVolume = 0.8f;
    bool  mute = false;
    bool  duckDuringTts = true;
};

struct NotificationSettings {
    bool inApp = true;
    bool windows = true;
    bool tts = true;
    std::string voicevoxEndpoint = "http://127.0.0.1:50021";
    int   speakerId = 1;
    float ttsVolume = 0.9f;
    int   minLevel = 2;        // AlertLevel index (0 Info .. 4 Emergency)
    int   cooldownSeconds = 15;
};

struct UiSettings {
    int language = 0; // 0 = Japanese
};

struct Settings {
    SimulationSettings sim;
    AudioSettings audio;
    NotificationSettings notify;
    UiSettings ui;
};

// --- Scenario presets (config/scenarios/*.json) ----------------------------

struct ScenarioInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string file;   // absolute/relative path to the scenario json
};

// Pure data shared across the application. Subsystems (audio, gui, simulation,
// notifications) are owned by App, not stored here.
class AppState {
public:
    Screen screen = Screen::Title;
    Settings settings;
    std::vector<ScenarioInfo> scenarios;
    int  selectedScenario = -1;  // index into scenarios; -1 = none chosen
    bool startRequested = false; // a new simulation start was requested
    bool quit = false;

    bool saveRequested = false;
    bool loadRequested = false;
    bool snapshotRequested = false;
    bool exportRequested = false;
    bool screenshotRequested = false;

    bool anomalyInjectionRequested = false;
    bool experimentRequested = false;
    std::string requestedAnomaly = "BlackHole";
    float requestedAnomalyStrength = 75.0f;

    bool testWindowsNotificationRequested = false;
    bool testVoicevoxRequested = false;
    bool testInAppAlertRequested = false;
    bool helpVisible = false;

    // Switches the active screen and logs the transition.
    void setScreen(Screen s);
};

} // namespace aic
