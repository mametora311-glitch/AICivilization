#include "gui/TimelinePanel.hpp"

#include "gui/GuiText.hpp"
#include "raygui.h"
#include "simulation/Simulation.hpp"

#define DrawText aic::drawText

namespace aic {

void TimelinePanel::draw(Simulation& simulation, Rectangle bounds) {
    GuiGroupBox(bounds, "タイムライン");
    if (!simulation.active()) return;
    int x = (int)bounds.x + 16;
    int y = (int)bounds.y + 24;
    const auto& events = simulation.world().events;
    const int start = events.size() > 8 ? (int)events.size() - 8 : 0;
    for (int i = start; i < (int)events.size(); ++i) {
        DrawText(TextFormat("サイクル %ld: %s", events[i].cycle, events[i].message.c_str()),
                 x, y, 14, Color{ 210, 220, 240, 255 });
        y += 20;
    }
}

} // namespace aic
