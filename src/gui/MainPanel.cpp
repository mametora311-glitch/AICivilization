#include "gui/MainPanel.hpp"

#include "raygui.h"

#include "app/AppState.hpp"
#include "audio/AudioManager.hpp"
#include "gui/GuiText.hpp"
#include "simulation/Simulation.hpp"
#include "util/Localization.hpp"

#include <algorithm>
#include <cmath>

#define DrawText aic::drawText

namespace aic {

namespace {
constexpr float kHeaderH = 54.0f;
constexpr float kControlH = 88.0f;
constexpr float kEventLogH = 172.0f;
constexpr float kLeftW = 336.0f;

void popUtf8(std::string& text) {
    if (text.empty()) return;
    size_t pos = text.size() - 1;
    while (pos > 0 && (static_cast<unsigned char>(text[pos]) & 0xc0) == 0x80) --pos;
    text.erase(pos);
}

std::string fitText(std::string text, int fontSize, int width) {
    if (measureText(text.c_str(), fontSize) <= width) return text;
    while (!text.empty() && measureText((text + "...").c_str(), fontSize) > width) {
        popUtf8(text);
    }
    return text.empty() ? "..." : text + "...";
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

Rectangle MainPanel::headerRect() {
    return { 0.0f, 0.0f, (float)GetScreenWidth(), kHeaderH };
}
Rectangle MainPanel::controlBarRect() {
    return { 0.0f, kHeaderH, (float)GetScreenWidth(), kControlH };
}
Rectangle MainPanel::leftPanelRect() {
    const float top = kHeaderH + kControlH;
    return { 0.0f, top, kLeftW, GetScreenHeight() - top - kEventLogH };
}
Rectangle MainPanel::worldViewRect() {
    const float top = kHeaderH + kControlH;
    return { kLeftW, top, GetScreenWidth() - kLeftW,
             GetScreenHeight() - top - kEventLogH };
}
Rectangle MainPanel::eventLogRect() {
    return { 0.0f, GetScreenHeight() - kEventLogH,
             (float)GetScreenWidth(), kEventLogH };
}

void MainPanel::draw(AppState& state, AudioManager& audio, Simulation& sim) {
    // Refit the camera whenever a fresh simulation becomes active.
    if (sim.active() && !wasActive_) {
        camInit_ = false;
        eventLogScroll_ = -1;
    }
    wasActive_ = sim.active();

    if (!sim.active()) {
        drawEmptyFrame(state, audio);
        return;
    }

    drawHeader(sim);
    drawControls(state, audio, sim, controlBarRect());
    drawLeftPanel(sim, leftPanelRect());
    drawWorldView(sim, worldViewRect());
    drawSpeedBar(state, sim, worldViewRect());
    drawEventLog(sim, eventLogRect());
}

void MainPanel::drawHeader(Simulation& sim) {
    const World& w = sim.world();
    Rectangle hr = headerRect();
    DrawRectangleRec(hr, Color{ 22, 26, 38, 255 });
    DrawLine(0, (int)kHeaderH, GetScreenWidth(), (int)kHeaderH, Color{ 60, 70, 90, 255 });

    const char* status = sim.finished() ? "完了"
                       : (sim.running() ? "実行中" : "一時停止");
    DrawText(TextFormat("サイクル: %ld / %d    段階: %s    人口: %d    安定度: %.0f%%    崩壊危険度: %.0f%%    [%s]",
                        w.cycle, sim.maxCycle(), jpStageName(w.civ.stage),
                        w.civ.population, w.civ.stability, w.civ.collapse_risk, status),
             16, 15, 20, Color{ 238, 244, 255, 255 });
    // The "< Title" button is drawn in drawControls().
}

void MainPanel::drawControls(AppState& state, AudioManager& audio, Simulation& sim, Rectangle cr) {
    DrawRectangleRec(cr, Color{ 18, 20, 30, 255 });

    // The "< Title" button lives in the header; wire it here for clarity.
    if (GuiButton({ (float)GetScreenWidth() - 96.0f, 10.0f, 84.0f, 28.0f }, "< タイトル")) {
        state.setScreen(Screen::Title);
    }

    const char* labels[] = { "開始", "一時停止", "1歩", "リセット", "保存",
                             "読込", "スナップ", "CSV出力", "異常注入",
                             "BGM", "設定" };
    const int n = 11;
    const float pad = 6.0f;
    const float bw = (cr.width - pad * (n + 1)) / n;
    for (int i = 0; i < n; ++i) {
        Rectangle b = { pad + i * (bw + pad), cr.y + 7.0f, bw, 32.0f };
        if (GuiButton(b, labels[i])) {
            switch (i) {
                case 0: sim.resume(); break;
                case 1: sim.pause(); break;
                case 2: sim.step(); break;
                case 3: sim.reset(); break;
                case 4: state.saveRequested = true; break;
                case 5: state.loadRequested = true; break;
                case 6: state.snapshotRequested = true; break;
                case 7: state.exportRequested = true; break;
                case 8: state.setScreen(Screen::AnomalyLab); break;
                case 9: audio.setMuted(!audio.isMuted()); break;
                case 10: state.setScreen(Screen::Settings); break;
                default: break;
            }
        }
    }

    const char* nav[] = { "観測", "エージェント", "概念", "文明",
                          "異常ラボ", "タイムライン", "ログ" };
    const Screen screens[] = { Screen::Main, Screen::Agents, Screen::Concepts,
                               Screen::Civilization, Screen::AnomalyLab,
                               Screen::Timeline, Screen::Logs };
    const int nn = 7;
    const float nbw = (cr.width - pad * (nn + 1)) / nn;
    for (int i = 0; i < nn; ++i) {
        Rectangle b = { pad + i * (nbw + pad), cr.y + 47.0f, nbw, 32.0f };
        const bool active = state.screen == screens[i];
        if (active) GuiSetState(STATE_PRESSED);
        if (GuiButton(b, nav[i])) state.setScreen(screens[i]);
        if (active) GuiSetState(STATE_NORMAL);
    }
}

void MainPanel::drawLeftPanel(Simulation& sim, Rectangle lp) {
    const World& w = sim.world();
    DrawRectangleRec(lp, Color{ 16, 18, 26, 255 });

    const float secH = (lp.height - 24.0f) / 3.0f;
    Rectangle civBox = { lp.x + 8, lp.y + 8, lp.width - 16, secH };
    Rectangle agBox  = { lp.x + 8, civBox.y + secH + 4, lp.width - 16, secH };
    Rectangle coBox  = { lp.x + 8, agBox.y + secH + 4, lp.width - 16, secH };
    GuiGroupBox(civBox, "文明");
    GuiGroupBox(agBox, "エージェント");
    GuiGroupBox(coBox, "概念");

    int x = (int)civBox.x + 12;
    int y = (int)civBox.y + 16;
    const int civFont = civBox.height < 136.0f ? 15 : 16;
    const int civLine = civBox.height < 136.0f ? 18 : 20;
    const int bodyW = (int)civBox.width - 24;
    const Color tc{ 220, 228, 246, 255 };
    BeginScissorMode((int)civBox.x + 2, (int)civBox.y + 14,
                     (int)civBox.width - 4, (int)civBox.height - 18);
    DrawText(fitText(TextFormat("段階: %s", jpStageName(w.civ.stage)), civFont, bodyW).c_str(), x, y, civFont, tc); y += civLine;
    DrawText(TextFormat("人口: %d", w.civ.population), x, y, civFont, tc); y += civLine;
    DrawText(TextFormat("資源: %.0f", w.civ.resources), x, y, civFont, tc); y += civLine;
    DrawText(TextFormat("概念: %d    法: %d", w.civ.concept_count, w.civ.law_count), x, y, civFont, tc); y += civLine;
    DrawText(TextFormat("宗教: %d    構造物: %d", w.civ.religion_count, w.civ.structure_count), x, y, civFont, tc); y += civLine;
    DrawText(TextFormat("安定度: %.0f%%    危険度: %.0f%%", w.civ.stability, w.civ.collapse_risk), x, y, civFont, tc);
    EndScissorMode();

    // Agent list (first entries)
    x = (int)agBox.x + 12; y = (int)agBox.y + 14;
    const int rowFont = 15;
    const int rowH = 19;
    const int maxRows = std::max(1, (int)(secH - 28) / rowH);
    int shown = 0;
    BeginScissorMode((int)agBox.x + 2, (int)agBox.y + 14,
                     (int)agBox.width - 4, (int)agBox.height - 18);
    for (const auto& a : w.agents) {
        if (shown >= maxRows) break;
        const std::string row = fitText(
            TextFormat("%s  生命:%.0f  %s", a.name.c_str(), a.life, jpAgentStateName(a.state)),
            rowFont, (int)agBox.width - 28);
        DrawText(row.c_str(), x, y, rowFont, Color{ 196, 211, 238, 255 });
        y += rowH; ++shown;
    }
    if (w.agents.size() > (size_t)shown)
        DrawText(TextFormat("... 他%d件", (int)w.agents.size() - shown), x, y, rowFont,
                 Color{ 130, 140, 160, 255 });
    EndScissorMode();

    // Concept list
    x = (int)coBox.x + 12; y = (int)coBox.y + 14;
    shown = 0;
    BeginScissorMode((int)coBox.x + 2, (int)coBox.y + 14,
                     (int)coBox.width - 4, (int)coBox.height - 18);
    for (const auto& c : w.concepts) {
        if (shown >= maxRows) break;
        const std::string cname = jpDomainName(c.name);
        const std::string row = fitText(TextFormat("%s  (%.1f)", cname.c_str(), c.strength),
                                        rowFont, (int)coBox.width - 28);
        DrawText(row.c_str(), x, y, rowFont, Color{ 210, 200, 238, 255 });
        y += rowH; ++shown;
    }
    if (w.concepts.size() > (size_t)shown)
        DrawText(TextFormat("... 他%d件", (int)w.concepts.size() - shown), x, y, rowFont,
                 Color{ 130, 140, 160, 255 });
    EndScissorMode();
}

void MainPanel::handleCamera(Simulation& sim, Rectangle wv) {
    const World& w = sim.world();
    cam_.offset = { wv.x + wv.width / 2.0f, wv.y + wv.height / 2.0f };
    if (!camInit_) {
        cam_.target = w.center;
        const float fit = std::min(wv.width / w.bounds.x, wv.height / w.bounds.y) * 0.9f;
        cam_.zoom = fit > 0.0f ? fit : 1.0f;
        camInit_ = true;
    }
    if (CheckCollisionPointRec(GetMousePosition(), wv)) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            cam_.zoom = std::clamp(cam_.zoom * (1.0f + wheel * 0.1f), 0.05f, 6.0f);
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 d = GetMouseDelta();
            cam_.target.x -= d.x / cam_.zoom;
            cam_.target.y -= d.y / cam_.zoom;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            const Vector2 wp = GetScreenToWorld2D(GetMousePosition(), cam_);
            float best = 1.0e9f;
            std::string label;
            for (const auto& a : w.agents) {
                if (!a.alive()) continue;
                const float dx = wp.x - a.position.x;
                const float dy = wp.y - a.position.y;
                const float d = dx * dx + dy * dy;
                if (d < best && d < 26.0f * 26.0f) {
                    best = d;
                    label = TextFormat("エージェント: %s  生命 %.0f  状態 %s",
                                       a.name.c_str(), a.life, jpAgentStateName(a.state));
                }
            }
            for (const auto& c : w.concepts) {
                const float dx = wp.x - c.position.x;
                const float dy = wp.y - c.position.y;
                const float d = dx * dx + dy * dy;
                if (d < best && d < 34.0f * 34.0f) {
                    best = d;
                    const std::string cname = jpDomainName(c.name);
                    label = TextFormat("概念: %s  強度 %.1f  起源 %ld",
                                       cname.c_str(), c.strength, c.origin_cycle);
                }
            }
            for (const auto& a : w.activeAnomalies) {
                const float dx = wp.x - a.position.x;
                const float dy = wp.y - a.position.y;
                const float d = dx * dx + dy * dy;
                if (d < best && d < a.radius * a.radius) {
                    best = d;
                    label = TextFormat("異常: %s  強度 %.0f  残り %d",
                                       jpAnomalyTypeName(a.type), a.strength, a.remainingCycles);
                }
            }
            if (!label.empty()) {
                selectedLabel_ = label;
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    contextMenuOpen_ = true;
                    contextMenuPos_ = GetMousePosition();
                }
            } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                contextMenuOpen_ = false;
            }
        }
    }
}

void MainPanel::drawWorldView(Simulation& sim, Rectangle wv) {
    handleCamera(sim, wv);
    renderer_.drawWorld(sim.world(), cam_, wv);
    DrawRectangleLinesEx(wv, 1.0f, Color{ 50, 58, 78, 255 });
    if (!selectedLabel_.empty()) {
        DrawRectangle((int)wv.x + 10, (int)(wv.y + wv.height - 36), 520, 26,
                      Color{ 12, 14, 22, 220 });
        DrawText(selectedLabel_.c_str(), (int)wv.x + 18,
                 (int)(wv.y + wv.height - 30), 15, Color{ 220, 230, 250, 255 });
    }
    if (contextMenuOpen_) {
        Rectangle r{ contextMenuPos_.x, contextMenuPos_.y, 340.0f, 94.0f };
        if (r.x + r.width > wv.x + wv.width) r.x = wv.x + wv.width - r.width - 8.0f;
        if (r.y + r.height > wv.y + wv.height) r.y = wv.y + wv.height - r.height - 8.0f;
        DrawRectangleRec(r, Color{ 14, 16, 24, 240 });
        DrawRectangleLinesEx(r, 1.0f, Color{ 90, 110, 150, 255 });
        DrawText("詳細", (int)r.x + 10, (int)r.y + 8, 16, RAYWHITE);
        DrawText(selectedLabel_.c_str(), (int)r.x + 10, (int)r.y + 34, 13,
                 Color{ 215, 225, 245, 255 });
        if (GuiButton({ r.x + 10.0f, r.y + 60.0f, 80.0f, 24.0f }, "閉じる")) {
            contextMenuOpen_ = false;
        }
    }
}

void MainPanel::drawSpeedBar(AppState& state, Simulation& sim, Rectangle wv) {
    const float vals[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };
    const char* labels[] = { "x0.25", "x0.5", "x1", "x2", "x4", "x8", "最大" };
    const int n = 7;
    float bx = wv.x + 8.0f;
    const float by = wv.y + 8.0f, bw = 52.0f, bh = 26.0f, gap = 4.0f;
    for (int i = 0; i < n; ++i) {
        Rectangle b = { bx + i * (bw + gap), by, bw, bh };
        bool active = (i < 6) ? (!sim.maxMode() && std::fabs(state.settings.sim.speed - vals[i]) < 0.001f)
                              : sim.maxMode();
        if (active) GuiSetState(STATE_PRESSED);
        if (GuiButton(b, labels[i])) {
            if (i < 6) {
                state.settings.sim.speed = vals[i];
                sim.setMaxMode(false);
            } else {
                sim.setMaxMode(true);
            }
        }
        if (active) GuiSetState(STATE_NORMAL);
    }
}

void MainPanel::drawEventLog(Simulation& sim, Rectangle el) {
    const World& w = sim.world();
    DrawRectangleRec(el, Color{ 14, 16, 22, 255 });
    DrawLine((int)el.x, (int)el.y, (int)(el.x + el.width), (int)el.y, Color{ 60, 70, 90, 255 });
    DrawText("イベントログ", (int)el.x + 12, (int)el.y + 6, 17, Color{ 170, 182, 210, 255 });

    const int lineH = 19;
    Rectangle body{ el.x + 8.0f, el.y + 30.0f, el.width - 16.0f, el.height - 36.0f };
    const int rows = std::max(1, (int)body.height / lineH);
    int count = (int)w.events.size();
    int start = updateTailScroll(body, eventLogScroll_, count, rows);
    int y = (int)body.y;
    BeginScissorMode((int)body.x, (int)body.y, (int)body.width, (int)body.height);
    for (int i = start; i < count && i < start + rows; ++i) {
        const Event& e = w.events[i];
        Color c = Color{ 170, 180, 200, 255 };
        if (e.type == EventType::Anomaly) c = Color{ 230, 150, 110, 255 };
        else if (e.type == EventType::Civilization) c = Color{ 150, 210, 160, 255 };
        else if (e.tag == "agent_dead") c = Color{ 200, 130, 130, 255 };
        const std::string msg = fitText(e.message, 15, (int)body.width - 128);
        DrawText(TextFormat("[サイクル %ld] %s", e.cycle, msg.c_str()),
                 (int)body.x + 4, y, 15, c);
        y += lineH;
    }
    EndScissorMode();
}

void MainPanel::drawEmptyFrame(AppState& state, AudioManager& audio) {
    const int W = GetScreenWidth();
    Rectangle hr = headerRect();
    DrawRectangleRec(hr, Color{ 22, 26, 38, 255 });
    DrawText("サイクル: -    段階: -    人口: -    安定度: -    崩壊危険度: -",
             16, 16, 19, RAYWHITE);

    Rectangle cr = controlBarRect();
    DrawRectangleRec(cr, Color{ 18, 20, 30, 255 });
    if (GuiButton({ (float)W - 96.0f, 10.0f, 84.0f, 28.0f }, "< タイトル"))
        state.setScreen(Screen::Title);
    if (GuiButton({ 12.0f, cr.y + 7.0f, 160.0f, 30.0f }, "BGM切替"))
        audio.setMuted(!audio.isMuted());
    if (GuiButton({ 180.0f, cr.y + 7.0f, 120.0f, 30.0f }, "設定"))
        state.setScreen(Screen::Settings);
    if (GuiButton({ 308.0f, cr.y + 7.0f, 150.0f, 30.0f }, "シナリオ選択"))
        state.setScreen(Screen::ScenarioSelect);

    Rectangle wv = worldViewRect();
    DrawRectangleRec(wv, Color{ 8, 9, 14, 255 });
    DrawRectangleLinesEx(wv, 1.0f, Color{ 50, 58, 78, 255 });
    DrawText("シミュレーション未開始。新規またはシナリオ選択から開始してください。",
             (int)wv.x + 24, (int)wv.y + 24, 18, Color{ 120, 130, 150, 255 });
}

} // namespace aic
