#pragma once

#include "raylib.h"

namespace aic {

class Simulation;

class TimelinePanel {
public:
    void draw(Simulation& simulation, Rectangle bounds);
};

} // namespace aic
