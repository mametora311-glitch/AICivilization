#include "gui/AnomalyPanel.hpp"

#include "app/AppState.hpp"
#include "gui/GuiText.hpp"
#include "raygui.h"
#include "simulation/Simulation.hpp"

#define DrawText aic::drawText

namespace aic {

void AnomalyPanel::draw(AppState& state, Simulation& simulation, Rectangle bounds) {
    GuiGroupBox(bounds, "異常パネル");
    const int x = (int)bounds.x + 16;
    int y = (int)bounds.y + 24;
    DrawText(TextFormat("発生中の異常: %d",
             simulation.active() ? (int)simulation.world().activeAnomalies.size() : 0),
             x, y, 16, Color{ 210, 220, 240, 255 });
    y += 32;
    if (GuiButton({ (float)x, (float)y, 160.0f, 30.0f }, "ラボを開く")) {
        state.setScreen(Screen::AnomalyLab);
    }
}

} // namespace aic
