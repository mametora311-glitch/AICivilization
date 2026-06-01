#include "simulation/Simulation.hpp"

#include "util/JsonUtil.hpp"
#include "util/Localization.hpp"
#include "util/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>

namespace aic {

namespace {
float clampf(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

Json vecToJson(Vector2 v) {
    return Json{ { "x", v.x }, { "y", v.y } };
}

Vector2 vecFromJson(const Json& j, Vector2 fallback = { 0.0f, 0.0f }) {
    if (!j.is_object()) return fallback;
    return { jsonutil::getFloat(j, "x", fallback.x),
             jsonutil::getFloat(j, "y", fallback.y) };
}

Stage parseStage(const std::string& s) {
    if (s == "Tribal") return Stage::Tribal;
    if (s == "Agricultural") return Stage::Agricultural;
    if (s == "Industrial") return Stage::Industrial;
    if (s == "Digital") return Stage::Digital;
    if (s == "PostSingularity") return Stage::PostSingularity;
    if (s == "Collapsed") return Stage::Collapsed;
    if (s == "Unknown") return Stage::Unknown;
    return Stage::Primitive;
}

AgentState parseAgentState(const std::string& s) {
    if (s == "Hungry") return AgentState::Hungry;
    if (s == "Mutating") return AgentState::Mutating;
    if (s == "Reproducing") return AgentState::Reproducing;
    if (s == "Believing") return AgentState::Believing;
    if (s == "Collapsed") return AgentState::Collapsed;
    if (s == "Dead") return AgentState::Dead;
    if (s == "Unknown") return AgentState::Unknown;
    return AgentState::Alive;
}

EventType parseEventType(const std::string& s) {
    if (s == "AGENT") return EventType::Agent;
    if (s == "CONCEPT") return EventType::Concept;
    if (s == "LAW") return EventType::Law;
    if (s == "RELIGION") return EventType::Religion;
    if (s == "STRUCTURE") return EventType::Structure;
    if (s == "ANOMALY") return EventType::Anomaly;
    if (s == "CIVILIZATION") return EventType::Civilization;
    return EventType::System;
}

Json settingsToJson(const Settings& s) {
    return Json{
        { "simulation", {
            { "seed", s.sim.seed },
            { "max_cycle", s.sim.maxCycle },
            { "speed", s.sim.speed },
            { "auto_snapshot_interval", s.sim.autoSnapshotInterval }
        } },
        { "audio", {
            { "bgm_volume", s.audio.bgmVolume },
            { "se_volume", s.audio.seVolume },
            { "mute", s.audio.mute },
            { "duck_during_tts", s.audio.duckDuringTts }
        } },
        { "notification", {
            { "in_app", s.notify.inApp },
            { "windows", s.notify.windows },
            { "tts", s.notify.tts },
            { "voicevox_endpoint", s.notify.voicevoxEndpoint },
            { "speaker_id", s.notify.speakerId },
            { "tts_volume", s.notify.ttsVolume },
            { "min_level", s.notify.minLevel },
            { "cooldown_seconds", s.notify.cooldownSeconds }
        } },
        { "ui", {
            { "language", 0 }
        } }
    };
}

void settingsFromJson(const Json& root, Settings& s) {
    if (root.contains("simulation") && root["simulation"].is_object()) {
        const Json& a = root["simulation"];
        s.sim.seed = jsonutil::getUInt(a, "seed", s.sim.seed);
        s.sim.maxCycle = jsonutil::getInt(a, "max_cycle", s.sim.maxCycle);
        s.sim.speed = jsonutil::getFloat(a, "speed", s.sim.speed);
        s.sim.autoSnapshotInterval =
            jsonutil::getInt(a, "auto_snapshot_interval", s.sim.autoSnapshotInterval);
    }
    if (root.contains("audio") && root["audio"].is_object()) {
        const Json& a = root["audio"];
        s.audio.bgmVolume = jsonutil::getFloat(a, "bgm_volume", s.audio.bgmVolume);
        s.audio.seVolume = jsonutil::getFloat(a, "se_volume", s.audio.seVolume);
        s.audio.mute = jsonutil::getBool(a, "mute", s.audio.mute);
        s.audio.duckDuringTts = jsonutil::getBool(a, "duck_during_tts", s.audio.duckDuringTts);
    }
    if (root.contains("notification") && root["notification"].is_object()) {
        const Json& a = root["notification"];
        s.notify.inApp = jsonutil::getBool(a, "in_app", s.notify.inApp);
        s.notify.windows = jsonutil::getBool(a, "windows", s.notify.windows);
        s.notify.tts = jsonutil::getBool(a, "tts", s.notify.tts);
        s.notify.voicevoxEndpoint =
            jsonutil::getString(a, "voicevox_endpoint", s.notify.voicevoxEndpoint);
        s.notify.speakerId = jsonutil::getInt(a, "speaker_id", s.notify.speakerId);
        s.notify.ttsVolume = jsonutil::getFloat(a, "tts_volume", s.notify.ttsVolume);
        s.notify.minLevel = jsonutil::getInt(a, "min_level", s.notify.minLevel);
        s.notify.cooldownSeconds =
            jsonutil::getInt(a, "cooldown_seconds", s.notify.cooldownSeconds);
    }
    if (root.contains("ui") && root["ui"].is_object()) {
        const Json& a = root["ui"];
        s.ui.language = jsonutil::getInt(a, "language", s.ui.language);
    }
    s.ui.language = 0;
}
} // namespace

bool loadScenarioConfig(const std::string& path, ScenarioConfig& out) {
    auto j = jsonutil::readFile(path);
    if (!j) return false;
    const Json& r = *j;
    out.id = jsonutil::getString(r, "id", out.id);
    out.name = jsonutil::getString(r, "name", out.id);
    out.seed = jsonutil::getUInt(r, "seed", out.seed);
    out.initialAgents = jsonutil::getInt(r, "initial_agents", out.initialAgents);
    out.resourceTotal = jsonutil::getFloat(r, "resource_total", (float)out.resourceTotal);
    out.environmentPressure = jsonutil::getFloat(r, "environment_pressure", out.environmentPressure);
    out.anomalyPressure = jsonutil::getFloat(r, "anomaly_pressure", out.anomalyPressure);
    out.maxCycle = jsonutil::getInt(r, "max_cycle", out.maxCycle);
    out.autoSnapshotInterval = jsonutil::getInt(r, "auto_snapshot_interval", out.autoSnapshotInterval);
    out.experimentBhGvHh = jsonutil::getBool(r, "experiment_bh_gv_hh", out.experimentBhGvHh);
    out.randomize = jsonutil::getBool(r, "randomize", out.randomize);
    out.faithBias = jsonutil::getFloat(r, "faith_bias", out.faithBias);
    out.creativityBias = jsonutil::getFloat(r, "creativity_bias", out.creativityBias);
    out.schedule.clear();
    if (r.contains("anomaly_schedule") && r["anomaly_schedule"].is_array()) {
        for (const auto& e : r["anomaly_schedule"]) {
            ScheduledAnomaly sa;
            sa.cycle = jsonutil::getInt(e, "cycle", 0);
            sa.type = jsonutil::getString(e, "type", "");
            sa.strength = jsonutil::getFloat(e, "strength", 50.0f);
            if (!sa.type.empty()) out.schedule.push_back(sa);
        }
    }
    return true;
}

Json Simulation::toJson() const {
    Json root;
    root["schema_version"] = 1;
    root["active"] = active_;
    root["running"] = running_;
    root["finished"] = finished_;
    root["max_cycle"] = maxCycle_;
    root["settings"] = settingsToJson(settings_);
    root["scenario"] = {
        { "id", config_.id },
        { "name", config_.name },
        { "seed", config_.seed },
        { "initial_agents", config_.initialAgents },
        { "resource_total", config_.resourceTotal },
        { "environment_pressure", config_.environmentPressure },
        { "anomaly_pressure", config_.anomalyPressure },
        { "max_cycle", config_.maxCycle },
        { "auto_snapshot_interval", config_.autoSnapshotInterval },
        { "experiment_bh_gv_hh", config_.experimentBhGvHh },
        { "randomize", config_.randomize },
        { "faith_bias", config_.faithBias },
        { "creativity_bias", config_.creativityBias }
    };

    const World& w = world_;
    Json jw;
    jw["cycle"] = w.cycle;
    jw["world_seed"] = w.worldSeed;
    jw["resource_total"] = w.resourceTotal;
    jw["environment_pressure"] = w.environmentPressure;
    jw["concept_density"] = w.conceptDensity;
    jw["anomaly_pressure"] = w.anomalyPressure;
    jw["stability"] = w.stability;
    jw["collapse_risk"] = w.collapseRisk;
    jw["bounds"] = vecToJson(w.bounds);
    jw["center"] = vecToJson(w.center);
    jw["experiment_bh_gv_hh"] = w.experimentBhGvHh;
    jw["experiment_result"] = w.experimentResult;
    jw["civilization"] = {
        { "population", w.civ.population },
        { "stage", stageName(w.civ.stage) },
        { "resources", w.civ.resources },
        { "concept_count", w.civ.concept_count },
        { "law_count", w.civ.law_count },
        { "religion_count", w.civ.religion_count },
        { "structure_count", w.civ.structure_count },
        { "stability", w.civ.stability },
        { "collapse_risk", w.civ.collapse_risk },
        { "evolution_score", w.civ.evolution_score },
        { "unknown_score", w.civ.unknown_score }
    };
    jw["laws"] = w.laws;
    jw["religions"] = w.religions;

    jw["agents"] = Json::array();
    for (const auto& a : w.agents) {
        jw["agents"].push_back({
            { "id", a.id }, { "name", a.name }, { "life", a.life },
            { "intelligence", a.intelligence }, { "emotion", a.emotion },
            { "adaptability", a.adaptability }, { "creativity", a.creativity },
            { "sociality", a.sociality }, { "faith_bias", a.faith_bias },
            { "memory", a.memory }, { "resource", a.resource },
            { "position", vecToJson(a.position) }, { "velocity", vecToJson(a.velocity) },
            { "state", agentStateName(a.state) }
        });
    }

    jw["concepts"] = Json::array();
    for (const auto& c : w.concepts) {
        jw["concepts"].push_back({
            { "id", c.id }, { "name", c.name }, { "strength", c.strength },
            { "parents", c.parents }, { "origin_cycle", c.origin_cycle },
            { "position", vecToJson(c.position) }, { "linked_agents", c.linked_agents }
        });
    }

    jw["structures"] = Json::array();
    for (const auto& s : w.structures) {
        jw["structures"].push_back({
            { "name", s.name }, { "position", vecToJson(s.position) },
            { "size", s.size }, { "origin_cycle", s.origin_cycle }
        });
    }

    jw["anomalies"] = Json::array();
    for (const auto& a : w.activeAnomalies) {
        jw["anomalies"].push_back({
            { "id", a.id }, { "type", anomalyTypeName(a.type) },
            { "strength", a.strength }, { "remaining_cycles", a.remainingCycles },
            { "position", vecToJson(a.position) }, { "radius", a.radius },
            { "phase", a.phase }, { "pulse_fired", a.pulseFired }
        });
    }

    jw["schedule"] = Json::array();
    for (const auto& s : w.schedule) {
        jw["schedule"].push_back({
            { "cycle", s.cycle }, { "type", s.type },
            { "strength", s.strength }, { "fired", s.fired }
        });
    }

    jw["events"] = Json::array();
    for (const auto& e : w.events) {
        jw["events"].push_back({
            { "cycle", e.cycle }, { "type", eventTypeName(e.type) },
            { "name", e.name }, { "source", e.source }, { "target", e.target },
            { "value", e.value }, { "message", e.message }, { "tag", e.tag }
        });
    }

    jw["civilization_log"] = Json::array();
    for (const auto& s : w.civilizationLog) {
        jw["civilization_log"].push_back({
            { "cycle", s.cycle }, { "population", s.population },
            { "stage", stageName(s.stage) }, { "resources", s.resources },
            { "concepts", s.concepts }, { "laws", s.laws },
            { "religions", s.religions }, { "structures", s.structures },
            { "stability", s.stability }, { "collapse_risk", s.collapse_risk },
            { "unknown_score", s.unknown_score }
        });
    }

    root["world"] = jw;
    return root;
}

bool Simulation::fromJson(const Json& root, Settings& settings) {
    if (!root.contains("world") || !root["world"].is_object()) return false;
    if (root.contains("settings") && root["settings"].is_object()) {
        settingsFromJson(root["settings"], settings);
    }
    settings_ = settings;

    if (root.contains("scenario") && root["scenario"].is_object()) {
        const Json& sc = root["scenario"];
        config_.id = jsonutil::getString(sc, "id", "loaded");
        config_.name = jsonutil::getString(sc, "name", config_.id);
        config_.seed = jsonutil::getUInt(sc, "seed", settings.sim.seed);
        config_.initialAgents = jsonutil::getInt(sc, "initial_agents", 0);
        config_.resourceTotal = jsonutil::getFloat(sc, "resource_total", 1000.0f);
        config_.environmentPressure = jsonutil::getFloat(sc, "environment_pressure", 0.2f);
        config_.anomalyPressure = jsonutil::getFloat(sc, "anomaly_pressure", 0.05f);
        config_.maxCycle = jsonutil::getInt(sc, "max_cycle", settings.sim.maxCycle);
        config_.autoSnapshotInterval = jsonutil::getInt(sc, "auto_snapshot_interval", settings.sim.autoSnapshotInterval);
        config_.experimentBhGvHh = jsonutil::getBool(sc, "experiment_bh_gv_hh", false);
        config_.randomize = jsonutil::getBool(sc, "randomize", false);
        config_.faithBias = jsonutil::getFloat(sc, "faith_bias", 0.2f);
        config_.creativityBias = jsonutil::getFloat(sc, "creativity_bias", 0.3f);
    }

    world_.clear();
    const Json& wj = root["world"];
    world_.cycle = jsonutil::getInt(wj, "cycle", 0);
    world_.worldSeed = jsonutil::getUInt(wj, "world_seed", settings.sim.seed);
    world_.resourceTotal = jsonutil::getFloat(wj, "resource_total", 1000.0f);
    world_.environmentPressure = jsonutil::getFloat(wj, "environment_pressure", 0.2f);
    world_.conceptDensity = jsonutil::getFloat(wj, "concept_density", 0.0f);
    world_.anomalyPressure = jsonutil::getFloat(wj, "anomaly_pressure", 0.0f);
    world_.stability = jsonutil::getFloat(wj, "stability", 90.0f);
    world_.collapseRisk = jsonutil::getFloat(wj, "collapse_risk", 5.0f);
    if (wj.contains("bounds")) world_.bounds = vecFromJson(wj["bounds"], world_.bounds);
    if (wj.contains("center")) world_.center = vecFromJson(wj["center"], world_.center);
    world_.experimentBhGvHh = jsonutil::getBool(wj, "experiment_bh_gv_hh", false);
    world_.experimentResult = jsonutil::getString(wj, "experiment_result", "Stable");

    if (wj.contains("civilization") && wj["civilization"].is_object()) {
        const Json& c = wj["civilization"];
        world_.civ.population = jsonutil::getInt(c, "population", 0);
        world_.civ.stage = parseStage(jsonutil::getString(c, "stage", "Primitive"));
        world_.civ.resources = jsonutil::getFloat(c, "resources", 0.0f);
        world_.civ.concept_count = jsonutil::getInt(c, "concept_count", 0);
        world_.civ.law_count = jsonutil::getInt(c, "law_count", 0);
        world_.civ.religion_count = jsonutil::getInt(c, "religion_count", 0);
        world_.civ.structure_count = jsonutil::getInt(c, "structure_count", 0);
        world_.civ.stability = jsonutil::getFloat(c, "stability", 90.0f);
        world_.civ.collapse_risk = jsonutil::getFloat(c, "collapse_risk", 5.0f);
        world_.civ.evolution_score = jsonutil::getFloat(c, "evolution_score", 0.0f);
        world_.civ.unknown_score = jsonutil::getFloat(c, "unknown_score", 0.0f);
    }

    if (wj.contains("laws") && wj["laws"].is_array()) {
        for (const auto& v : wj["laws"]) if (v.is_string()) world_.laws.push_back(v.get<std::string>());
    }
    if (wj.contains("religions") && wj["religions"].is_array()) {
        for (const auto& v : wj["religions"]) if (v.is_string()) world_.religions.push_back(v.get<std::string>());
    }
    if (wj.contains("agents") && wj["agents"].is_array()) {
        for (const auto& aj : wj["agents"]) {
            Agent a;
            a.id = jsonutil::getInt(aj, "id", 0);
            a.name = jsonutil::getString(aj, "name", "Agent");
            a.life = jsonutil::getFloat(aj, "life", 100.0f);
            a.intelligence = jsonutil::getFloat(aj, "intelligence", 50.0f);
            a.emotion = jsonutil::getFloat(aj, "emotion", 50.0f);
            a.adaptability = jsonutil::getFloat(aj, "adaptability", 50.0f);
            a.creativity = jsonutil::getFloat(aj, "creativity", 50.0f);
            a.sociality = jsonutil::getFloat(aj, "sociality", 50.0f);
            a.faith_bias = jsonutil::getFloat(aj, "faith_bias", 30.0f);
            a.memory = jsonutil::getFloat(aj, "memory", 50.0f);
            a.resource = jsonutil::getFloat(aj, "resource", 50.0f);
            if (aj.contains("position")) a.position = vecFromJson(aj["position"]);
            if (aj.contains("velocity")) a.velocity = vecFromJson(aj["velocity"]);
            a.state = parseAgentState(jsonutil::getString(aj, "state", "Alive"));
            world_.agents.push_back(a);
        }
    }
    if (wj.contains("concepts") && wj["concepts"].is_array()) {
        for (const auto& cj : wj["concepts"]) {
            Concept c;
            c.id = jsonutil::getInt(cj, "id", 0);
            c.name = jsonutil::getString(cj, "name", "Concept");
            c.strength = jsonutil::getFloat(cj, "strength", 1.0f);
            if (cj.contains("parents") && cj["parents"].is_array()) c.parents = cj["parents"].get<std::vector<int>>();
            c.origin_cycle = jsonutil::getInt(cj, "origin_cycle", 0);
            if (cj.contains("position")) c.position = vecFromJson(cj["position"]);
            if (cj.contains("linked_agents") && cj["linked_agents"].is_array()) c.linked_agents = cj["linked_agents"].get<std::vector<int>>();
            world_.concepts.push_back(c);
        }
    }
    if (wj.contains("structures") && wj["structures"].is_array()) {
        for (const auto& sj : wj["structures"]) {
            Structure s;
            s.name = jsonutil::getString(sj, "name", "Structure");
            if (sj.contains("position")) s.position = vecFromJson(sj["position"]);
            s.size = jsonutil::getFloat(sj, "size", 12.0f);
            s.origin_cycle = jsonutil::getInt(sj, "origin_cycle", 0);
            world_.structures.push_back(s);
        }
    }
    if (wj.contains("anomalies") && wj["anomalies"].is_array()) {
        for (const auto& aj : wj["anomalies"]) {
            ActiveAnomaly a;
            a.id = jsonutil::getInt(aj, "id", 0);
            AnomalyType type = AnomalyType::OutsideSignal;
            parseAnomalyType(jsonutil::getString(aj, "type", "OutsideSignal"), type);
            a.type = type;
            a.strength = jsonutil::getFloat(aj, "strength", 50.0f);
            a.remainingCycles = jsonutil::getInt(aj, "remaining_cycles", 60);
            if (aj.contains("position")) a.position = vecFromJson(aj["position"]);
            a.radius = jsonutil::getFloat(aj, "radius", 160.0f);
            a.phase = jsonutil::getFloat(aj, "phase", 0.0f);
            a.pulseFired = jsonutil::getBool(aj, "pulse_fired", false);
            world_.activeAnomalies.push_back(a);
        }
    }
    if (wj.contains("schedule") && wj["schedule"].is_array()) {
        for (const auto& sj : wj["schedule"]) {
            ScheduledAnomaly s;
            s.cycle = jsonutil::getInt(sj, "cycle", 0);
            s.type = jsonutil::getString(sj, "type", "");
            s.strength = jsonutil::getFloat(sj, "strength", 50.0f);
            s.fired = jsonutil::getBool(sj, "fired", false);
            world_.schedule.push_back(s);
        }
    }
    if (wj.contains("events") && wj["events"].is_array()) {
        for (const auto& ej : wj["events"]) {
            Event e;
            e.cycle = jsonutil::getInt(ej, "cycle", 0);
            e.type = parseEventType(jsonutil::getString(ej, "type", "SYSTEM"));
            e.name = jsonutil::getString(ej, "name", "");
            e.source = jsonutil::getString(ej, "source", "");
            e.target = jsonutil::getString(ej, "target", "");
            e.value = jsonutil::getFloat(ej, "value", 0.0f);
            e.message = jsonutil::getString(ej, "message", "");
            e.tag = jsonutil::getString(ej, "tag", "");
            world_.events.push_back(e);
        }
    }
    if (wj.contains("civilization_log" ) && wj["civilization_log"].is_array()) {
        for (const auto& sj : wj["civilization_log"]) {
            CivilizationSample s;
            s.cycle = jsonutil::getInt(sj, "cycle", 0);
            s.population = jsonutil::getInt(sj, "population", 0);
            s.stage = parseStage(jsonutil::getString(sj, "stage", "Primitive"));
            s.resources = jsonutil::getFloat(sj, "resources", 0.0f);
            s.concepts = jsonutil::getInt(sj, "concepts", 0);
            s.laws = jsonutil::getInt(sj, "laws", 0);
            s.religions = jsonutil::getInt(sj, "religions", 0);
            s.structures = jsonutil::getInt(sj, "structures", 0);
            s.stability = jsonutil::getFloat(sj, "stability", 0.0f);
            s.collapse_risk = jsonutil::getFloat(sj, "collapse_risk", 0.0f);
            s.unknown_score = jsonutil::getFloat(sj, "unknown_score", 0.0f);
            world_.civilizationLog.push_back(s);
        }
    }

    world_.rebuildNextIds();
    rng_.seed(world_.worldSeed == 0 ? settings.sim.seed : world_.worldSeed);
    active_ = true;
    running_ = false;
    finished_ = jsonutil::getBool(root, "finished", false);
    maxMode_ = false;
    accumulator_ = 0.0f;
    maxCycle_ = jsonutil::getInt(root, "max_cycle", settings.sim.maxCycle);
    previousPopulation_ = world_.civ.population;
    collapseRiskWarningActive_ = world_.civ.collapse_risk >= 70.0f;
    resourceWarningActive_ = world_.resourceTotal <= 25.0;
    if (world_.civilizationLog.empty()) world_.recordCivilizationSample();
    Logger::info("Simulation loaded from JSON");
    return true;
}

bool Simulation::saveToFile(const std::string& path) const {
    return jsonutil::writeFile(path, toJson());
}

bool Simulation::loadFromFile(const std::string& path, Settings& settings) {
    auto j = jsonutil::readFile(path);
    if (!j) return false;
    const bool ok = fromJson(*j, settings);
    if (ok) Logger::info("Loaded simulation: " + path);
    return ok;
}

void Simulation::start(const ScenarioConfig& cfg, const Settings& settings) {
    config_ = cfg;
    settings_ = settings;

    unsigned int seed = settings.sim.seed;
    if (cfg.randomize) {
        seed = static_cast<unsigned int>(std::time(nullptr));
    }
    rng_.seed(seed);

    world_.clear();
    world_.worldSeed = seed;
    world_.resourceTotal = cfg.resourceTotal;
    world_.environmentPressure = cfg.environmentPressure;
    world_.anomalyPressure = cfg.anomalyPressure;
    world_.experimentBhGvHh = cfg.experimentBhGvHh;
    world_.schedule = cfg.schedule;
    world_.experimentResult = cfg.experimentBhGvHh ? "Running" : "Stable";

    maxCycle_ = settings.sim.maxCycle > 0 ? settings.sim.maxCycle : cfg.maxCycle;

    const int n = std::max(1, cfg.initialAgents);
    for (int i = 0; i < n; ++i) {
        world_.agents.push_back(spawnAgentAroundCenter());
    }
    seedInitialConcepts();

    world_.civ.stage = Stage::Primitive;
    recomputeCivilization();

    active_ = true;
    running_ = true;
    finished_ = false;
    maxMode_ = false;
    accumulator_ = 0.0f;
    previousPopulation_ = world_.civ.population;
    collapseRiskWarningActive_ = false;
    resourceWarningActive_ = false;

    Event e;
    e.cycle = 0;
    e.type = EventType::System;
    e.name = cfg.name;
    e.source = "System";
    e.target = "World";
    e.value = (float)seed;
    e.message = "シミュレーション開始: " + cfg.name + "（シード " + std::to_string(seed) + "）";
    e.tag = "sim_start";
    world_.pushEvent(e);
}

void Simulation::reset() {
    if (active_) {
        Logger::info("Simulation reset");
        start(config_, settings_);
    }
}

Agent Simulation::spawnAgentAroundCenter() {
    Agent a;
    a.id = world_.newAgentId();
    a.name = Agent::makeName(rng_);
    a.life = rng_.nextFloat(70.0f, 100.0f);
    a.intelligence = rng_.nextFloat(30.0f, 80.0f);
    a.emotion = rng_.nextFloat(30.0f, 80.0f);
    a.adaptability = rng_.nextFloat(30.0f, 80.0f);
    a.creativity = clampf(rng_.nextFloat(20.0f, 70.0f) + config_.creativityBias * 40.0f, 0.0f, 100.0f);
    a.sociality = rng_.nextFloat(30.0f, 80.0f);
    a.faith_bias = clampf(rng_.nextFloat(10.0f, 50.0f) + config_.faithBias * 60.0f, 0.0f, 100.0f);
    a.memory = rng_.nextFloat(40.0f, 80.0f);
    a.resource = rng_.nextFloat(40.0f, 70.0f);

    const float ang = rng_.nextFloat(0.0f, 2.0f * PI);
    const float rad = rng_.nextFloat(40.0f, 240.0f);
    a.position = { world_.center.x + std::cos(ang) * rad,
                   world_.center.y + std::sin(ang) * rad };
    a.velocity = { rng_.nextFloat(-4.0f, 4.0f), rng_.nextFloat(-4.0f, 4.0f) };
    a.state = AgentState::Alive;
    return a;
}

void Simulation::seedInitialConcepts() {
    auto names = primitiveConceptNames();
    // Fisher-Yates partial shuffle for distinct picks.
    for (int i = (int)names.size() - 1; i > 0; --i) {
        std::swap(names[i], names[rng_.nextInt(0, i)]);
    }
    const int n = std::min<int>(5, (int)names.size());
    for (int k = 0; k < n; ++k) {
        Concept c;
        c.id = world_.newConceptId();
        c.name = names[k];
        c.strength = rng_.nextFloat(1.0f, 3.0f);
        c.origin_cycle = 0;
        const float ang = rng_.nextFloat(0.0f, 2.0f * PI);
        const float rad = rng_.nextFloat(120.0f, 360.0f);
        c.position = { world_.center.x + std::cos(ang) * rad,
                       world_.center.y + std::sin(ang) * rad };
        world_.concepts.push_back(c);
    }
    world_.civ.concept_count = (int)world_.concepts.size();
}

void Simulation::stepCycle() {
    world_.cycle++;

    // The population produces resources each cycle (scaled down by environment
    // pressure); a healthy society therefore accumulates a surplus.
    const double regen = world_.aliveAgents() * 2.2 * (1.0 - world_.environmentPressure);
    world_.resourceTotal = std::min(world_.resourceTotal + regen,
                                    config_.resourceTotal * 2.0);

    // Civilization systems (order matters: agents act, then their outputs feed
    // concept/law/religion/structure emergence).
    agentSystem_.update(world_, rng_);
    conceptSystem_.update(world_, rng_);
    lawSystem_.update(world_, rng_);
    religionSystem_.update(world_, rng_);
    structureSystem_.update(world_, rng_);
    anomalySystem_.update(world_, rng_);

    recomputeCivilization();
    updateStage();
    emitThresholdWarnings();
    world_.recordCivilizationSample();

    if (world_.civ.population == 0 && world_.civ.stage != Stage::Collapsed) {
        world_.civ.stage = Stage::Collapsed;
        if (world_.experimentBhGvHh && world_.experimentResult == "Running") {
            world_.experimentResult = "Erased";
        }
        running_ = false;
        finished_ = true;
        Event e;
        e.cycle = world_.cycle;
        e.type = EventType::Civilization;
        e.name = "Collapsed";
        e.source = "System";
        e.target = "World";
        e.value = 1.0f;
        e.message = "文明が崩壊しました（人口が0に到達）";
        e.tag = "collapse";
        world_.pushEvent(e);
        world_.recordCivilizationSample();
        return;
    }

    if (world_.cycle >= maxCycle_) {
        if (world_.experimentBhGvHh && world_.experimentResult == "Running") {
            if (world_.civ.population == 0) world_.experimentResult = "Erased";
            else if (world_.civ.stage == Stage::PostSingularity) world_.experimentResult = "PostSingularity";
            else if (world_.civ.stage == Stage::Unknown || world_.civ.unknown_score >= 75.0f) world_.experimentResult = "Unknown";
            else if (world_.civ.stability < 25.0f) world_.experimentResult = "Collapsed";
            else if (world_.civ.unknown_score >= 35.0f) world_.experimentResult = "Transferred";
            else world_.experimentResult = "Stable";
        }
        running_ = false;
        finished_ = true;
        Event e;
        e.cycle = world_.cycle;
        e.type = EventType::System;
        e.name = "End";
        e.source = "System";
        e.target = "World";
        e.value = (float)maxCycle_;
        e.message = "最終サイクル " + std::to_string(maxCycle_) + " に到達しました";
        e.tag = "sim_end";
        world_.pushEvent(e);
    }

    previousPopulation_ = world_.civ.population;
}

void Simulation::recomputeCivilization() {
    Civilization& c = world_.civ;
    c.population = world_.aliveAgents();
    c.resources = world_.resourceTotal;
    c.concept_count = (int)world_.concepts.size();
    c.law_count = (int)world_.laws.size();
    c.religion_count = (int)world_.religions.size();
    c.structure_count = (int)world_.structures.size();

    // Stability tracks agent wellbeing (life + personal resource), reinforced
    // by institutions and eroded by environment/anomaly pressure.
    float avgLife = 0.0f, avgRes = 0.0f;
    int n = 0;
    for (const auto& a : world_.agents) {
        if (!a.alive()) continue;
        avgLife += a.life; avgRes += a.resource; ++n;
    }
    if (n > 0) { avgLife /= n; avgRes /= n; }
    const float wellbeing = avgLife * 0.5f + avgRes * 0.5f;   // 0..100
    float stab = wellbeing * 0.75f
                 + c.law_count * 1.5f
                 + c.structure_count * 0.6f
                 + c.religion_count * 0.4f
                 - world_.environmentPressure * 20.0f
                 - world_.anomalyPressure * 45.0f;
    if (c.population == 0) stab = 0.0f;
    c.stability = clampf(stab, 0.0f, 100.0f);
    c.collapse_risk = clampf(100.0f - c.stability + (c.population <= 1 ? 20.0f : 0.0f),
                             0.0f, 100.0f);

    c.evolution_score = c.concept_count * 1.0f + c.law_count * 5.0f +
                        c.religion_count * 4.0f + c.structure_count * 6.0f +
                        c.population * 0.5f + c.stability * 0.2f;

    world_.stability = c.stability;
    world_.collapseRisk = c.collapse_risk;
    world_.conceptDensity = (float)world_.concepts.size() / 20.0f;
}

void Simulation::updateStage() {
    Civilization& c = world_.civ;
    if (c.stage == Stage::Collapsed || c.stage == Stage::Unknown) return;

    const int pop = c.population;
    const int concepts = c.concept_count;
    const int laws = c.law_count;
    const int structures = c.structure_count;
    const bool hasAiOrProtocol = world_.hasConcept("AI") || world_.hasConcept("Protocol");

    // Highest stage the current metrics justify (linear path Primitive..PostSingularity).
    int target = (int)Stage::Primitive;
    if (laws >= 1 && pop >= 8)                                              target = (int)Stage::Tribal;
    if (target >= (int)Stage::Tribal && structures >= 1 && pop >= 12)       target = (int)Stage::Agricultural;
    if (target >= (int)Stage::Agricultural && structures >= 3 &&
        concepts >= 15 && laws >= 2)                                        target = (int)Stage::Industrial;
    if (target >= (int)Stage::Industrial && concepts >= 30 && laws >= 3 &&
        hasAiOrProtocol)                                                    target = (int)Stage::Digital;
    if (target >= (int)Stage::Digital && concepts >= 60 &&
        c.unknown_score > 40.0f)                                           target = (int)Stage::PostSingularity;

    const int cur = (int)c.stage;
    if (cur < target && cur < (int)Stage::PostSingularity) {
        c.stage = (Stage)(cur + 1);
        Event e;
        e.cycle = world_.cycle; e.type = EventType::Civilization;
        e.name = stageName(c.stage); e.source = "System"; e.target = "World";
        e.value = 1.0f;
        e.message = std::string("文明段階が「") + jpStageName(c.stage) + "」へ進化しました";
        e.tag = (c.stage == Stage::PostSingularity) ? "post_singularity" : "stage_change";
        world_.pushEvent(e);
    }
}

void Simulation::emitThresholdWarnings() {
    const Civilization& c = world_.civ;
    if (c.collapse_risk >= 70.0f && !collapseRiskWarningActive_) {
        collapseRiskWarningActive_ = true;
        Event e;
        e.cycle = world_.cycle;
        e.type = EventType::Civilization;
        e.name = "CollapseRisk";
        e.source = "Civilization";
        e.target = "World";
        e.value = c.collapse_risk;
        e.message = "崩壊危険度が70%以上に到達しました";
        e.tag = "collapse_risk";
        world_.pushEvent(e);
    } else if (c.collapse_risk < 60.0f) {
        collapseRiskWarningActive_ = false;
    }

    if (world_.resourceTotal <= 25.0 && !resourceWarningActive_) {
        resourceWarningActive_ = true;
        Event e;
        e.cycle = world_.cycle;
        e.type = EventType::Civilization;
        e.name = "ResourceDepletion";
        e.source = "World";
        e.target = "Agents";
        e.value = (float)world_.resourceTotal;
        e.message = "資源が枯渇しました";
        e.tag = "resource_depleted";
        world_.pushEvent(e);
    } else if (world_.resourceTotal > 80.0) {
        resourceWarningActive_ = false;
    }

    if (previousPopulation_ > 0 && c.population <= (int)(previousPopulation_ * 0.65f)) {
        Event e;
        e.cycle = world_.cycle;
        e.type = EventType::Civilization;
        e.name = "PopulationDrop";
        e.source = "Agents";
        e.target = "Civilization";
        e.value = (float)c.population;
        e.message = "人口が急減しました";
        e.tag = "population_drop";
        world_.pushEvent(e);
    }
}

void Simulation::update(float dt, float speed) {
    if (!active_ || !running_ || finished_) return;

    const float effSpeed = maxMode_ ? 60.0f : std::max(0.01f, speed);
    accumulator_ += dt * effSpeed;

    const int maxStepsPerFrame = maxMode_ ? 2000 : 60;
    int steps = 0;
    while (accumulator_ >= secondsPerCycle_ && steps < maxStepsPerFrame) {
        stepCycle();
        accumulator_ -= secondsPerCycle_;
        ++steps;
        if (finished_) break;
    }
}

void Simulation::step() {
    if (active_ && !finished_) {
        stepCycle();
    }
}

bool Simulation::injectAnomaly(const std::string& type, float strength) {
    if (!active_) return false;
    const bool ok = anomalySystem_.inject(world_, rng_, type, strength, "Manual");
    if (ok) {
        recomputeCivilization();
        world_.recordCivilizationSample();
    }
    return ok;
}

bool Simulation::injectAnomaly(AnomalyType type, float strength) {
    if (!active_) return false;
    const bool ok = anomalySystem_.inject(world_, rng_, type, strength, "Manual");
    if (ok) {
        recomputeCivilization();
        world_.recordCivilizationSample();
    }
    return ok;
}

void Simulation::startBhGvHhExperiment() {
    if (!active_) return;
    anomalySystem_.startBhGvHhExperiment(world_, rng_);
    recomputeCivilization();
    world_.recordCivilizationSample();
}

void Simulation::togglePause() {
    if (!active_ || finished_) {
        running_ = false;
        return;
    }
    running_ = !running_;
}

} // namespace aic
