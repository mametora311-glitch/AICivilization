#include "app/App.hpp"

#include "raylib.h"

#include "analysis/CivilizationAnalyzer.hpp"
#include "audio/AudioManager.hpp"
#include "gui/GuiText.hpp"
#include "notify/AlertRouter.hpp"
#include "util/CsvWriter.hpp"
#include "util/JsonUtil.hpp"
#include "util/Localization.hpp"
#include "util/Logger.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#define DrawText aic::drawText

namespace aic {

namespace {
constexpr int   kWinW = 1280;
constexpr int   kWinH = 720;
constexpr char  kSettingsPath[] = "config/app_settings.json";
constexpr char  kScenarioDir[]  = "config/scenarios";
constexpr char  kDefaultScenarioId[] = "primitive_seed";

bool hasProjectMarkers(const std::filesystem::path& dir) {
    std::error_code ec;
    return std::filesystem::exists(dir / "config" / "app_settings.json", ec) &&
           std::filesystem::exists(dir / "assets", ec);
}

void normalizeWorkingDirectory() {
    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (!ec && hasProjectMarkers(cwd)) return;
    if (!ec && cwd.has_parent_path() && hasProjectMarkers(cwd.parent_path())) {
        std::filesystem::current_path(cwd.parent_path(), ec);
        return;
    }
    const std::filesystem::path designedRoot = "D:/AICivilization";
    if (hasProjectMarkers(designedRoot)) {
        std::filesystem::current_path(designedRoot, ec);
    }
}

std::string timestampForFile() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

// Maps an event routing tag to a sound effect (spec: SE on events).
SeId seForTag(const std::string& tag) {
    if (tag == "agent_born")      return SeId::AgentBorn;
    if (tag == "agent_dead")      return SeId::AgentDead;
    if (tag == "concept_created") return SeId::ConceptCreated;
    if (tag == "law_created")     return SeId::LawCreated;
    if (tag == "religion_created")return SeId::ReligionCreated;
    if (tag == "blackhole_start") return SeId::BlackHoleStart;
    if (tag == "gravity_start")   return SeId::GravityStart;
    if (tag == "whitehole_start") return SeId::WhiteHoleStart;
    if (tag == "collapse")        return SeId::Collapse;
    if (tag == "warning")         return SeId::Warning;
    return SeId::Count;  // no sound
}
} // namespace

bool App::init() {
    normalizeWorkingDirectory();
    Logger::init("logs/app.log");
    Logger::info("AICivilization starting");
    Logger::info("Working directory: " + std::filesystem::current_path().string());
    ensureRuntimeFiles();

    loadSettings();
    loadScenarios();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(kWinW, kWinH, "AICivilization");
    if (!IsWindowReady()) {
        Logger::error("Window initialization failed; aborting");
        return false;
    }
    windowOpen_ = true;
    SetExitKey(KEY_NULL);   // ESC must not quit; navigation is explicit
    SetTargetFPS(60);
    Logger::info("Window initialized (1280x720)");

    audio_.init(state_.settings.audio);
    notifications_.init(state_.settings.notify);
    gui_.init();
    updateScreenBgm();

    return true;
}

void App::run() {
    while (windowOpen_ && !WindowShouldClose() && !state_.quit) {
        const float dt = GetFrameTime();
        update(dt);

        BeginDrawing();
        ClearBackground(Color{ 10, 12, 20, 255 });
        gui_.draw(state_, audio_, sim_);
        drawHelpOverlay();
        notifications_.drawInApp();
        EndDrawing();

        if (gui_.takeSaveSettingsRequest()) {
            saveSettings();
        }
    }
    Logger::info("Main loop exited");
}

void App::update(float dt) {
    if (state_.startRequested) {
        startSimulation();
        state_.startRequested = false;
    }

    handleHotkeys();
    handleRequests();

    if (sim_.active()) {
        sim_.update(dt, state_.settings.sim.speed);
        dispatchEvents();
        flushCivilizationLog();
        const int snap = state_.settings.sim.autoSnapshotInterval;
        if (snap > 0 && sim_.world().cycle > 0 &&
            sim_.world().cycle % snap == 0 &&
            sim_.world().cycle != lastAutoSnapshotCycle_) {
            saveSimulation(true);
            lastAutoSnapshotCycle_ = sim_.world().cycle;
        }
        if (sim_.finished() && !finalReportWritten_) {
            writeFinalReport("logs/final_report.txt");
            finalReportWritten_ = true;
        }
    }

    updateScreenBgm();
    audio_.update(dt);
    notifications_.applySettings(state_.settings.notify);
    notifications_.update(dt, audio_);
}

void App::startSimulation() {
    if (state_.selectedScenario < 0 ||
        state_.selectedScenario >= (int)state_.scenarios.size()) {
        Logger::warn("Start requested with no scenario selected");
        return;
    }
    const ScenarioInfo& info = state_.scenarios[state_.selectedScenario];
    ScenarioConfig cfg;
    if (!loadScenarioConfig(info.file, cfg)) {
        Logger::warn("Failed to load scenario config: " + info.file);
        return;
    }
    // Adopt the scenario's seed/cycle defaults the first time it is chosen, so
    // the Settings screen reflects them; subsequent starts keep user edits.
    if (state_.selectedScenario != lastAppliedScenario_) {
        state_.settings.sim.seed = cfg.seed;
        state_.settings.sim.maxCycle = cfg.maxCycle;
        state_.settings.sim.autoSnapshotInterval = cfg.autoSnapshotInterval;
        lastAppliedScenario_ = state_.selectedScenario;
    }
    sim_.start(cfg, state_.settings);
    lastEventIndex_ = 0;
    lastCivilizationLogIndex_ = 0;
    lastAutoSnapshotCycle_ = -1;
    finalReportWritten_ = false;
    Logger::info("Simulation started for scenario: " + info.name);
}

void App::dispatchEventSe() {
    const auto& evs = sim_.world().events;
    if ((int)evs.size() < lastEventIndex_) lastEventIndex_ = 0;  // restarted
    for (int i = lastEventIndex_; i < (int)evs.size(); ++i) {
        const SeId se = seForTag(evs[i].tag);
        if (se != SeId::Count) audio_.playSe(se);
    }
    lastEventIndex_ = (int)evs.size();
}

void App::dispatchEvents() {
    const auto& evs = sim_.world().events;
    if ((int)evs.size() < lastEventIndex_) lastEventIndex_ = 0;
    for (int i = lastEventIndex_; i < (int)evs.size(); ++i) {
        const Event& e = evs[i];
        const SeId se = seForTag(e.tag);
        if (se != SeId::Count) audio_.playSe(se);
        CsvWriter::appendRow("logs/event_log.csv", {
            std::to_string(e.cycle), eventTypeName(e.type), e.name, e.source,
            e.target, std::to_string(e.value), e.message
        });
        notifications_.processEvent(e, audio_);
    }
    lastEventIndex_ = (int)evs.size();
}

void App::flushCivilizationLog() {
    const auto& rows = sim_.world().civilizationLog;
    if ((int)rows.size() < lastCivilizationLogIndex_) lastCivilizationLogIndex_ = 0;
    for (int i = lastCivilizationLogIndex_; i < (int)rows.size(); ++i) {
        const CivilizationSample& s = rows[i];
        CsvWriter::appendRow("logs/civilization_log.csv", {
            std::to_string(s.cycle), std::to_string(s.population), stageName(s.stage),
            std::to_string(s.resources), std::to_string(s.concepts), std::to_string(s.laws),
            std::to_string(s.religions), std::to_string(s.structures),
            std::to_string(s.stability), std::to_string(s.collapse_risk),
            std::to_string(s.unknown_score)
        });
    }
    lastCivilizationLogIndex_ = (int)rows.size();
}

void App::handleRequests() {
    if (state_.saveRequested) {
        state_.saveRequested = false;
        saveSimulation(false);
    }
    if (state_.snapshotRequested) {
        state_.snapshotRequested = false;
        saveSimulation(true);
    }
    if (state_.loadRequested) {
        state_.loadRequested = false;
        loadLatestSimulation();
    }
    if (state_.exportRequested) {
        state_.exportRequested = false;
        exportData();
    }
    if (state_.screenshotRequested) {
        state_.screenshotRequested = false;
        captureScreenshot("data/exports/screenshot_" + timestampForFile() + ".png");
    }
    if (state_.anomalyInjectionRequested) {
        state_.anomalyInjectionRequested = false;
        if (sim_.active()) {
            sim_.injectAnomaly(state_.requestedAnomaly, state_.requestedAnomalyStrength);
        } else {
            Logger::warn("Anomaly injection requested without an active simulation");
        }
    }
    if (state_.experimentRequested) {
        state_.experimentRequested = false;
        if (sim_.active()) sim_.startBhGvHhExperiment();
        else Logger::warn("BH/GV/HH experiment requested without an active simulation");
    }
    if (state_.testWindowsNotificationRequested) {
        state_.testWindowsNotificationRequested = false;
        notifications_.testWindows(audio_);
    }
    if (state_.testVoicevoxRequested) {
        state_.testVoicevoxRequested = false;
        notifications_.testVoicevox(audio_);
    }
    if (state_.testInAppAlertRequested) {
        state_.testInAppAlertRequested = false;
        notifications_.testInApp(audio_);
    }
}

void App::handleHotkeys() {
    if (state_.screen == Screen::Main && sim_.active()) {
        if (IsKeyPressed(KEY_SPACE)) sim_.togglePause();
        if (IsKeyPressed(KEY_R))     { sim_.reset(); lastEventIndex_ = (int)sim_.world().events.size(); }
        if (IsKeyPressed(KEY_S))     state_.snapshotRequested = true;
        if (IsKeyPressed(KEY_L))     state_.exportRequested = true;
    }
    if (IsKeyPressed(KEY_F1)) state_.helpVisible = !state_.helpVisible;
}

void App::updateScreenBgm() {
    switch (state_.screen) {
        case Screen::Main:
        case Screen::Agents:
        case Screen::Concepts:
        case Screen::Civilization:
        case Screen::AnomalyLab:
        case Screen::Timeline:
        case Screen::Logs:
            if (sim_.active()) {
                const World& w = sim_.world();
                bool bh = false, gv = false, hh = false, unk = false;
                for (const auto& a : w.activeAnomalies) {
                    bh = bh || a.type == AnomalyType::BlackHole;
                    gv = gv || a.type == AnomalyType::Gravity;
                    hh = hh || a.type == AnomalyType::WhiteHole;
                    unk = unk || isUnknownStress(a.type);
                }
                if (bh) audio_.playBgm(BgmState::BlackHole);
                else if (gv) audio_.playBgm(BgmState::Gravity);
                else if (hh) audio_.playBgm(BgmState::WhiteHole);
                else if (w.civ.stage == Stage::Collapsed) audio_.playBgm(BgmState::Collapse);
                else if (w.civ.stage == Stage::PostSingularity) audio_.playBgm(BgmState::PostSingularity);
                else if (unk || w.anomalyPressure > 0.25f) audio_.playBgm(BgmState::Anomaly);
                else if (w.civ.stage == Stage::Digital) audio_.playBgm(BgmState::Digital);
                else if ((int)w.civ.stage >= (int)Stage::Tribal) audio_.playBgm(BgmState::Growth);
                else audio_.playBgm(BgmState::Primitive);
            } else {
                audio_.playBgm(BgmState::Title);
            }
            break;
        case Screen::Settings:
            break;  // keep whatever was playing
        default:
            audio_.playBgm(BgmState::Title);
            break;
    }
}

void App::shutdown() {
    notifications_.shutdown();
    audio_.shutdown();
    if (windowOpen_) {
        CloseWindow();
        windowOpen_ = false;
    }
    Logger::info("AICivilization stopped cleanly");
    Logger::shutdown();
}

void App::ensureRuntimeFiles() {
    const char* dirs[] = {
        "assets/audio", "assets/sfx", "assets/fonts", "assets/textures",
        "cache/tts", "cache/fallback", "config/scenarios", "data/saves",
        "data/snapshots", "data/exports", "logs", "external/raylib",
        "external/raygui", "external/cpp-httplib"
    };
    for (const char* d : dirs) {
        std::error_code ec;
        std::filesystem::create_directories(d, ec);
    }
    CsvWriter::writeHeader("logs/event_log.csv",
        { "cycle", "type", "name", "source", "target", "value", "message" });
    CsvWriter::writeHeader("logs/civilization_log.csv",
        { "cycle", "population", "stage", "resources", "concepts", "laws",
          "religions", "structures", "stability", "collapse_risk", "unknown_score" });
    CsvWriter::writeHeader("logs/alert_log.csv",
        { "cycle", "level", "type", "title", "message", "windows", "tts", "result" });
    std::ofstream report("logs/final_report.txt", std::ios::binary | std::ios::trunc);
    report << "AICivilization 最終レポート\n\n結果:\n完了したシミュレーションはまだありません。\n";

    auto makeTexture = [](const char* path, Color color) {
        if (FileExists(path)) return;
        Image img = GenImageColor(16, 16, Color{ 0, 0, 0, 0 });
        ImageDrawCircle(&img, 8, 8, 7, color);
        ExportImage(img, path);
        UnloadImage(img);
    };
    makeTexture("assets/textures/particle.png", Color{ 180, 200, 255, 220 });
    makeTexture("assets/textures/agent.png", Color{ 120, 200, 255, 255 });
    makeTexture("assets/textures/concept.png", Color{ 200, 170, 240, 255 });
}

void App::saveSimulation(bool snapshot) {
    if (!sim_.active()) {
        Logger::warn("Save requested without an active simulation");
        return;
    }
    std::string path;
    if (snapshot) {
        std::ostringstream ss;
        ss << "data/snapshots/cycle_" << std::setw(6) << std::setfill('0')
           << sim_.world().cycle << ".json";
        path = ss.str();
    } else {
        path = "data/saves/save_" + timestampForFile() + ".json";
    }
    if (sim_.saveToFile(path)) {
        Logger::info(std::string(snapshot ? "Snapshot saved: " : "Save written: ") + path);
    }
}

void App::loadLatestSimulation() {
    std::filesystem::path best;
    std::filesystem::file_time_type bestTime{};
    bool found = false;
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator("data/saves", ec)) {
        if (!e.is_regular_file() || e.path().extension() != ".json") continue;
        const auto t = std::filesystem::last_write_time(e.path(), ec);
        if (!found || t > bestTime) {
            best = e.path();
            bestTime = t;
            found = true;
        }
    }
    if (!found) {
        Logger::warn("Load requested but no save files were found");
        return;
    }
    if (sim_.loadFromFile(best.string(), state_.settings)) {
        lastEventIndex_ = 0;
        lastCivilizationLogIndex_ = 0;
        lastAutoSnapshotCycle_ = -1;
        finalReportWritten_ = false;
        state_.setScreen(Screen::Main);
        Logger::info("Latest save loaded: " + best.string());
    }
}

void App::writeFinalReport(const std::string& path) {
    if (!sim_.active()) return;
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        Logger::warn("final_report write failed: " + path);
        return;
    }
    out << CivilizationAnalyzer::finalReport(sim_);
    Logger::info("final_report written: " + path);
}

void App::captureScreenshot(const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    TakeScreenshot(path.c_str());
    Logger::info("Screenshot exported: " + path);
}

void App::exportData() {
    std::error_code ec;
    std::filesystem::create_directories("data/exports", ec);
    std::filesystem::copy_file("logs/event_log.csv", "data/exports/event_log.csv",
                               std::filesystem::copy_options::overwrite_existing, ec);
    std::filesystem::copy_file("logs/civilization_log.csv", "data/exports/civilization_log.csv",
                               std::filesystem::copy_options::overwrite_existing, ec);
    std::filesystem::copy_file("logs/alert_log.csv", "data/exports/alert_log.csv",
                               std::filesystem::copy_options::overwrite_existing, ec);
    writeFinalReport("logs/final_report.txt");
    std::filesystem::copy_file("logs/final_report.txt", "data/exports/final_report.txt",
                               std::filesystem::copy_options::overwrite_existing, ec);
    captureScreenshot("data/exports/screenshot_" + timestampForFile() + ".png");
    Logger::info("CSV/final_report/screenshot export completed");
}

void App::drawHelpOverlay() {
    if (!state_.helpVisible) return;
    const int W = GetScreenWidth();
    Rectangle r{ (float)W - 420.0f, 96.0f, 390.0f, 210.0f };
    DrawRectangleRec(r, Color{ 12, 14, 22, 235 });
    DrawRectangleLinesEx(r, 1.0f, Color{ 80, 95, 130, 255 });
    DrawText("ヘルプ", (int)r.x + 16, (int)r.y + 12, 20, RAYWHITE);
    DrawText("スペース  一時停止 / 再開", (int)r.x + 16, (int)r.y + 48, 16, Color{ 210, 220, 240, 255 });
    DrawText("R      リセット", (int)r.x + 16, (int)r.y + 72, 16, Color{ 210, 220, 240, 255 });
    DrawText("S      スナップショット", (int)r.x + 16, (int)r.y + 96, 16, Color{ 210, 220, 240, 255 });
    DrawText("L      ログ出力", (int)r.x + 16, (int)r.y + 120, 16, Color{ 210, 220, 240, 255 });
    DrawText("ホイール  世界をズーム", (int)r.x + 16, (int)r.y + 144, 16, Color{ 210, 220, 240, 255 });
    DrawText("ドラッグ  世界を移動", (int)r.x + 16, (int)r.y + 168, 16, Color{ 210, 220, 240, 255 });
}

void App::loadSettings() {
    auto j = jsonutil::readFile(kSettingsPath);
    if (!j) {
        Logger::warn("Using default settings (config/app_settings.json not loaded)");
        return;
    }
    const Json& root = *j;
    Settings& s = state_.settings;

    if (root.contains("simulation") && root["simulation"].is_object()) {
        const Json& a = root["simulation"];
        s.sim.seed = jsonutil::getUInt(a, "seed", s.sim.seed);
        s.sim.maxCycle = jsonutil::getInt(a, "max_cycle", s.sim.maxCycle);
        s.sim.speed = jsonutil::getFloat(a, "speed", s.sim.speed);
        s.sim.autoSnapshotInterval =
            jsonutil::getInt(a, "auto_snapshot_interval", s.sim.autoSnapshotInterval);
    }
    if (root.contains("audio") && root["audio"].is_object()) {
        const Json& a = root["audio"];
        s.audio.bgmVolume = jsonutil::getFloat(a, "bgm_volume", s.audio.bgmVolume);
        s.audio.seVolume = jsonutil::getFloat(a, "se_volume", s.audio.seVolume);
        s.audio.mute = jsonutil::getBool(a, "mute", s.audio.mute);
        s.audio.duckDuringTts = jsonutil::getBool(a, "duck_during_tts", s.audio.duckDuringTts);
    }
    if (root.contains("notification") && root["notification"].is_object()) {
        const Json& a = root["notification"];
        s.notify.inApp = jsonutil::getBool(a, "in_app", s.notify.inApp);
        s.notify.windows = jsonutil::getBool(a, "windows", s.notify.windows);
        s.notify.tts = jsonutil::getBool(a, "tts", s.notify.tts);
        s.notify.voicevoxEndpoint =
            jsonutil::getString(a, "voicevox_endpoint", s.notify.voicevoxEndpoint);
        s.notify.speakerId = jsonutil::getInt(a, "speaker_id", s.notify.speakerId);
        s.notify.ttsVolume = jsonutil::getFloat(a, "tts_volume", s.notify.ttsVolume);
        s.notify.minLevel = jsonutil::getInt(a, "min_level", s.notify.minLevel);
        s.notify.cooldownSeconds =
            jsonutil::getInt(a, "cooldown_seconds", s.notify.cooldownSeconds);
    }
    if (root.contains("ui") && root["ui"].is_object()) {
        const Json& a = root["ui"];
        s.ui.language = jsonutil::getInt(a, "language", s.ui.language);
    }
    s.ui.language = 0;
    Logger::info("Settings loaded from config/app_settings.json");
}

void App::saveSettings() {
    const Settings& s = state_.settings;
    Json root;
    root["simulation"] = {
        { "seed", s.sim.seed },
        { "max_cycle", s.sim.maxCycle },
        { "speed", s.sim.speed },
        { "auto_snapshot_interval", s.sim.autoSnapshotInterval }
    };
    root["audio"] = {
        { "bgm_volume", s.audio.bgmVolume },
        { "se_volume", s.audio.seVolume },
        { "mute", s.audio.mute },
        { "duck_during_tts", s.audio.duckDuringTts }
    };
    root["notification"] = {
        { "in_app", s.notify.inApp },
        { "windows", s.notify.windows },
        { "tts", s.notify.tts },
        { "voicevox_endpoint", s.notify.voicevoxEndpoint },
        { "speaker_id", s.notify.speakerId },
        { "tts_volume", s.notify.ttsVolume },
        { "min_level", s.notify.minLevel },
        { "cooldown_seconds", s.notify.cooldownSeconds }
    };
    root["ui"] = {
        { "language", 0 }
    };
    if (jsonutil::writeFile(kSettingsPath, root)) {
        Logger::info("Settings saved to config/app_settings.json");
    }
}

void App::loadScenarios() {
    state_.scenarios.clear();
    std::error_code ec;
    if (!std::filesystem::exists(kScenarioDir, ec)) {
        Logger::warn("Scenario directory missing: config/scenarios");
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(kScenarioDir, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;
        auto j = jsonutil::readFile(entry.path().string());
        if (!j) continue;
        ScenarioInfo info;
        info.file = entry.path().string();
        info.id = jsonutil::getString(*j, "id", entry.path().stem().string());
        info.name = jsonutil::getString(*j, "name", info.id);
        info.description = jsonutil::getString(*j, "description", "");
        state_.scenarios.push_back(std::move(info));
    }
    std::sort(state_.scenarios.begin(), state_.scenarios.end(),
              [](const ScenarioInfo& a, const ScenarioInfo& b) {
                  return a.name < b.name;
              });
    if (!state_.scenarios.empty() && state_.selectedScenario < 0) {
        state_.selectedScenario = 0;
        for (int i = 0; i < (int)state_.scenarios.size(); ++i) {
        if (state_.scenarios[(size_t)i].id == kDefaultScenarioId) {
                state_.selectedScenario = i;
                break;
            }
        }
    }
    Logger::info("Loaded " + std::to_string(state_.scenarios.size()) + " scenarios");
}

} // namespace aic
