#pragma once

#include "raylib.h"

#include "render/Renderer.hpp"

#include <string>

namespace aic {

class AppState;
class AudioManager;
class Simulation;

// Renders the Main Observation screen: header, left panel, camera-driven world
// view and bottom event log, plus the control button row (spec section 5.2).
class MainPanel {
public:
    void draw(AppState& state, AudioManager& audio, Simulation& sim);

    static Rectangle headerRect();
    static Rectangle controlBarRect();
    static Rectangle leftPanelRect();
    static Rectangle worldViewRect();
    static Rectangle eventLogRect();

private:
    void drawHeader(Simulation& sim);
    void drawControls(AppState& state, AudioManager& audio, Simulation& sim, Rectangle bar);
    void drawLeftPanel(Simulation& sim, Rectangle lp);
    void drawWorldView(Simulation& sim, Rectangle wv);
    void drawSpeedBar(AppState& state, Simulation& sim, Rectangle wv);
    void drawEventLog(Simulation& sim, Rectangle el);
    void handleCamera(Simulation& sim, Rectangle wv);
    void drawEmptyFrame(AppState& state, AudioManager& audio);

    Renderer renderer_;
    Camera2D cam_{};
    bool camInit_ = false;
    bool wasActive_ = false;
    std::string selectedLabel_;
    bool contextMenuOpen_ = false;
    Vector2 contextMenuPos_{ 0.0f, 0.0f };
    int eventLogScroll_ = -1; // -1 follows latest events
};

} // namespace aic
