#pragma once

#include "raylib.h"

#include "app/AppState.hpp"
#include "gui/MainPanel.hpp"

#include <array>

namespace aic {

class AudioManager;
class Simulation;

// Top-level GUI controller. Routes the active screen to the correct renderer
// and owns the screens that do not have a dedicated panel file (Title, Scenario
// Select, Settings). Must be invoked between BeginDrawing()/EndDrawing().
class GuiManager {
public:
    void init();
    void draw(AppState& state, AudioManager& audio, Simulation& sim);

    // True once if the user pressed "Save Settings" since the last call.
    bool takeSaveSettingsRequest();

private:
    void onEnterScreen(AppState& state, Screen previous);
    void drawTitle(AppState& state, AudioManager& audio);
    void drawScenarioSelect(AppState& state, AudioManager& audio);
    void drawSettings(AppState& state, AudioManager& audio);
    void drawDetailScreen(AppState& state, AudioManager& audio, Simulation& sim);
    void drawTopNav(AppState& state, const char* title);
    void drawAgentsScreen(Simulation& sim, Rectangle content);
    void drawConceptsScreen(Simulation& sim, Rectangle content);
    void drawCivilizationScreen(Simulation& sim, Rectangle content);
    void drawAnomalyLabScreen(AppState& state, Simulation& sim, Rectangle content);
    void drawTimelineScreen(Simulation& sim, Rectangle content);
    void drawLogsScreen(AppState& state, Simulation& sim, Rectangle content);
    void updateAndDrawParticles();

    MainPanel mainPanel_;

    Screen lastScreen_ = Screen::Title;
    bool firstFrame_ = true;
    Screen settingsReturn_ = Screen::Title;
    bool saveSettingsRequested_ = false;

    struct Particle { Vector2 pos; Vector2 vel; float r; unsigned char a; };
    std::array<Particle, 140> particles_{};
    bool particlesInit_ = false;

    // Settings screen edit mirrors and per-field edit-mode flags.
    int seedI_ = 12345, maxCycleI_ = 18000, autoSnapI_ = 3000;
    int speakerI_ = 1, cooldownI_ = 15, minLevelI_ = 2;
    bool seedEdit_ = false, maxCycleEdit_ = false, autoSnapEdit_ = false;
    bool speakerEdit_ = false, cooldownEdit_ = false, minLevelEdit_ = false;
    char endpointBuf_[128] = {0};
    bool endpointEdit_ = false;
    int languageActive_ = 0;
    bool languageEdit_ = false;

    int scenarioActive_ = 0;
    float anomalyStrength_ = 75.0f;
    int agentActive_ = 0;
    int conceptActive_ = 0;
    int logFilter_ = 0;
    int agentScroll_ = 0;
    int conceptScroll_ = 0;
    int conceptHistoryScroll_ = 0;
    int civScroll_ = 0;
    int timelineScroll_ = -1; // -1 follows latest events
    int logScroll_ = -1;      // -1 follows latest events
};

} // namespace aic
