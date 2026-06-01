#include "systems/LawSystem.hpp"

#include "simulation/World.hpp"
#include "util/Localization.hpp"
#include "util/Random.hpp"

#include <algorithm>

namespace aic {

namespace {
constexpr int kMaxLaws = 20;

const std::vector<std::string>& lawNames() {
    static const std::vector<std::string> names = {
        "ResourceSharing", "TerritoryRights", "Cooperation", "Inheritance",
        "ConflictResolution", "Trade", "Migration", "ConceptProtection",
        "FaithFreedom", "Order", "Stewardship", "Assembly"
    };
    return names;
}

bool hasLaw(const World& w, const std::string& n) {
    return std::find(w.laws.begin(), w.laws.end(), n) != w.laws.end();
}
} // namespace

void LawSystem::update(World& world, Random& rng) {
    if ((int)world.laws.size() >= kMaxLaws) return;

    const int pop = world.aliveAgents();
    if (pop < 5 || world.stability < 40.0f) return;

    float avgInt = 0.0f;
    int n = 0;
    for (const auto& a : world.agents) {
        if (!a.alive()) continue;
        avgInt += a.intelligence; ++n;
    }
    if (n == 0) return;
    avgInt /= n;

    const float chance = 0.004f + avgInt * 0.00005f +
                         (float)world.concepts.size() * 0.0003f;
    if (!rng.chance(chance)) return;

    // Pick a law that does not yet exist.
    auto names = lawNames();
    for (int s = (int)names.size() - 1; s > 0; --s)
        std::swap(names[s], names[rng.nextInt(0, s)]);
    std::string chosen;
    for (const auto& nm : names) {
        if (!hasLaw(world, nm)) { chosen = nm; break; }
    }
    if (chosen.empty()) return;

    world.laws.push_back(chosen);

    Event e;
    e.cycle = world.cycle; e.type = EventType::Law; e.name = chosen;
    e.source = "Civilization"; e.target = "World"; e.value = (float)world.laws.size();
    e.message = "法制定: " + jpDomainName(chosen);
    e.tag = "law_created";
    world.pushEvent(e);
}

} // namespace aic
