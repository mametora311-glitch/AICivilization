#pragma once

#include "app/AppState.hpp"
#include "anomaly/AnomalySystem.hpp"
#include "simulation/World.hpp"
#include "systems/AgentSystem.hpp"
#include "systems/ConceptSystem.hpp"
#include "systems/LawSystem.hpp"
#include "systems/ReligionSystem.hpp"
#include "systems/StructureSystem.hpp"
#include "util/JsonUtil.hpp"
#include "util/Random.hpp"

#include <string>
#include <vector>

namespace aic {

// Fully resolved scenario parameters (parsed from config/scenarios/*.json).
struct ScenarioConfig {
    std::string id;
    std::string name;
    unsigned int seed = 12345;
    int   initialAgents = 6;
    double resourceTotal = 1000.0;
    float environmentPressure = 0.2f;
    float anomalyPressure = 0.05f;
    int   maxCycle = 18000;
    int   autoSnapshotInterval = 3000;
    bool  experimentBhGvHh = false;
    bool  randomize = false;
    float faithBias = 0.2f;
    float creativityBias = 0.3f;
    std::vector<ScheduledAnomaly> schedule;
};

bool loadScenarioConfig(const std::string& path, ScenarioConfig& out);

// Owns the World and advances it. Implements the run controls (Start/Pause/
// Step/Reset) and the speed/Max handling (spec sections 3.1-3.3).
class Simulation {
public:
    void start(const ScenarioConfig& cfg, const Settings& settings);
    void reset();
    void update(float dt, float speed);
    void step();
    bool injectAnomaly(const std::string& type, float strength);
    bool injectAnomaly(AnomalyType type, float strength);
    void startBhGvHhExperiment();
    Json toJson() const;
    bool fromJson(const Json& root, Settings& settings);
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path, Settings& settings);

    void pause()  { running_ = false; }
    void resume() { if (active_ && !finished_) running_ = true; }
    void togglePause();
    void setMaxMode(bool on) { maxMode_ = on; }
    bool maxMode() const { return maxMode_; }

    bool active() const { return active_; }
    bool running() const { return running_; }
    bool finished() const { return finished_; }
    int  maxCycle() const { return maxCycle_; }
    const ScenarioConfig& config() const { return config_; }

    World& world() { return world_; }
    const World& world() const { return world_; }
    Random& rng() { return rng_; }

private:
    void stepCycle();
    Agent spawnAgentAroundCenter();
    void seedInitialConcepts();
    void recomputeCivilization();
    void updateStage();
    void emitThresholdWarnings();

    World world_;
    Random rng_;
    ScenarioConfig config_;
    Settings settings_{};

    AgentSystem agentSystem_;
    ConceptSystem conceptSystem_;
    LawSystem lawSystem_;
    ReligionSystem religionSystem_;
    StructureSystem structureSystem_;
    AnomalySystem anomalySystem_;

    bool active_ = false;
    bool running_ = false;
    bool finished_ = false;
    bool maxMode_ = false;
    int  maxCycle_ = 18000;

    float accumulator_ = 0.0f;
    float secondsPerCycle_ = 0.2f;

    int previousPopulation_ = 0;
    bool collapseRiskWarningActive_ = false;
    bool resourceWarningActive_ = false;
};

} // namespace aic
