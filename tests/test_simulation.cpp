// Headless verification of deterministic stepping, agent/concept population,
// event generation, reset and run-to-completion.
#include "app/AppState.hpp"
#include "simulation/Simulation.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using namespace aic;

static int g_fail = 0;
#define CHECK(cond, msg)                              \
    do {                                              \
        if (!(cond)) { std::printf("FAIL: %s\n", msg); ++g_fail; } \
        else         { std::printf("ok  : %s\n", msg); }           \
    } while (0)

static ScenarioConfig makeCfg() {
    ScenarioConfig cfg;
    cfg.id = "test";
    cfg.name = "Test";
    cfg.seed = 12345;
    cfg.initialAgents = 8;
    cfg.resourceTotal = 1000.0;
    cfg.environmentPressure = 0.2f;
    cfg.anomalyPressure = 0.05f;
    cfg.maxCycle = 200;
    return cfg;
}

int main() {
    ScenarioConfig cfg = makeCfg();
    Settings s;
    s.sim.seed = 12345;
    s.sim.maxCycle = 200;

    Simulation sim;
    sim.start(cfg, s);
    CHECK(sim.active(), "active after start");
    CHECK(sim.world().agents.size() == 8, "8 initial agents spawned");
    CHECK(!sim.world().concepts.empty(), "initial concepts seeded");
    CHECK(sim.world().cycle == 0, "cycle starts at 0");

    for (int i = 0; i < 50; ++i) sim.step();
    CHECK(sim.world().cycle == 50, "cycle advanced by 50 steps");
    CHECK(sim.world().events.size() >= 1, "events were generated");

    // Determinism: same seed must reproduce identical state.
    Simulation sim2;
    sim2.start(cfg, s);
    for (int i = 0; i < 50; ++i) sim2.step();
    CHECK(sim2.world().cycle == sim.world().cycle, "deterministic cycle");
    CHECK(sim2.world().agents.size() == sim.world().agents.size(),
          "deterministic agent count");
    CHECK(sim2.world().resourceTotal == sim.world().resourceTotal,
          "deterministic resources");

    // Reset returns to a fresh run.
    sim.reset();
    CHECK(sim.world().cycle == 0, "reset returns to cycle 0");
    CHECK(sim.world().agents.size() == 8, "reset respawns agents");

    // Run to completion.
    ScenarioConfig cfg3 = cfg;
    Settings s3 = s;
    s3.sim.maxCycle = 120;
    Simulation sim3;
    sim3.start(cfg3, s3);
    int guard = 0;
    while (!sim3.finished() && guard < 100000) { sim3.step(); ++guard; }
    CHECK(sim3.finished(), "simulation reaches a terminal state");
    CHECK(sim3.world().cycle <= 120, "did not overrun max cycle");

    // --- Civilization systems emergence (favorable, sustained run) ---
    {
        ScenarioConfig ec;
        ec.id = "emerge"; ec.name = "Emerge"; ec.seed = 2024;
        ec.initialAgents = 12; ec.resourceTotal = 2000.0;
        ec.environmentPressure = 0.15f; ec.anomalyPressure = 0.0f;
        ec.maxCycle = 300; ec.faithBias = 0.6f; ec.creativityBias = 0.5f;
        Settings es; es.sim.seed = 2024; es.sim.maxCycle = 300;
        Simulation se; se.start(ec, es);
        int g = 0;
        while (!se.finished() && g < 100000) { se.step(); ++g; }
        const World& w = se.world();
        std::printf("[emerge] cycle=%ld pop=%d concepts=%d laws=%d religions=%d "
                    "structures=%d stage=%s evo=%.0f\n",
                    w.cycle, w.civ.population, (int)w.concepts.size(),
                    (int)w.laws.size(), (int)w.religions.size(),
                    (int)w.structures.size(), stageName(w.civ.stage),
                    w.civ.evolution_score);
        CHECK(w.concepts.size() > 5, "concepts grew beyond initial seed");
        CHECK(w.laws.size() >= 1, "at least one law emerged");
        CHECK(w.religions.size() >= 1, "at least one religion emerged");
        CHECK(w.structures.size() >= 1, "at least one structure built");
        CHECK((int)w.civ.stage > (int)Stage::Primitive, "stage advanced beyond Primitive");
    }

    // All configured scenarios load and run to a terminal state.
    {
        std::vector<std::filesystem::path> scenarios;
        for (const auto& entry : std::filesystem::directory_iterator("config/scenarios")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                scenarios.push_back(entry.path());
            }
        }
        CHECK(scenarios.size() >= 4, "scenario json files discovered");
        for (const auto& path : scenarios) {
            ScenarioConfig sc;
            CHECK(loadScenarioConfig(path.string(), sc), ("scenario loads: " + path.string()).c_str());
            Settings ss;
            ss.sim.seed = sc.seed;
            ss.sim.maxCycle = sc.maxCycle;
            Simulation sx;
            sx.start(sc, ss);
            int guard = 0;
            while (!sx.finished() && guard < 200000) { sx.step(); ++guard; }
            CHECK(sx.finished(), ("scenario finishes: " + sc.name).c_str());
            CHECK(!sx.world().events.empty(), ("scenario has events: " + sc.name).c_str());
            bool conceptNamesBounded = true;
            for (const auto& c : sx.world().concepts) {
                if (c.name.size() > 48) {
                    conceptNamesBounded = false;
                    break;
                }
            }
            CHECK(conceptNamesBounded,
                  ("scenario concept names stay bounded: " + sc.name).c_str());
            if (sc.id == "primitive_seed") {
                CHECK(sx.world().cycle == sc.maxCycle,
                      "default primitive scenario reaches the final cycle");
                CHECK(sx.world().civ.population > 0,
                      "default primitive scenario keeps a living population");
                CHECK(sx.world().civ.stage != Stage::Collapsed,
                      "default primitive scenario does not collapse on its own");
                long latestOngoingEvent = 0;
                for (const auto& e : sx.world().events) {
                    if (e.tag != "sim_end") latestOngoingEvent = std::max(latestOngoingEvent, e.cycle);
                }
                CHECK(latestOngoingEvent > sc.maxCycle / 2,
                      "default primitive scenario keeps generating late events");
                CHECK(sx.world().concepts.size() > 240,
                      "default primitive scenario grows beyond the old two-atom concept ceiling");
            }
            if (sc.id == "bh_gv_hh_experiment") {
                CHECK(sx.world().cycle >= 160,
                      "BH-GV-HH experiment survives until WhiteHole injection");
                CHECK(sx.world().experimentResult != "Erased",
                      "BH-GV-HH experiment has a recoverable result");
            }
        }
    }

    std::printf(g_fail == 0 ? "\nALL PASS\n" : "\nFAILURES: %d\n", g_fail);
    return g_fail == 0 ? 0 : 1;
}
