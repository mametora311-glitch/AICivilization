#include "app/AppState.hpp"
#include "simulation/Simulation.hpp"

#include <cstdio>
#include <filesystem>

using namespace aic;

static int fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++fail; } else std::printf("ok  : %s\n", msg); } while (0)

int main() {
    ScenarioConfig cfg;
    cfg.id = "save";
    cfg.name = "Save Load Test";
    cfg.seed = 444;
    cfg.initialAgents = 7;
    cfg.resourceTotal = 1200.0;
    cfg.maxCycle = 120;

    Settings settings;
    settings.sim.seed = 444;
    settings.sim.maxCycle = 120;

    Simulation sim;
    sim.start(cfg, settings);
    for (int i = 0; i < 25; ++i) sim.step();
    sim.injectAnomaly("NoiseRain", 50.0f);

    const std::string path = "data/saves/test_save_load.json";
    CHECK(sim.saveToFile(path), "save file written");

    Settings loadedSettings;
    Simulation loaded;
    CHECK(loaded.loadFromFile(path, loadedSettings), "save file loaded");
    CHECK(loaded.world().cycle == sim.world().cycle, "cycle restored");
    CHECK(loaded.world().agents.size() == sim.world().agents.size(), "agents restored");
    CHECK(loaded.world().concepts.size() == sim.world().concepts.size(), "concepts restored");
    CHECK(loaded.world().activeAnomalies.size() == sim.world().activeAnomalies.size(), "anomalies restored");

    std::error_code ec;
    std::filesystem::remove(path, ec);
    return fail == 0 ? 0 : 1;
}
