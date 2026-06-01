#pragma once

#include "raylib.h"

namespace aic {

class World;

// Draws the world view (civilization core, agents, concepts) inside a viewport
// rectangle using a 2D camera for pan/zoom. Anomaly overlays are drawn as part
// of the same world pass so the observation view stays coherent.
class Renderer {
public:
    void drawWorld(const World& world, const Camera2D& cam, Rectangle viewport);
};

} // namespace aic
