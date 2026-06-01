#pragma once

#include "app/AppState.hpp"
#include "audio/AudioManager.hpp"
#include "gui/GuiManager.hpp"
#include "notify/NotificationManager.hpp"
#include "simulation/Simulation.hpp"

namespace aic {

// Application root: owns the window, subsystems and the main loop.
class App {
public:
    bool init();
    void run();
    void shutdown();

private:
    void update(float dt);
    void updateScreenBgm();
    void startSimulation();
    void dispatchEventSe();
    void dispatchEvents();
    void flushCivilizationLog();
    void handleHotkeys();
    void handleRequests();
    void loadSettings();
    void saveSettings();
    void loadScenarios();
    void ensureRuntimeFiles();
    void saveSimulation(bool snapshot);
    void loadLatestSimulation();
    void exportData();
    void writeFinalReport(const std::string& path);
    void captureScreenshot(const std::string& path);
    void drawHelpOverlay();

    AppState state_;
    AudioManager audio_;
    GuiManager gui_;
    NotificationManager notifications_;
    Simulation sim_;
    int lastEventIndex_ = 0;
    int lastCivilizationLogIndex_ = 0;
    int lastAppliedScenario_ = -1;
    long lastAutoSnapshotCycle_ = -1;
    bool finalReportWritten_ = false;
    bool windowOpen_ = false;
};

} // namespace aic
