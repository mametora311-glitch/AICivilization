#pragma once

namespace aic {

class World;
class Random;

// Structures are built once the population and resource surplus allow it
// (spec sections 7 / 8.4). Structures are placed in the world and rendered.
class StructureSystem {
public:
    void update(World& world, Random& rng);
};

} // namespace aic
