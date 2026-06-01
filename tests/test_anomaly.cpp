#include "app/AppState.hpp"
#include "simulation/Simulation.hpp"

#include <cstdio>

using namespace aic;

static int fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++fail; } else std::printf("ok  : %s\n", msg); } while (0)

int main() {
    ScenarioConfig cfg;
    cfg.id = "anomaly";
    cfg.name = "Anomaly Test";
    cfg.seed = 333;
    cfg.initialAgents = 10;
    cfg.resourceTotal = 1800.0;
    cfg.maxCycle = 260;

    Settings settings;
    settings.sim.seed = 333;
    settings.sim.maxCycle = 260;

    Simulation sim;
    sim.start(cfg, settings);
    CHECK(sim.injectAnomaly("BlackHole", 75.0f), "manual BlackHole injection");
    CHECK(!sim.world().activeAnomalies.empty(), "active anomaly registered");
    CHECK(sim.world().hasConcept("BlackHole"), "BlackHole concept created");

    sim.startBhGvHhExperiment();
    for (int i = 0; i < 220 && sim.world().experimentResult == "Running"; ++i) sim.step();
    CHECK(sim.world().experimentBhGvHh, "experiment flag set");
    CHECK(sim.world().experimentResult != "Running", "experiment reaches result");
    CHECK(!sim.world().events.empty(), "anomaly events emitted");
    return fail == 0 ? 0 : 1;
}
