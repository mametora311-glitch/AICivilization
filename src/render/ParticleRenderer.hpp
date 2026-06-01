#pragma once

#include "raylib.h"

namespace aic {

class ParticleRenderer {
public:
    void drawRadiation(Vector2 center, float radius, Color color, float phase, int rays);
};

} // namespace aic
