#include "app/AppState.hpp"
#include "simulation/Concept.hpp"
#include "simulation/Simulation.hpp"

#include <cstdio>

using namespace aic;

static int fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++fail; } else std::printf("ok  : %s\n", msg); } while (0)

int main() {
    CHECK(primitiveConceptNames().size() == 15, "primitive concept list size");
    CHECK(primitiveConceptNames().front() == "AI", "primitive list starts with AI");

    ScenarioConfig cfg;
    cfg.id = "concept";
    cfg.name = "Concept Test";
    cfg.seed = 222;
    cfg.initialAgents = 12;
    cfg.resourceTotal = 2000.0;
    cfg.environmentPressure = 0.05f;
    cfg.creativityBias = 1.0f;
    cfg.maxCycle = 220;

    Settings settings;
    settings.sim.seed = 222;
    settings.sim.maxCycle = 220;

    Simulation sim;
    sim.start(cfg, settings);
    const size_t initial = sim.world().concepts.size();
    for (int i = 0; i < 180; ++i) sim.step();
    CHECK(sim.world().concepts.size() > initial, "concepts grow during simulation");
    CHECK(sim.world().civ.concept_count == (int)sim.world().concepts.size(), "concept count synchronized");
    return fail == 0 ? 0 : 1;
}
