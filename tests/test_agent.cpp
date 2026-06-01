#include "app/AppState.hpp"
#include "simulation/Simulation.hpp"

#include <cstdio>

using namespace aic;

static int fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++fail; } else std::printf("ok  : %s\n", msg); } while (0)

int main() {
    ScenarioConfig cfg;
    cfg.id = "agent";
    cfg.name = "Agent Test";
    cfg.seed = 111;
    cfg.initialAgents = 6;
    cfg.resourceTotal = 900.0;
    cfg.maxCycle = 100;

    Settings settings;
    settings.sim.seed = 111;
    settings.sim.maxCycle = 100;

    Simulation sim;
    sim.start(cfg, settings);
    const auto firstPos = sim.world().agents.front().position;
    sim.step();
    const auto movedPos = sim.world().agents.front().position;

    CHECK(sim.world().agents.front().id > 0, "agent has id");
    CHECK(!sim.world().agents.front().name.empty(), "agent has name");
    CHECK(firstPos.x != movedPos.x || firstPos.y != movedPos.y, "agent moves");
    CHECK(sim.world().civ.population > 0, "population remains positive");
    return fail == 0 ? 0 : 1;
}
