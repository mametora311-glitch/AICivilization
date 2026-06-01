#pragma once

#include "raylib.h"

namespace aic {

class AppState;
class Simulation;

class AnomalyPanel {
public:
    void draw(AppState& state, Simulation& simulation, Rectangle bounds);
};

} // namespace aic
