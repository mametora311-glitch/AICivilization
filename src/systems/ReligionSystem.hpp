#pragma once

namespace aic {

class World;
class Random;

// Religions emerge from high collective faith bias, believing agents, or
// exposure to anomalies/UNKNOWN concepts (spec sections 6 / 7 / 8.4).
class ReligionSystem {
public:
    void update(World& world, Random& rng);
};

} // namespace aic
