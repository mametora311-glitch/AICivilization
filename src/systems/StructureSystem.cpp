#include "systems/StructureSystem.hpp"

#include "simulation/World.hpp"
#include "util/Localization.hpp"
#include "util/Random.hpp"

#include <algorithm>
#include <cmath>

namespace aic {

namespace {
constexpr int kMaxStructures = 30;

const std::vector<std::string>& structureNames() {
    static const std::vector<std::string> names = {
        "Shelter", "Granary", "Temple", "Archive", "Tower",
        "Workshop", "Monument", "Aqueduct", "Beacon", "Hall"
    };
    return names;
}
} // namespace

void StructureSystem::update(World& world, Random& rng) {
    if ((int)world.structures.size() >= kMaxStructures) return;

    const int pop = world.aliveAgents();
    if (pop < 6 || world.resourceTotal < 120.0) return;

    const float chance = 0.003f + pop * 0.0003f;
    if (!rng.chance(chance)) return;

    Structure s;
    s.name = structureNames()[rng.nextInt(0, (int)structureNames().size() - 1)];
    const float ang = rng.nextFloat(0.0f, 2.0f * PI);
    const float rad = rng.nextFloat(60.0f, 320.0f);
    s.position = { world.center.x + std::cos(ang) * rad,
                   world.center.y + std::sin(ang) * rad };
    s.size = 10.0f + pop * 0.3f;
    s.origin_cycle = world.cycle;
    world.structures.push_back(s);

    world.resourceTotal -= std::min(50.0, world.resourceTotal);   // building cost

    Event e;
    e.cycle = world.cycle; e.type = EventType::Structure; e.name = s.name;
    e.source = "Civilization"; e.target = "World"; e.value = (float)world.structures.size();
    e.message = "構造物建設: " + jpDomainName(s.name);
    e.tag = "structure_created";
    world.pushEvent(e);
}

} // namespace aic
