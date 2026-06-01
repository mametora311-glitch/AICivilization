#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "gui/GuiManager.hpp"

#include "audio/AudioManager.hpp"
#include "anomaly/Anomaly.hpp"
#include "gui/GuiText.hpp"
#include "simulation/Simulation.hpp"
#include "util/Localization.hpp"
#include "util/Logger.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#define DrawText aic::drawText
#define MeasureText aic::measureText

namespace aic {

namespace {
constexpr char kVersion[] = "v0.1.0";
constexpr int kBg = 0x0a0c14;  // window background reference
constexpr char kDefaultScenarioId[] = "primitive_seed";

int defaultScenarioIndex(const AppState& state) {
    for (int i = 0; i < (int)state.scenarios.size(); ++i) {
        if (state.scenarios[(size_t)i].id == kDefaultScenarioId) return i;
    }
    return state.scenarios.empty() ? -1 : 0;
}

// Minimal greedy word-wrap renderer (raylib has no built-in text wrapping).
void drawWrapped(const std::string& text, int x, int y, int width,
                 int fontSize, Color color) {
    std::string line;
    int curY = y;
    size_t i = 0;
    while (i <= text.size()) {
        // Extract next word (up to space or end).
        std::string word;
        while (i < text.size() && text[i] != ' ') word += text[i++];
        while (i < text.size() && text[i] == ' ') ++i;  // skip spaces
        const std::string test = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureText(test.c_str(), fontSize) > width) {
            DrawText(line.c_str(), x, curY, fontSize, color);
            curY += fontSize + 6;
            line = word;
        } else {
            line = test;
        }
        if (i >= text.size()) break;
    }
    if (!line.empty()) DrawText(line.c_str(), x, curY, fontSize, color);
}

Rectangle innerRect(Rectangle r, float padX, float padY) {
    return { r.x + padX, r.y + padY, r.width - padX * 2.0f, r.height - padY * 2.0f };
}

void popUtf8(std::string& text) {
    if (text.empty()) return;
    size_t pos = text.size() - 1;
    while (pos > 0 && (static_cast<unsigned char>(text[pos]) & 0xc0) == 0x80) {
        --pos;
    }
    text.erase(pos);
}

std::string fitText(std::string text, int fontSize, int width) {
    if (MeasureText(text.c_str(), fontSize) <= width) return text;
    while (!text.empty() && MeasureText((text + "...").c_str(), fontSize) > width) {
        popUtf8(text);
    }
    return text.empty() ? "..." : text + "...";
}

void updateListScroll(Rectangle area, int& scroll, int totalRows, int visibleRows) {
    const int maxStart = std::max(0, totalRows - visibleRows);
    scroll = std::clamp(scroll, 0, maxStart);
    if (CheckCollisionPointRec(GetMousePosition(), area)) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            scroll = std::clamp(scroll - (int)(wheel * 3.0f), 0, maxStart);
        }
    }
}

int updateTailScroll(Rectangle area, int& scroll, int totalRows, int visibleRows) {
    const int maxStart = std::max(0, totalRows - visibleRows);
    if (totalRows <= visibleRows) {
        scroll = -1;
        return 0;
    }
    if (CheckCollisionPointRec(GetMousePosition(), area)) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            if (scroll < 0) scroll = maxStart;
            scroll = std::clamp(scroll - (int)(wheel * 3.0f), 0, maxStart);
            if (scroll == maxStart) scroll = -1;
        }
    }
    return scroll < 0 ? maxStart : std::clamp(scroll, 0, maxStart);
}
} // namespace

void GuiManager::init() {
    initGuiText();
    GuiSetFont(guiFont());
    GuiSetStyle(DEFAULT, TEXT_SIZE, 19);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xeaf0ffff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, 0x7f8aa8ff);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x20283cff);
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0x62709cff);
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, 0x303b5cff);
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, 0x83a4e8ff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, 0xffffffff);
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, 0x4d8ddfff);
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, 0x9fc0ffff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, 0xffffffff);
    GuiSetStyle(DEFAULT, LINE_COLOR, 0x546080ff);
    GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
    (void)kBg;
}

bool GuiManager::takeSaveSettingsRequest() {
    const bool r = saveSettingsRequested_;
    saveSettingsRequested_ = false;
    return r;
}

void GuiManager::onEnterScreen(AppState& state, Screen previous) {
    languageActive_ = 0;
    state.settings.ui.language = 0;
    if (state.screen == Screen::Settings) {
        settingsReturn_ = (previous == Screen::Settings) ? Screen::Title : previous;
        seedI_ = static_cast<int>(state.settings.sim.seed);
        maxCycleI_ = state.settings.sim.maxCycle;
        autoSnapI_ = state.settings.sim.autoSnapshotInterval;
        speakerI_ = state.settings.notify.speakerId;
        cooldownI_ = state.settings.notify.cooldownSeconds;
        minLevelI_ = state.settings.notify.minLevel;
        std::strncpy(endpointBuf_, state.settings.notify.voicevoxEndpoint.c_str(),
                     sizeof(endpointBuf_) - 1);
        endpointBuf_[sizeof(endpointBuf_) - 1] = '\0';
    }
    if (state.screen == Screen::ScenarioSelect) {
        if (state.selectedScenario >= 0) scenarioActive_ = state.selectedScenario;
        scenarioActive_ = std::clamp(scenarioActive_, 0,
                                     std::max(0, (int)state.scenarios.size() - 1));
    }
}

void GuiManager::draw(AppState& state, AudioManager& audio, Simulation& sim) {
    if (firstFrame_ || state.screen != lastScreen_) {
        onEnterScreen(state, lastScreen_);
        lastScreen_ = state.screen;
        firstFrame_ = false;
    }

    switch (state.screen) {
        case Screen::Title:          drawTitle(state, audio); break;
        case Screen::ScenarioSelect: drawScenarioSelect(state, audio); break;
        case Screen::Settings:       drawSettings(state, audio); break;
        case Screen::Main:           mainPanel_.draw(state, audio, sim); break;
        default:                     drawDetailScreen(state, audio, sim); break;
    }
}

void GuiManager::updateAndDrawParticles() {
    const float W = (float)GetScreenWidth();
    const float H = (float)GetScreenHeight();
    if (!particlesInit_) {
        for (auto& p : particles_) {
            p.pos = { (float)GetRandomValue(0, (int)W), (float)GetRandomValue(0, (int)H) };
            p.vel = { GetRandomValue(-20, 20) / 100.0f, GetRandomValue(-20, 20) / 100.0f };
            p.r = GetRandomValue(4, 22) / 10.0f;
            p.a = (unsigned char)GetRandomValue(30, 120);
        }
        particlesInit_ = true;
    }
    const float dt = GetFrameTime();
    // Soft nebula gradient bands.
    DrawRectangleGradientV(0, 0, (int)W, (int)H, Color{ 10, 12, 22, 255 },
                           Color{ 16, 12, 28, 255 });
    for (auto& p : particles_) {
        p.pos.x += p.vel.x * 60.0f * dt;
        p.pos.y += p.vel.y * 60.0f * dt;
        if (p.pos.x < 0) p.pos.x += W; else if (p.pos.x > W) p.pos.x -= W;
        if (p.pos.y < 0) p.pos.y += H; else if (p.pos.y > H) p.pos.y -= H;
        DrawCircleV(p.pos, p.r, Color{ 150, 170, 230, p.a });
    }
}

void GuiManager::drawTitle(AppState& state, AudioManager& audio) {
    (void)audio;
    const int W = GetScreenWidth();
    const int H = GetScreenHeight();

    updateAndDrawParticles();

    const char* title = "AICivilization";
    const int tsize = 72;
    const int tw = MeasureText(title, tsize);
    DrawText(title, (W - tw) / 2, (int)(H * 0.17f), tsize,
             Color{ 244, 248, 255, 255 });

    const char* sub = "観測型文明シミュレーター";
    const int sw = MeasureText(sub, 22);
    DrawText(sub, (W - sw) / 2, (int)(H * 0.17f) + tsize + 10, 22,
             Color{ 176, 194, 238, 255 });

    const float bw = 340.0f, bh = 50.0f, gap = 12.0f;
    const float x = (W - bw) / 2.0f;
    float y = H * 0.43f;

    DrawText("表示言語", W - 244, 22, 16, Color{ 180, 190, 215, 255 });
    if (GuiDropdownBox({ (float)W - 146.0f, 18.0f, 116.0f, 30.0f },
                       "日本語", &languageActive_, languageEdit_)) {
        languageEdit_ = !languageEdit_;
    }
    languageActive_ = 0;
    state.settings.ui.language = 0;

    if (GuiButton({ x, y, bw, bh }, "新規シミュレーション")) {
        if (state.selectedScenario < 0 && !state.scenarios.empty())
            state.selectedScenario = defaultScenarioIndex(state);
        if (!state.scenarios.empty()) {
            state.startRequested = true;
            state.setScreen(Screen::Main);
        } else {
            state.setScreen(Screen::ScenarioSelect);
        }
    }
    y += bh + gap;

    if (GuiButton({ x, y, bw, bh }, "読込")) {
        state.loadRequested = true;
        state.setScreen(Screen::Main);
    }
    y += bh + gap;

    if (GuiButton({ x, y, bw, bh }, "シナリオ選択")) {
        state.setScreen(Screen::ScenarioSelect);
    }
    y += bh + gap;

    if (GuiButton({ x, y, bw, bh }, "設定")) {
        state.setScreen(Screen::Settings);
    }
    y += bh + gap;

    if (GuiButton({ x, y, bw, bh }, "終了")) {
        state.quit = true;
    }

    DrawText(kVersion, 16, H - 28, 18, Color{ 90, 100, 130, 255 });
}

void GuiManager::drawScenarioSelect(AppState& state, AudioManager& audio) {
    (void)audio;
    const int W = GetScreenWidth();
    const int H = GetScreenHeight();
    DrawRectangle(0, 0, W, H, Color{ 12, 14, 22, 255 });

    DrawText("シナリオ選択", 32, 28, 32, RAYWHITE);

    const float listX = 32, listY = 90, listW = 380, rowH = 44, gap = 8;
    for (int i = 0; i < (int)state.scenarios.size(); ++i) {
        Rectangle r = { listX, listY + i * (rowH + gap), listW, rowH };
        if (i == scenarioActive_) {
            DrawRectangleRec(r, Color{ 40, 70, 120, 255 });
        }
        if (GuiButton(r, state.scenarios[i].name.c_str())) {
            scenarioActive_ = i;
        }
    }

    // Description panel
    Rectangle dp = { listX + listW + 24, listY, W - (listX + listW + 24) - 32,
                     (float)H - listY - 110 };
    GuiGroupBox(dp, "説明");
    if (scenarioActive_ >= 0 && scenarioActive_ < (int)state.scenarios.size()) {
        const ScenarioInfo& sc = state.scenarios[scenarioActive_];
        DrawText(sc.name.c_str(), (int)dp.x + 16, (int)dp.y + 16, 24, RAYWHITE);
        drawWrapped(sc.description, (int)dp.x + 16, (int)dp.y + 52,
                    (int)dp.width - 32, 18, Color{ 170, 180, 205, 255 });
    }

    const float by = H - 64.0f;
    if (GuiButton({ 32, by, 160, 40 }, "戻る")) {
        state.setScreen(Screen::Title);
    }
    if (GuiButton({ 208, by, 200, 40 }, "開始")) {
        state.selectedScenario = scenarioActive_;
        state.startRequested = true;
        state.setScreen(Screen::Main);
    }
}

void GuiManager::drawSettings(AppState& state, AudioManager& audio) {
    const int W = GetScreenWidth();
    const int H = GetScreenHeight();
    DrawRectangle(0, 0, W, H, Color{ 12, 14, 22, 255 });
    DrawText("設定", 32, 24, 32, RAYWHITE);
    DrawText("表示言語", W - 244, 30, 16, Color{ 180, 190, 215, 255 });
    if (GuiDropdownBox({ (float)W - 146.0f, 26.0f, 116.0f, 30.0f },
                       "日本語", &languageActive_, languageEdit_)) {
        languageEdit_ = !languageEdit_;
    }
    languageActive_ = 0;
    state.settings.ui.language = 0;

    Settings& s = state.settings;
    const float colW = 360.0f;
    float x = 32.0f;

    // ----- Audio column -----
    float y = 80.0f;
    GuiGroupBox({ x, y, colW, 220 }, "音声");
    y += 18;
    GuiSliderBar({ x + 130, y, 180, 24 }, "BGM音量",
                 TextFormat("%.2f", s.audio.bgmVolume), &s.audio.bgmVolume, 0.0f, 1.0f);
    y += 34;
    GuiSliderBar({ x + 130, y, 180, 24 }, "SE音量",
                 TextFormat("%.2f", s.audio.seVolume), &s.audio.seVolume, 0.0f, 1.0f);
    y += 34;
    GuiCheckBox({ x + 16, y, 22, 22 }, "ミュート", &s.audio.mute);
    y += 34;
    GuiCheckBox({ x + 16, y, 22, 22 }, "TTS中BGMを下げる", &s.audio.duckDuringTts);
    // Apply audio settings live.
    audio.setBgmVolume(s.audio.bgmVolume);
    audio.setSeVolume(s.audio.seVolume);
    audio.setMuted(s.audio.mute);
    audio.setDuckEnabled(s.audio.duckDuringTts);

    // ----- Simulation column -----
    x = 32.0f + colW + 24.0f;
    y = 80.0f;
    GuiGroupBox({ x, y, colW, 220 }, "シミュレーション");
    y += 18;
    if (GuiValueBox({ x + 170, y, 150, 26 }, "シード  ", &seedI_, 0, 2000000000, seedEdit_))
        seedEdit_ = !seedEdit_;
    y += 36;
    if (GuiValueBox({ x + 170, y, 150, 26 }, "最大サイクル  ", &maxCycleI_, 1, 100000, maxCycleEdit_))
        maxCycleEdit_ = !maxCycleEdit_;
    y += 36;
    GuiSliderBar({ x + 130, y, 190, 24 }, "速度",
                 TextFormat("x%.2f", s.sim.speed), &s.sim.speed, 0.1f, 8.0f);
    y += 36;
    if (GuiValueBox({ x + 170, y, 150, 26 }, "自動スナップ  ", &autoSnapI_, 0, 100000, autoSnapEdit_))
        autoSnapEdit_ = !autoSnapEdit_;
    s.sim.seed = (unsigned)seedI_;
    s.sim.maxCycle = maxCycleI_;
    s.sim.autoSnapshotInterval = autoSnapI_;
    y += 36;
    DrawText("保存先: data/saves", (int)x + 16, (int)y, 14, Color{ 150, 160, 185, 255 });
    y += 18;
    DrawText("スナップショット先: data/snapshots", (int)x + 16, (int)y, 14, Color{ 150, 160, 185, 255 });

    // ----- Notification column -----
    x = 32.0f + (colW + 24.0f) * 2.0f;
    y = 80.0f;
    GuiGroupBox({ x, y, colW, 360 }, "通知");
    y += 18;
    GuiCheckBox({ x + 16, y, 22, 22 }, "画面内通知", &s.notify.inApp); y += 30;
    GuiCheckBox({ x + 16, y, 22, 22 }, "Windows通知", &s.notify.windows); y += 30;
    GuiCheckBox({ x + 16, y, 22, 22 }, "VOICEVOX読み上げ", &s.notify.tts); y += 34;
    GuiLabel({ x + 16, y, 110, 24 }, "エンドポイント");
    if (GuiTextBox({ x + 110, y, 230, 26 }, endpointBuf_, sizeof(endpointBuf_), endpointEdit_))
        endpointEdit_ = !endpointEdit_;
    s.notify.voicevoxEndpoint = endpointBuf_;
    y += 36;
    if (GuiValueBox({ x + 170, y, 100, 26 }, "話者ID  ", &speakerI_, 0, 1000, speakerEdit_))
        speakerEdit_ = !speakerEdit_;
    y += 34;
    GuiSliderBar({ x + 130, y, 190, 24 }, "TTS音量",
                 TextFormat("%.2f", s.notify.ttsVolume), &s.notify.ttsVolume, 0.0f, 1.0f);
    y += 34;
    if (GuiValueBox({ x + 170, y, 100, 26 }, "通知レベル  ", &minLevelI_, 0, 4, minLevelEdit_))
        minLevelEdit_ = !minLevelEdit_;
    y += 34;
    if (GuiValueBox({ x + 170, y, 100, 26 }, "間隔秒  ", &cooldownI_, 0, 600, cooldownEdit_))
        cooldownEdit_ = !cooldownEdit_;
    s.notify.speakerId = speakerI_;
    s.notify.minLevel = minLevelI_;
    s.notify.cooldownSeconds = cooldownI_;

    y += 40;
    if (GuiButton({ x + 16, y, 150, 28 }, "Windows通知テスト"))
        state.testWindowsNotificationRequested = true;
    if (GuiButton({ x + 174, y, 160, 28 }, "VOICEVOXテスト"))
        state.testVoicevoxRequested = true;
    y += 34;
    if (GuiButton({ x + 16, y, 318, 28 }, "画面内通知テスト"))
        state.testInAppAlertRequested = true;

    // ----- Footer buttons -----
    const float by = H - 60.0f;
    if (GuiButton({ 32, by, 160, 40 }, "戻る")) {
        state.setScreen(settingsReturn_);
    }
    if (GuiButton({ 208, by, 200, 40 }, "設定保存")) {
        saveSettingsRequested_ = true;
    }
}

void GuiManager::drawTopNav(AppState& state, const char* title) {
    const int W = GetScreenWidth();
    DrawRectangle(0, 0, W, 48, Color{ 22, 26, 38, 255 });
    DrawText(title, 16, 14, 22, RAYWHITE);
    if (GuiButton({ (float)W - 96.0f, 10.0f, 84.0f, 28.0f }, "< タイトル"))
        state.setScreen(Screen::Title);

    const char* nav[] = { "観測", "エージェント", "概念", "文明",
                          "異常ラボ", "タイムライン", "ログ", "設定" };
    const Screen screens[] = { Screen::Main, Screen::Agents, Screen::Concepts,
                               Screen::Civilization, Screen::AnomalyLab,
                               Screen::Timeline, Screen::Logs, Screen::Settings };
    const float x0 = 16.0f, y = 56.0f, gap = 6.0f;
    const float bw = ((float)W - 32.0f - gap * 7.0f) / 8.0f;
    for (int i = 0; i < 8; ++i) {
        const bool active = state.screen == screens[i];
        if (active) GuiSetState(STATE_PRESSED);
        if (GuiButton({ x0 + i * (bw + gap), y, bw, 30.0f }, nav[i]))
            state.setScreen(screens[i]);
        if (active) GuiSetState(STATE_NORMAL);
    }
}

void GuiManager::drawDetailScreen(AppState& state, AudioManager& audio, Simulation& sim) {
    (void)audio;
    const int W = GetScreenWidth();
    const int H = GetScreenHeight();
    DrawRectangle(0, 0, W, H, Color{ 12, 14, 22, 255 });
    drawTopNav(state, screenName(state.screen));

    Rectangle content{ 24.0f, 104.0f, (float)W - 48.0f, (float)H - 124.0f };
    if (!sim.active() && state.screen != Screen::Logs) {
        GuiGroupBox(content, "シミュレーション未開始");
        DrawText("新規またはシナリオ選択から開始してください。",
                 (int)content.x + 16, (int)content.y + 28, 18,
                 Color{ 150, 160, 185, 255 });
        return;
    }

    switch (state.screen) {
        case Screen::Agents:       drawAgentsScreen(sim, content); break;
        case Screen::Concepts:     drawConceptsScreen(sim, content); break;
        case Screen::Civilization: drawCivilizationScreen(sim, content); break;
        case Screen::AnomalyLab:   drawAnomalyLabScreen(state, sim, content); break;
        case Screen::Timeline:     drawTimelineScreen(sim, content); break;
        case Screen::Logs:         drawLogsScreen(state, sim, content); break;
        default:                   break;
    }
}

void GuiManager::drawAgentsScreen(Simulation& sim, Rectangle content) {
    const World& w = sim.world();
    GuiGroupBox(content, "エージェント");
    agentActive_ = std::clamp(agentActive_, 0, std::max(0, (int)w.agents.size() - 1));

    Rectangle list{ content.x + 12.0f, content.y + 20.0f, content.width * 0.62f,
                    content.height - 32.0f };
    Rectangle detail{ list.x + list.width + 18.0f, list.y,
                      content.width - list.width - 42.0f, list.height };
    GuiGroupBox(list, "エージェント一覧");
    GuiGroupBox(detail, "エージェント詳細");

    const int x = (int)list.x + 12;
    int y = (int)list.y + 26;
    const int headerFont = 16;
    const int rowFont = 15;
    const int rowH = 24;
    const int colName = x;
    const int colLife = x + 104;
    const int colInt = x + 150;
    const int colEmo = x + 196;
    const int colAdapt = x + 242;
    const int colCreative = x + 288;
    const int colSoc = x + 334;
    const int colFaith = x + 380;
    const int colMemory = x + 426;
    const int colResource = x + 472;
    const int colState = x + 522;
    auto header = [&](const char* label, int cx) {
        DrawText(label, cx, y, headerFont, Color{ 190, 202, 228, 255 });
    };
    header("名前", colName);
    header("生命", colLife);
    header("知性", colInt);
    header("感情", colEmo);
    header("適応", colAdapt);
    header("創造", colCreative);
    header("社会", colSoc);
    header("信仰", colFaith);
    header("記憶", colMemory);
    header("資源", colResource);
    header("状態", colState);
    y += 28;
    Rectangle listBody{ list.x + 8.0f, (float)y - 4.0f, list.width - 16.0f,
                        list.y + list.height - y - 8.0f };
    const int rows = std::max(1, (int)(listBody.height) / rowH);
    updateListScroll(listBody, agentScroll_, (int)w.agents.size(), rows);
    int shown = 0;
    BeginScissorMode((int)listBody.x, (int)listBody.y,
                     (int)listBody.width, (int)listBody.height);
    for (int idx = agentScroll_; idx < (int)w.agents.size(); ++idx) {
        const auto& a = w.agents[idx];
        if (shown >= rows) break;
        Rectangle row{ (float)x - 4.0f, (float)y - 3.0f, list.width - 24.0f, 22.0f };
        if (idx == agentActive_) DrawRectangleRec(row, Color{ 38, 55, 84, 180 });
        if (CheckCollisionPointRec(GetMousePosition(), row) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            agentActive_ = idx;
        Color c = agentStateColor(a.state);
        const std::string aname = fitText(a.name, rowFont, colLife - colName - 12);
        DrawText(aname.c_str(), colName, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.life), colLife, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.intelligence), colInt, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.emotion), colEmo, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.adaptability), colAdapt, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.creativity), colCreative, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.sociality), colSoc, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.faith_bias), colFaith, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.memory), colMemory, y, rowFont, c);
        DrawText(TextFormat("%.0f", a.resource), colResource, y, rowFont, c);
        DrawText(jpAgentStateName(a.state), colState, y, rowFont, c);
        y += rowH;
        ++shown;
    }
    EndScissorMode();

    if (!w.agents.empty()) {
        const Agent& a = w.agents[(size_t)agentActive_];
        int dx = (int)detail.x + 14;
        int dy = (int)detail.y + 24;
        DrawText(a.name.c_str(), dx, dy, 22, RAYWHITE); dy += 34;
        DrawText(TextFormat("状態: %s    資源: %.0f    記憶: %.0f",
                 jpAgentStateName(a.state), a.resource, a.memory),
                 dx, dy, 15, Color{ 210, 220, 240, 255 }); dy += 34;

        auto bar = [&](const char* label, float value, Color color) {
            DrawText(label, dx, dy + 2, 15, Color{ 190, 202, 228, 255 });
            DrawRectangle(dx + 110, dy, 190, 18, Color{ 28, 32, 45, 255 });
            DrawRectangle(dx + 110, dy, (int)(190.0f * std::clamp(value, 0.0f, 100.0f) / 100.0f),
                          18, color);
            DrawText(TextFormat("%.0f", value), dx + 310, dy + 1, 15, Color{ 225, 234, 248, 255 });
            dy += 30;
        };
        bar("生命", a.life, Color{ 120, 220, 150, 255 });
        bar("知性", a.intelligence, Color{ 120, 180, 240, 255 });
        bar("適応", a.adaptability, Color{ 220, 190, 100, 255 });
        bar("創造", a.creativity, Color{ 190, 130, 235, 255 });
        bar("信仰傾向", a.faith_bias, Color{ 235, 190, 110, 255 });
    }
}

void GuiManager::drawConceptsScreen(Simulation& sim, Rectangle content) {
    const World& w = sim.world();
    GuiGroupBox(content, "概念");
    conceptActive_ = std::clamp(conceptActive_, 0, std::max(0, (int)w.concepts.size() - 1));
    Rectangle list{ content.x + 12.0f, content.y + 20.0f, content.width * 0.58f,
                    content.height - 32.0f };
    Rectangle history{ list.x + list.width + 18.0f, list.y,
                       content.width - list.width - 42.0f, list.height };
    GuiGroupBox(list, "概念ツリー");
    GuiGroupBox(history, "複合履歴");
    const int x = (int)list.x + 12;
    int y = (int)list.y + 24;
    const int headerFont = 16;
    const int rowFont = 15;
    const int rowH = 24;
    const int colName = x;
    const int colStrength = x + 260;
    const int colOrigin = x + 334;
    const int colParents = x + 410;
    DrawText("名前", colName, y, headerFont, Color{ 190, 202, 228, 255 });
    DrawText("強度", colStrength, y, headerFont, Color{ 190, 202, 228, 255 });
    DrawText("起源", colOrigin, y, headerFont, Color{ 190, 202, 228, 255 });
    DrawText("親", colParents, y, headerFont, Color{ 190, 202, 228, 255 });
    y += 28;
    Rectangle listBody{ list.x + 8.0f, (float)y - 4.0f, list.width - 16.0f,
                        list.y + list.height - y - 8.0f };
    const int rows = std::max(1, (int)(listBody.height) / rowH);
    updateListScroll(listBody, conceptScroll_, (int)w.concepts.size(), rows);
    int shown = 0;
    BeginScissorMode((int)listBody.x, (int)listBody.y,
                     (int)listBody.width, (int)listBody.height);
    for (int idx = conceptScroll_; idx < (int)w.concepts.size(); ++idx) {
        const auto& c = w.concepts[idx];
        if (shown >= rows) break;
        Rectangle row{ (float)x - 4.0f, (float)y - 3.0f, list.width - 24.0f, 22.0f };
        if (idx == conceptActive_) DrawRectangleRec(row, Color{ 48, 42, 82, 180 });
        if (CheckCollisionPointRec(GetMousePosition(), row) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            conceptActive_ = idx;
        std::string parents;
        for (size_t i = 0; i < c.parents.size(); ++i) {
            if (i) parents += "+";
            parents += std::to_string(c.parents[i]);
        }
        const std::string cname = jpDomainName(c.name);
        const std::string displayName = fitText(cname, rowFont, colStrength - colName - 14);
        const std::string displayParents = fitText(parents, rowFont,
                                                   (int)(list.x + list.width) - colParents - 16);
        DrawText(displayName.c_str(), colName, y, rowFont, Color{ 215, 204, 244, 255 });
        DrawText(TextFormat("%.2f", c.strength), colStrength, y, rowFont, Color{ 215, 204, 244, 255 });
        DrawText(TextFormat("%ld", c.origin_cycle), colOrigin, y, rowFont, Color{ 190, 182, 220, 255 });
        DrawText(displayParents.c_str(), colParents, y, rowFont, Color{ 190, 182, 220, 255 });
        y += rowH;
        ++shown;
    }
    EndScissorMode();

    int hx = (int)history.x + 12;
    int hy = (int)history.y + 24;
    Rectangle historyBody{ history.x + 8.0f, (float)hy - 4.0f, history.width - 16.0f,
                           history.y + history.height - hy - 8.0f };
    int compoundRows = std::max(1, (int)(historyBody.height) / 42);
    int compoundTotal = 0;
    for (const auto& c : w.concepts) if (!c.parents.empty()) ++compoundTotal;
    updateListScroll(historyBody, conceptHistoryScroll_, compoundTotal, compoundRows);
    int count = 0;
    int seen = 0;
    BeginScissorMode((int)historyBody.x, (int)historyBody.y,
                     (int)historyBody.width, (int)historyBody.height);
    for (const auto& c : w.concepts) {
        if (c.parents.empty()) continue;
        if (seen++ < conceptHistoryScroll_) continue;
        if (count >= compoundRows) break;
        std::string parents;
        for (size_t i = 0; i < c.parents.size(); ++i) {
            if (i) parents += " + ";
            parents += "#" + std::to_string(c.parents[i]);
        }
        const std::string cname = jpDomainName(c.name);
        const std::string displayName = fitText(cname, 15, (int)historyBody.width - 112);
        DrawText(TextFormat("サイクル %ld  %s", c.origin_cycle, displayName.c_str()),
                 hx, hy, 15, Color{ 220, 210, 248, 255 });
        DrawText(TextFormat("親: %s", fitText(parents, 13, (int)historyBody.width - 36).c_str()), hx + 12, hy + 19, 13,
                 Color{ 160, 150, 190, 255 });
        hy += 42;
        ++count;
    }
    if (count == 0) DrawText("複合概念はまだありません。", hx, hy, 15,
                              Color{ 150, 160, 185, 255 });
    EndScissorMode();
}

void GuiManager::drawCivilizationScreen(Simulation& sim, Rectangle content) {
    const World& w = sim.world();
    GuiGroupBox(content, "文明");
    int x = (int)content.x + 16;
    int y = (int)content.y + 24;
    DrawText(TextFormat("段階: %s", jpStageName(w.civ.stage)), x, y, 22, RAYWHITE); y += 34;
    DrawText(TextFormat("人口: %d    資源: %.0f", w.civ.population, w.civ.resources),
             x, y, 18, Color{ 205, 215, 235, 255 }); y += 28;
    DrawText(TextFormat("概念: %d    法: %d    宗教: %d    構造物: %d",
             w.civ.concept_count, w.civ.law_count, w.civ.religion_count,
             w.civ.structure_count), x, y, 18, Color{ 205, 215, 235, 255 }); y += 28;
    DrawText(TextFormat("安定度: %.1f%%    崩壊危険度: %.1f%%    進化: %.1f    UNKNOWN: %.1f",
             w.civ.stability, w.civ.collapse_risk, w.civ.evolution_score,
             w.civ.unknown_score), x, y, 18, Color{ 205, 215, 235, 255 }); y += 42;

    Rectangle body{ content.x + 14.0f, (float)y - 4.0f, content.width - 28.0f,
                    content.y + content.height - y - 12.0f };
    const int visibleRows = std::max(1, (int)body.height / 20);
    const int leftRows = 3 + (int)w.laws.size() + (int)w.religions.size();
    const int rightRows = 1 + (int)w.structures.size();
    updateListScroll(body, civScroll_, std::max(leftRows, rightRows), visibleRows);

    BeginScissorMode((int)body.x, (int)body.y, (int)body.width, (int)body.height);
    int ly = (int)body.y + 4 - civScroll_ * 20;
    DrawText("法", x, ly, 18, Color{ 140, 190, 245, 255 }); ly += 24;
    for (const auto& law : w.laws) {
        const std::string text = fitText(jpDomainName(law), 15, (int)(content.width * 0.42f));
        DrawText(text.c_str(), x + 16, ly, 15, Color{ 180, 210, 245, 255 });
        ly += 20;
    }
    ly += 12;
    DrawText("宗教", x, ly, 18, Color{ 235, 190, 110, 255 }); ly += 24;
    for (const auto& r : w.religions) {
        const std::string text = fitText(jpDomainName(r), 15, (int)(content.width * 0.42f));
        DrawText(text.c_str(), x + 16, ly, 15, Color{ 235, 205, 150, 255 });
        ly += 20;
    }

    int sx = (int)content.x + (int)(content.width * 0.48f);
    int sy = (int)body.y + 4 - civScroll_ * 20;
    DrawText("構造物", sx, sy, 18, Color{ 220, 205, 170, 255 }); sy += 24;
    for (const auto& s : w.structures) {
        const std::string text = fitText(jpDomainName(s.name), 15, 150);
        DrawText(TextFormat("%s  サイクル %ld", text.c_str(), s.origin_cycle),
                 sx + 16, sy, 15, Color{ 210, 200, 175, 255 });
        sy += 20;
    }
    EndScissorMode();
}

void GuiManager::drawAnomalyLabScreen(AppState& state, Simulation& sim, Rectangle content) {
    World& w = sim.world();
    GuiGroupBox(content, "異常ラボ");
    float x = content.x + 16.0f;
    float y = content.y + 24.0f;
    GuiSliderBar({ x + 120.0f, y, 260.0f, 24.0f }, "強度",
                 TextFormat("%.0f", anomalyStrength_), &anomalyStrength_, 1.0f, 100.0f);
    state.requestedAnomalyStrength = anomalyStrength_;
    y += 44.0f;

    const char* ids[] = { "BlackHole", "Gravity", "WhiteHole", "OutsideSignal",
                          "TimeFracture", "MemoryEater", "Eclipse", "Mirror",
                          "NoiseRain" };
    const char* names[] = { "ブラックホール", "重力", "ホワイトホール", "外部信号",
                            "時間断裂", "記憶喰らい", "蝕", "鏡",
                            "ノイズ雨" };
    for (int i = 0; i < 9; ++i) {
        Rectangle b{ x + (i % 3) * 170.0f, y + (i / 3) * 38.0f, 158.0f, 30.0f };
        if (GuiButton(b, names[i])) {
            state.requestedAnomaly = ids[i];
            state.anomalyInjectionRequested = true;
        }
    }
    y += 126.0f;
    if (GuiButton({ x, y, 310.0f, 34.0f }, "BH → GV → HH 実験開始")) {
        state.experimentRequested = true;
    }
    y += 54.0f;

    const std::string expResult = jpExperimentResult(w.experimentResult);
    DrawText(TextFormat("実験結果: %s", expResult.c_str()),
             (int)x, (int)y, 18, Color{ 205, 215, 235, 255 });
    y += 30.0f;
    DrawText("発生中の異常", (int)x, (int)y, 18, Color{ 205, 215, 235, 255 });
    y += 24.0f;
    for (const auto& a : w.activeAnomalies) {
        DrawText(TextFormat("%s  強度 %.0f  残り %d",
                 jpAnomalyTypeName(a.type), a.strength, a.remainingCycles),
                 (int)x + 16, (int)y, 15, anomalyColor(a.type));
        y += 20.0f;
    }
}

void GuiManager::drawTimelineScreen(Simulation& sim, Rectangle content) {
    const World& w = sim.world();
    GuiGroupBox(content, "タイムライン");
    Rectangle body = innerRect(content, 14.0f, 22.0f);
    const int rows = std::max(1, (int)(body.height - 8.0f) / 22);
    int start = updateTailScroll(body, timelineScroll_, (int)w.events.size(), rows);
    int x = (int)body.x + 2;
    int y = (int)body.y + 2;
    BeginScissorMode((int)body.x, (int)body.y, (int)body.width, (int)body.height);
    for (int i = start; i < (int)w.events.size() && i < start + rows; ++i) {
        const Event& e = w.events[i];
        const std::string msg = fitText(e.message, 15, (int)body.width - 210);
        DrawText(TextFormat("サイクル %04ld  %-12s  %s",
                 e.cycle, jpEventTypeName(e.type), msg.c_str()),
                 x, y, 15, Color{ 190, 200, 225, 255 });
        y += 22;
    }
    EndScissorMode();
}

void GuiManager::drawLogsScreen(AppState& state, Simulation& sim, Rectangle content) {
    GuiGroupBox(content, "ログ");
    int x = (int)content.x + 16;
    int y = (int)content.y + 24;
    const char* filters[] = { "すべて", "エージェント", "概念", "法", "宗教",
                              "構造物", "異常", "文明", "システム" };
    const EventType filterTypes[] = { EventType::System, EventType::Agent, EventType::Concept,
                                      EventType::Law, EventType::Religion, EventType::Structure,
                                      EventType::Anomaly, EventType::Civilization, EventType::System };
    for (int i = 0; i < 9; ++i) {
        if (i == logFilter_) GuiSetState(STATE_PRESSED);
        if (GuiButton({ (float)x + i * 94.0f, (float)y, 88.0f, 28.0f }, filters[i])) {
            logFilter_ = i;
            logScroll_ = -1;
        }
        if (i == logFilter_) GuiSetState(STATE_NORMAL);
    }
    y += 42;
    if (GuiButton({ (float)x, (float)y, 140.0f, 30.0f }, "CSV出力")) {
        state.exportRequested = true;
    }
    y += 44;

    DrawText("ファイル: logs/app.log, event_log.csv, civilization_log.csv, alert_log.csv, final_report.txt",
             x, y, 15, Color{ 180, 190, 215, 255 });
    y += 28;

    if (!sim.active()) {
        DrawText("メモリ上に実行中シミュレーションのログはありません。", x, y, 15, Color{ 150, 160, 185, 255 });
        return;
    }

    const World& w = sim.world();
    DrawText(TextFormat("イベント: %d    文明サンプル: %d",
             (int)w.events.size(), (int)w.civilizationLog.size()),
             x, y, 16, Color{ 180, 190, 215, 255 });
    y += 28;

    Rectangle body{ content.x + 14.0f, (float)y - 4.0f, content.width - 28.0f,
                    content.y + content.height - y - 10.0f };
    const int rows = std::max(1, (int)(body.height) / 20);
    std::vector<const Event*> filtered;
    for (const auto& e : w.events) {
        if (logFilter_ == 0 || e.type == filterTypes[logFilter_])
            filtered.push_back(&e);
    }
    int start = updateTailScroll(body, logScroll_, (int)filtered.size(), rows);
    int ly = (int)body.y + 2;
    BeginScissorMode((int)body.x, (int)body.y, (int)body.width, (int)body.height);
    for (int i = start; i < (int)filtered.size() && i < start + rows; ++i) {
        const Event& e = *filtered[(size_t)i];
        const std::string msg = fitText(e.message, 14, (int)body.width - 190);
        DrawText(TextFormat("[サイクル %04ld] %-12s %s",
                 e.cycle, jpEventTypeName(e.type), msg.c_str()),
                 x, ly, 14, Color{ 205, 215, 235, 255 });
        ly += 20;
    }
    EndScissorMode();
}

} // namespace aic
