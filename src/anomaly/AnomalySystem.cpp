#include "anomaly/AnomalySystem.hpp"

#include "anomaly/BlackHole.hpp"
#include "anomaly/Gravity.hpp"
#include "anomaly/UnknownStress.hpp"
#include "anomaly/WhiteHole.hpp"
#include "simulation/World.hpp"
#include "util/Localization.hpp"
#include "util/Random.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace aic {

namespace {

float clampf(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

float dist(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

Vector2 randomNear(World& world, Random& rng, float minR, float maxR) {
    const float ang = rng.nextFloat(0.0f, 2.0f * PI);
    const float r = rng.nextFloat(minR, maxR);
    return { world.center.x + std::cos(ang) * r,
             world.center.y + std::sin(ang) * r };
}

std::string lowerAscii(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
        if (c == '_' || c == '-' || c == ' ') c = '\0';
    }
    s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
    return s;
}

void pushAnomalyEvent(World& world, const std::string& name, const std::string& source,
                      float strength, const std::string& message, const std::string& tag) {
    Event e;
    e.cycle = world.cycle;
    e.type = EventType::Anomaly;
    e.name = name;
    e.source = source;
    e.target = "Civilization";
    e.value = strength;
    e.message = message;
    e.tag = tag;
    world.pushEvent(e);
}

} // namespace

const char* anomalyTypeName(AnomalyType type) {
    switch (type) {
        case AnomalyType::BlackHole:     return "BlackHole";
        case AnomalyType::Gravity:       return "Gravity";
        case AnomalyType::WhiteHole:     return "WhiteHole";
        case AnomalyType::OutsideSignal: return "OutsideSignal";
        case AnomalyType::TimeFracture:  return "TimeFracture";
        case AnomalyType::MemoryEater:   return "MemoryEater";
        case AnomalyType::Eclipse:       return "Eclipse";
        case AnomalyType::Mirror:        return "Mirror";
        case AnomalyType::NoiseRain:     return "NoiseRain";
    }
    return "OutsideSignal";
}

bool parseAnomalyType(const std::string& text, AnomalyType& out) {
    const std::string s = lowerAscii(text);
    if (s == "blackhole" || s == "bh") { out = AnomalyType::BlackHole; return true; }
    if (s == "gravity" || s == "gv" || s == "gravityfield") { out = AnomalyType::Gravity; return true; }
    if (s == "whitehole" || s == "hh") { out = AnomalyType::WhiteHole; return true; }
    if (s == "outsidesignal") { out = AnomalyType::OutsideSignal; return true; }
    if (s == "timefracture") { out = AnomalyType::TimeFracture; return true; }
    if (s == "memoryeater") { out = AnomalyType::MemoryEater; return true; }
    if (s == "eclipse") { out = AnomalyType::Eclipse; return true; }
    if (s == "mirror") { out = AnomalyType::Mirror; return true; }
    if (s == "noiserain") { out = AnomalyType::NoiseRain; return true; }
    return false;
}

bool isUnknownStress(AnomalyType type) {
    return type == AnomalyType::OutsideSignal || type == AnomalyType::TimeFracture ||
           type == AnomalyType::MemoryEater || type == AnomalyType::Eclipse ||
           type == AnomalyType::Mirror || type == AnomalyType::NoiseRain;
}

Color anomalyColor(AnomalyType type) {
    switch (type) {
        case AnomalyType::BlackHole:     return Color{ 8, 8, 12, 235 };
        case AnomalyType::Gravity:       return Color{ 120, 160, 255, 210 };
        case AnomalyType::WhiteHole:     return Color{ 245, 245, 255, 230 };
        case AnomalyType::OutsideSignal: return Color{ 220, 70, 170, 190 };
        case AnomalyType::TimeFracture:  return Color{ 180, 90, 230, 190 };
        case AnomalyType::MemoryEater:   return Color{ 170, 40, 100, 200 };
        case AnomalyType::Eclipse:       return Color{ 110, 70, 160, 190 };
        case AnomalyType::Mirror:        return Color{ 160, 220, 235, 170 };
        case AnomalyType::NoiseRain:     return Color{ 230, 95, 120, 180 };
    }
    return MAGENTA;
}

bool AnomalySystem::inject(World& world, Random& rng, const std::string& type,
                           float strength, const std::string& source) {
    AnomalyType parsed;
    if (!parseAnomalyType(type, parsed)) return false;
    return inject(world, rng, parsed, strength, source);
}

bool AnomalySystem::inject(World& world, Random& rng, AnomalyType type, float strength,
                           const std::string& source) {
    strength = clampf(strength, 1.0f, 100.0f);
    const Vector2 p = (type == AnomalyType::BlackHole || type == AnomalyType::Gravity ||
                       type == AnomalyType::WhiteHole)
                    ? randomNear(world, rng, 40.0f, 120.0f)
                    : randomNear(world, rng, 120.0f, 430.0f);

    ActiveAnomaly anomaly;
    const int id = world.newAnomalyId();
    if (type == AnomalyType::BlackHole)      anomaly = BlackHole::create(id, strength, p);
    else if (type == AnomalyType::Gravity)   anomaly = Gravity::create(id, strength, p);
    else if (type == AnomalyType::WhiteHole) anomaly = WhiteHole::create(id, strength, p);
    else                                     anomaly = UnknownStress::create(id, type, strength, p);

    world.activeAnomalies.push_back(anomaly);
    world.anomalyPressure = clampf(world.anomalyPressure + strength / 420.0f, 0.0f, 1.0f);

    const std::string name = anomalyTypeName(type);
    ensureConcept(world, rng, name == "OutsideSignal" ? "Unknown" : name,
                  strength / 25.0f, p);

    std::string tag = "unknown_stress";
    std::string message = jpDomainName(name) + "が発生しました";
    if (type == AnomalyType::BlackHole) {
        tag = "blackhole_start";
        message = "ブラックホールが発生し、文明圧縮が開始されました";
        world.civ.collapse_risk = clampf(world.civ.collapse_risk + strength * 0.16f, 0.0f, 100.0f);
    } else if (type == AnomalyType::Gravity) {
        tag = "gravity_start";
        message = "重力場が起動し、文明断片の移送が始まりました";
    } else if (type == AnomalyType::WhiteHole) {
        tag = "whitehole_start";
        message = "ホワイトホール放出が始まり、圧縮概念が展開されています";
    } else if (type == AnomalyType::MemoryEater) {
        tag = "memory_eater";
        message = "記憶喰らいが発生し、エージェント記憶の汚染が始まりました";
    }

    pushAnomalyEvent(world, name, source, strength, message, tag);
    return true;
}

void AnomalySystem::startBhGvHhExperiment(World& world, Random& rng) {
    world.experimentBhGvHh = true;
    world.experimentResult = "Running";

    ScheduledAnomaly gv;
    gv.cycle = world.cycle + 50;
    gv.type = "Gravity";
    gv.strength = 55.0f;

    ScheduledAnomaly hh;
    hh.cycle = world.cycle + 80;
    hh.type = "WhiteHole";
    hh.strength = 90.0f;

    world.schedule.push_back(gv);
    world.schedule.push_back(hh);
    inject(world, rng, AnomalyType::BlackHole, 60.0f, "Experiment");
}

void AnomalySystem::processSchedule(World& world, Random& rng) {
    for (auto& scheduled : world.schedule) {
        if (scheduled.fired || scheduled.cycle > world.cycle) continue;
        inject(world, rng, scheduled.type, scheduled.strength, "Scenario");
        scheduled.fired = true;
    }
}

void AnomalySystem::ensureConcept(World& world, Random& rng, const std::string& name,
                                  float strength, Vector2 position, int parentA, int parentB) {
    for (auto& c : world.concepts) {
        if (c.name == name) {
            c.strength = clampf(c.strength + strength * 0.25f, 0.5f, 30.0f);
            return;
        }
    }

    Concept c;
    c.id = world.newConceptId();
    c.name = name;
    c.strength = clampf(strength, 0.5f, 30.0f);
    if (parentA > 0) c.parents.push_back(parentA);
    if (parentB > 0) c.parents.push_back(parentB);
    c.origin_cycle = world.cycle;
    c.position = { position.x + rng.nextFloat(-40.0f, 40.0f),
                   position.y + rng.nextFloat(-40.0f, 40.0f) };
    world.concepts.push_back(c);

    Event e;
    e.cycle = world.cycle;
    e.type = EventType::Concept;
    e.name = name;
    e.source = "Anomaly";
    e.target = "World";
    e.value = c.strength;
    e.message = "概念生成: " + jpDomainName(name);
    e.tag = "concept_created";
    world.pushEvent(e);
}

void AnomalySystem::applyBlackHole(World& world, Random& rng, ActiveAnomaly& anomaly) {
    const float pull = anomaly.strength * 0.0020f;
    int absorbed = 0;
    for (auto& a : world.agents) {
        if (!a.alive()) continue;
        const float d = std::max(1.0f, dist(a.position, anomaly.position));
        const float range = anomaly.radius * 1.15f;
        if (d > range) continue;
        const float f = (1.0f - d / range) * pull;
        a.velocity.x += (anomaly.position.x - a.position.x) * f;
        a.velocity.y += (anomaly.position.y - a.position.y) * f;
        a.resource = std::max(0.0f, a.resource - anomaly.strength * 0.005f);
        a.life -= anomaly.strength * 0.002f;
        if (d < anomaly.radius * 0.12f && rng.chance(anomaly.strength * 0.0002f)) {
            a.state = AgentState::Dead;
            ++absorbed;
        }
    }

    for (auto& c : world.concepts) {
        const float d = std::max(1.0f, dist(c.position, anomaly.position));
        if (d > anomaly.radius * 1.8f) continue;
        const float f = (1.0f - d / (anomaly.radius * 1.8f)) * pull * 0.55f;
        c.position.x += (anomaly.position.x - c.position.x) * f;
        c.position.y += (anomaly.position.y - c.position.y) * f;
        c.strength = clampf(c.strength + anomaly.strength * 0.001f, 0.5f, 30.0f);
    }

    if (absorbed > 0) {
        Event e;
        e.cycle = world.cycle;
        e.type = EventType::Agent;
        e.name = "BlackHoleAbsorption";
        e.source = "BlackHole";
        e.target = "Agents";
        e.value = (float)absorbed;
        e.message = std::to_string(absorbed) + "体のエージェントがブラックホールに吸収されました";
        e.tag = "warning";
        world.pushEvent(e);
    }

    world.resourceTotal = std::max(0.0, world.resourceTotal - anomaly.strength * 0.03);
    world.civ.collapse_risk = clampf(world.civ.collapse_risk + anomaly.strength * 0.006f, 0.0f, 100.0f);
    world.civ.unknown_score = clampf(world.civ.unknown_score + anomaly.strength * 0.010f, 0.0f, 100.0f);
}

void AnomalySystem::applyGravity(World& world, Random&, ActiveAnomaly& anomaly) {
    const float orbit = anomaly.strength * 0.0025f;
    for (auto& a : world.agents) {
        if (!a.alive()) continue;
        const float dx = a.position.x - anomaly.position.x;
        const float dy = a.position.y - anomaly.position.y;
        const float d = std::sqrt(dx * dx + dy * dy) + 0.001f;
        if (d > anomaly.radius * 1.5f) continue;
        const float bind = (1.0f - d / (anomaly.radius * 1.5f)) * orbit;
        a.velocity.x += (-dy / d) * anomaly.strength * 0.04f + (anomaly.position.x - a.position.x) * bind;
        a.velocity.y += ( dx / d) * anomaly.strength * 0.04f + (anomaly.position.y - a.position.y) * bind;
    }
    for (auto& c : world.concepts) {
        const float dx = c.position.x - anomaly.position.x;
        const float dy = c.position.y - anomaly.position.y;
        const float d = std::sqrt(dx * dx + dy * dy) + 0.001f;
        if (d > anomaly.radius * 1.8f) continue;
        c.position.x += (-dy / d) * anomaly.strength * 0.02f;
        c.position.y += ( dx / d) * anomaly.strength * 0.02f;
        c.strength = clampf(c.strength + anomaly.strength * 0.0007f, 0.5f, 30.0f);
    }
    world.civ.unknown_score = clampf(world.civ.unknown_score + anomaly.strength * 0.006f, 0.0f, 100.0f);
}

void AnomalySystem::applyWhiteHole(World& world, Random& rng, ActiveAnomaly& anomaly) {
    if (!anomaly.pulseFired) {
        anomaly.pulseFired = true;
        const std::vector<std::string> names = {
            "WhiteHoleProtocol", "MemoryOcean", "AIDream", "DreamMindAI",
            "GravityLaw", "UnknownSeed", "Reconstruction"
        };
        for (const auto& n : names) {
            ensureConcept(world, rng, n, anomaly.strength / 18.0f, anomaly.position);
        }
        for (auto& a : world.agents) {
            if (!a.alive()) continue;
            a.life = clampf(a.life + anomaly.strength * 0.18f, 0.0f, 100.0f);
            a.resource = clampf(a.resource + anomaly.strength * 0.12f, 0.0f, 100.0f);
            a.memory = clampf(a.memory + anomaly.strength * 0.10f, 0.0f, 100.0f);
            if (rng.chance(anomaly.strength * 0.0015f)) a.state = AgentState::Unknown;
        }
        world.resourceTotal += anomaly.strength * 8.0;
        world.civ.unknown_score = clampf(world.civ.unknown_score + anomaly.strength * 0.40f, 0.0f, 100.0f);
        world.civ.stability = clampf(world.civ.stability + anomaly.strength * 0.25f, 0.0f, 100.0f);
        if (world.civ.unknown_score >= 65.0f && world.aliveAgents() > 0) {
            world.civ.stage = Stage::PostSingularity;
            Event e;
            e.cycle = world.cycle;
            e.type = EventType::Civilization;
            e.name = "PostSingularity";
            e.source = "WhiteHole";
            e.target = "World";
            e.value = world.civ.unknown_score;
            e.message = "ホワイトホール放出後、文明がポストシンギュラリティに到達しました";
            e.tag = "post_singularity";
            world.pushEvent(e);
        }
    }

    for (auto& c : world.concepts) {
        const float dx = c.position.x - anomaly.position.x;
        const float dy = c.position.y - anomaly.position.y;
        const float d = std::sqrt(dx * dx + dy * dy) + 0.001f;
        if (d > anomaly.radius * 2.2f) continue;
        c.position.x += (dx / d) * anomaly.strength * 0.06f;
        c.position.y += (dy / d) * anomaly.strength * 0.06f;
    }
    world.anomalyPressure = clampf(world.anomalyPressure - anomaly.strength * 0.0009f, 0.0f, 1.0f);
}

void AnomalySystem::applyUnknownStress(World& world, Random& rng, ActiveAnomaly& anomaly) {
    int affected = 0;
    for (auto& a : world.agents) {
        if (!a.alive()) continue;
        if (!rng.chance(0.03f + anomaly.strength * 0.0008f)) continue;
        ++affected;
        if (anomaly.type == AnomalyType::MemoryEater) {
            a.memory = clampf(a.memory - anomaly.strength * 0.18f, 0.0f, 100.0f);
        } else if (anomaly.type == AnomalyType::Mirror) {
            std::swap(a.intelligence, a.emotion);
        } else if (anomaly.type == AnomalyType::TimeFracture) {
            a.velocity.x *= -0.7f;
            a.velocity.y *= -0.7f;
        } else if (anomaly.type == AnomalyType::Eclipse) {
            a.faith_bias = clampf(a.faith_bias + anomaly.strength * 0.10f, 0.0f, 100.0f);
            a.state = AgentState::Believing;
        } else if (anomaly.type == AnomalyType::NoiseRain) {
            a.creativity = clampf(a.creativity + rng.gaussian(0.0f, anomaly.strength * 0.06f), 0.0f, 100.0f);
            a.state = AgentState::Mutating;
        } else {
            a.adaptability = clampf(a.adaptability + anomaly.strength * 0.05f, 0.0f, 100.0f);
        }
    }

    if (!world.concepts.empty() && rng.chance(0.05f + anomaly.strength * 0.0008f)) {
        auto& c = world.concepts[(size_t)rng.nextInt(0, (int)world.concepts.size() - 1)];
        if (c.name != "Unknown" && c.name.find("Unknown") == std::string::npos &&
            c.name.size() <= 40) {
            c.name += "Unknown";
        }
        c.strength = clampf(c.strength + anomaly.strength * 0.03f, 0.5f, 30.0f);
    }

    if (affected > 0 && anomaly.type == AnomalyType::MemoryEater && !anomaly.pulseFired) {
        anomaly.pulseFired = true;
        Event e;
        e.cycle = world.cycle;
        e.type = EventType::Anomaly;
        e.name = "MemoryEater";
        e.source = "MemoryEater";
        e.target = "Agents";
        e.value = (float)affected;
        e.message = "記憶喰らいがエージェント記憶を汚染しました";
        e.tag = "memory_eater";
        world.pushEvent(e);
    }

    world.civ.unknown_score = clampf(world.civ.unknown_score + anomaly.strength * 0.010f, 0.0f, 100.0f);
    world.civ.stability = clampf(world.civ.stability - anomaly.strength * 0.012f, 0.0f, 100.0f);
}

void AnomalySystem::evaluateExperiment(World& world) {
    if (!world.experimentBhGvHh || world.experimentResult != "Running") return;

    bool hasB = false, hasG = false, hasW = false;
    for (const auto& s : world.schedule) {
        if (s.type == "BlackHole" && !s.fired) hasB = true;
        if (s.type == "Gravity" && !s.fired) hasG = true;
        if (s.type == "WhiteHole" && !s.fired) hasW = true;
    }
    for (const auto& a : world.activeAnomalies) {
        if (a.type == AnomalyType::BlackHole) hasB = true;
        if (a.type == AnomalyType::Gravity) hasG = true;
        if (a.type == AnomalyType::WhiteHole) hasW = true;
    }
    if (hasB || hasG || hasW) return;

    if (world.aliveAgents() == 0) world.experimentResult = "Erased";
    else if (world.civ.stage == Stage::PostSingularity) world.experimentResult = "PostSingularity";
    else if (world.civ.stage == Stage::Unknown || world.civ.unknown_score >= 75.0f) world.experimentResult = "Unknown";
    else if (world.civ.stability < 25.0f) world.experimentResult = "Collapsed";
    else if (world.civ.concept_count >= 25 && world.civ.structure_count >= 1) world.experimentResult = "Reconstructed";
    else if (world.civ.unknown_score >= 35.0f) world.experimentResult = "Transferred";
    else world.experimentResult = "Stable";

    Event e;
    e.cycle = world.cycle;
    e.type = EventType::Civilization;
    e.name = world.experimentResult;
    e.source = "Experiment";
    e.target = "Civilization";
    e.value = world.civ.unknown_score;
    e.message = "BH/GV/HH実験結果: " + jpExperimentResult(world.experimentResult);
    e.tag = (world.experimentResult == "Erased") ? "emergency" : "experiment_result";
    world.pushEvent(e);
}

void AnomalySystem::update(World& world, Random& rng) {
    processSchedule(world, rng);

    for (auto& anomaly : world.activeAnomalies) {
        anomaly.phase += 0.08f + anomaly.strength * 0.0008f;
        if (anomaly.type == AnomalyType::BlackHole) applyBlackHole(world, rng, anomaly);
        else if (anomaly.type == AnomalyType::Gravity) applyGravity(world, rng, anomaly);
        else if (anomaly.type == AnomalyType::WhiteHole) applyWhiteHole(world, rng, anomaly);
        else applyUnknownStress(world, rng, anomaly);
        --anomaly.remainingCycles;
    }

    world.activeAnomalies.erase(
        std::remove_if(world.activeAnomalies.begin(), world.activeAnomalies.end(),
                       [](const ActiveAnomaly& a) { return a.remainingCycles <= 0; }),
        world.activeAnomalies.end());

    world.agents.erase(
        std::remove_if(world.agents.begin(), world.agents.end(),
                       [](const Agent& a) { return a.state == AgentState::Dead; }),
        world.agents.end());

    if (world.activeAnomalies.empty()) {
        world.anomalyPressure = clampf(world.anomalyPressure * 0.995f, 0.0f, 1.0f);
    }
    evaluateExperiment(world);
}

} // namespace aic
