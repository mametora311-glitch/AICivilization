#include "systems/ReligionSystem.hpp"

#include "simulation/World.hpp"
#include "util/Localization.hpp"
#include "util/Random.hpp"

#include <algorithm>

namespace aic {

namespace {
constexpr int kMaxReligions = 10;

const std::vector<std::string>& religionBases() {
    static const std::vector<std::string> bases = {
        "WhiteHole", "Ocean", "Mind", "Dream", "Gravity", "Memory",
        "BlackHole", "Unknown", "Rain", "Emotion"
    };
    return bases;
}

bool hasReligion(const World& w, const std::string& n) {
    return std::find(w.religions.begin(), w.religions.end(), n) != w.religions.end();
}
} // namespace

void ReligionSystem::update(World& world, Random& rng) {
    if ((int)world.religions.size() >= kMaxReligions) return;

    const int pop = world.aliveAgents();
    if (pop < 4) return;

    float avgFaith = 0.0f;
    int n = 0, believers = 0;
    for (const auto& a : world.agents) {
        if (!a.alive()) continue;
        avgFaith += a.faith_bias; ++n;
        if (a.state == AgentState::Believing) ++believers;
    }
    if (n == 0) return;
    avgFaith /= n;

    int knownBases = 0;
    for (const auto& b : religionBases()) {
        if (world.hasConcept(b)) ++knownBases;
    }

    const float chance = 0.003f + avgFaith * 0.00008f +
                         knownBases * 0.0005f +
                         (float)world.concepts.size() * 0.000015f +
                         world.anomalyPressure * 0.03f +
                         believers * 0.002f;
    if (!rng.chance(chance)) return;

    // Prefer a base concept the civilization actually knows.
    auto bases = religionBases();
    for (int s = (int)bases.size() - 1; s > 0; --s)
        std::swap(bases[s], bases[rng.nextInt(0, s)]);
    std::string base = bases.front();
    for (const auto& b : bases) {
        if (world.hasConcept(b)) { base = b; break; }
    }
    static const char* suffixes[] = { "Cult", "Faith", "Order", "Reverence" };
    std::string name = base + suffixes[rng.nextInt(0, 3)];
    if (hasReligion(world, name)) return;

    world.religions.push_back(name);

    // A few faithful agents convert.
    int converted = 0;
    for (auto& a : world.agents) {
        if (converted >= 3) break;
        if (a.alive() && a.faith_bias > 55.0f) { a.state = AgentState::Believing; ++converted; }
    }

    Event e;
    e.cycle = world.cycle; e.type = EventType::Religion; e.name = name;
    e.source = "Civilization"; e.target = "World"; e.value = (float)world.religions.size();
    e.message = "宗教成立: " + jpDomainName(name);
    e.tag = "religion_created";
    world.pushEvent(e);
}

} // namespace aic
