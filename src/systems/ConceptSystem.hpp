#pragma once

namespace aic {

class World;
class Random;

// Agents generate concepts from observation and creativity; existing concepts
// combine into compound concepts (spec section 7.2).
class ConceptSystem {
public:
    void update(World& world, Random& rng);
};

} // namespace aic
