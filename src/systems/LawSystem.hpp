#pragma once

namespace aic {

class World;
class Random;

// Laws emerge once a stable, sufficiently populous society has accumulated
// concepts to formalize (spec sections 7 / 8.4).
class LawSystem {
public:
    void update(World& world, Random& rng);
};

} // namespace aic
