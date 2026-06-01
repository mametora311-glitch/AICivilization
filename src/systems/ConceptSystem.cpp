#include "systems/ConceptSystem.hpp"

#include "simulation/World.hpp"
#include "util/Localization.hpp"
#include "util/Random.hpp"

#include <algorithm>
#include <cmath>

namespace aic {

namespace {
constexpr int kMaxConcepts = 6000;
constexpr int kMaxCreatePerCycle = 3;
constexpr size_t kMaxConceptNameBytes = 48;

Concept* findConcept(World& world, const std::string& name) {
    for (auto& c : world.concepts) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

std::vector<std::string> conceptAtoms(const std::string& name) {
    std::vector<std::string> atoms;
    for (const auto& atom : primitiveConceptNames()) {
        if (name.find(atom) != std::string::npos) {
            atoms.push_back(atom);
        }
    }
    if (atoms.empty()) atoms.push_back(name);
    return atoms;
}

void appendUniqueAtom(std::vector<std::string>& atoms, const std::string& atom) {
    if (std::find(atoms.begin(), atoms.end(), atom) == atoms.end()) {
        atoms.push_back(atom);
    }
}

std::string compoundConceptName(const Concept& a, const Concept& b, Random& rng) {
    auto aa = conceptAtoms(a.name);
    auto bb = conceptAtoms(b.name);

    std::vector<std::string> pool;
    for (const auto& atom : aa) appendUniqueAtom(pool, atom);
    for (const auto& atom : bb) appendUniqueAtom(pool, atom);

    std::vector<std::string> picked;
    appendUniqueAtom(picked, aa[(size_t)rng.nextInt(0, (int)aa.size() - 1)]);
    appendUniqueAtom(picked, bb[(size_t)rng.nextInt(0, (int)bb.size() - 1)]);

    int targetAtoms = 2;
    if (pool.size() >= 4 && rng.chance(0.35f)) {
        targetAtoms = 4;
    } else if (pool.size() >= 3 && rng.chance(0.65f)) {
        targetAtoms = 3;
    }
    int guard = 0;
    while ((int)picked.size() < targetAtoms && guard++ < 8) {
        appendUniqueAtom(picked, pool[(size_t)rng.nextInt(0, (int)pool.size() - 1)]);
    }

    if (picked.size() < 2) {
        for (const auto& atom : primitiveConceptNames()) {
            appendUniqueAtom(picked, atom);
            if (picked.size() >= 2) break;
        }
    }

    for (int s = (int)picked.size() - 1; s > 0; --s) {
        std::swap(picked[s], picked[(size_t)rng.nextInt(0, s)]);
    }

    std::string out;
    for (const auto& atom : picked) {
        if (out.size() + atom.size() > kMaxConceptNameBytes) break;
        out += atom;
    }
    if (out.empty()) out = a.name + b.name;
    if (out.size() > kMaxConceptNameBytes) {
        out.resize(kMaxConceptNameBytes);
    }
    return out;
}

bool tryCreateCompound(World& world, Random& rng, std::string& name,
                       std::vector<int>& parents, float& strength) {
    Concept* duplicate = nullptr;
    for (int attempt = 0; attempt < 10; ++attempt) {
        int i = rng.nextInt(0, (int)world.concepts.size() - 1);
        int j = rng.nextInt(0, (int)world.concepts.size() - 1);
        if (i == j) continue;
        const Concept& p1 = world.concepts[(size_t)i];
        const Concept& p2 = world.concepts[(size_t)j];
        std::string candidate = compoundConceptName(p1, p2, rng);
        if (Concept* ex = findConcept(world, candidate)) {
            duplicate = ex;
            continue;
        }
        name = candidate;
        parents = { p1.id, p2.id };
        strength = (p1.strength + p2.strength) * 0.6f + 1.0f;
        return true;
    }
    if (duplicate) {
        duplicate->strength += 0.5f;
    }
    return false;
}

bool tryCreatePrimitive(World& world, Random& rng, std::string& name) {
    auto names = primitiveConceptNames();
    for (int s = (int)names.size() - 1; s > 0; --s) {
        std::swap(names[(size_t)s], names[(size_t)rng.nextInt(0, s)]);
    }
    for (const auto& n : names) {
        if (!world.hasConcept(n)) {
            name = n;
            return true;
        }
    }
    return false;
}

bool tryCreateConcept(World& world, Random& rng, std::string& name,
                      std::vector<int>& parents, float& strength) {
    const bool canCombine = world.concepts.size() >= 2;
    if (canCombine && rng.chance(0.65f)) {
        if (tryCreateCompound(world, rng, name, parents, strength)) return true;
        return tryCreatePrimitive(world, rng, name);
    }
    if (tryCreatePrimitive(world, rng, name)) return true;
    if (canCombine) {
        return tryCreateCompound(world, rng, name, parents, strength);
    }
    return false;
}
} // namespace

void ConceptSystem::update(World& world, Random& rng) {
    if (world.agents.empty() || (int)world.concepts.size() >= kMaxConcepts) return;

    int created = 0;
    for (auto& a : world.agents) {
        if (created >= kMaxCreatePerCycle) break;
        if (!a.alive()) continue;

        const float saturation =
            std::clamp(1.0f - ((float)world.concepts.size() / (float)kMaxConcepts) * 0.85f,
                       0.15f, 1.0f);
        const float chance = (0.002f + a.creativity * 0.00008f +
                              a.intelligence * 0.00004f +
                              world.anomalyPressure * 0.02f) * saturation;
        if (!rng.chance(chance)) continue;

        std::string name;
        std::vector<int> parents;
        float strength = rng.nextFloat(1.0f, 3.0f);

        if (!tryCreateConcept(world, rng, name, parents, strength)) continue;

        Concept c;
        c.id = world.newConceptId();
        c.name = name;
        c.strength = strength;
        c.parents = parents;
        c.origin_cycle = world.cycle;
        c.position = { a.position.x + rng.nextFloat(-30.0f, 30.0f),
                       a.position.y + rng.nextFloat(-30.0f, 30.0f) };
        c.linked_agents = { a.id };
        world.concepts.push_back(std::move(c));

        Event e;
        e.cycle = world.cycle; e.type = EventType::Concept; e.name = name;
        e.source = a.name; e.target = "World"; e.value = strength;
        e.message = "概念生成: " + jpDomainName(name) + (parents.empty() ? "" : "（複合）");
        e.tag = "concept_created";
        world.pushEvent(e);
        ++created;
    }
}

} // namespace aic
