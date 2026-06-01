#pragma once

#include "simulation/Agent.hpp"

namespace aic {

class World;
class Random;

// Drives per-cycle agent behaviour (spec section 7.1 actions): move, consume,
// observe, mutate, share_resource, reproduce, escape, collapse. Concept/law/
// religion/structure creation are delegated to their dedicated systems.
class AgentSystem {
public:
    void update(World& world, Random& rng);

private:
    Agent makeChild(World& world, Random& rng, const Agent& parent);
};

} // namespace aic
