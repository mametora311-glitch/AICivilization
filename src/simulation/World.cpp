#include "simulation/World.hpp"

#include "util/Localization.hpp"
#include "util/Logger.hpp"

namespace aic {

const char* eventTypeName(EventType t) {
    switch (t) {
        case EventType::Agent:        return "AGENT";
        case EventType::Concept:      return "CONCEPT";
        case EventType::Law:          return "LAW";
        case EventType::Religion:     return "RELIGION";
        case EventType::Structure:    return "STRUCTURE";
        case EventType::Anomaly:      return "ANOMALY";
        case EventType::Civilization: return "CIVILIZATION";
        case EventType::System:       return "SYSTEM";
    }
    return "SYSTEM";
}

void World::clear() {
    cycle = 0;
    resourceTotal = 1000.0;
    environmentPressure = 0.2f;
    conceptDensity = 0.0f;
    anomalyPressure = 0.0f;
    stability = 90.0f;
    collapseRisk = 5.0f;
    agents.clear();
    concepts.clear();
    laws.clear();
    religions.clear();
    structures.clear();
    activeAnomalies.clear();
    schedule.clear();
    events.clear();
    civilizationLog.clear();
    experimentBhGvHh = false;
    experimentResult = "Stable";
    civ = Civilization{};
    nextAgentId_ = 1;
    nextConceptId_ = 1;
    nextAnomalyId_ = 1;
}

bool World::hasConcept(const std::string& name) const {
    for (const auto& c : concepts) {
        if (c.name == name) return true;
    }
    return false;
}

int World::aliveAgents() const {
    int n = 0;
    for (const auto& a : agents) {
        if (a.alive()) ++n;
    }
    return n;
}

void World::pushEvent(const Event& e) {
    events.push_back(e);
    Logger::info("[サイクル " + std::to_string(e.cycle) + "] " +
                 jpEventTypeName(e.type) + ": " + e.message);
}

void World::rebuildNextIds() {
    nextAgentId_ = 1;
    nextConceptId_ = 1;
    nextAnomalyId_ = 1;
    for (const auto& a : agents) nextAgentId_ = std::max(nextAgentId_, a.id + 1);
    for (const auto& c : concepts) nextConceptId_ = std::max(nextConceptId_, c.id + 1);
    for (const auto& a : activeAnomalies) nextAnomalyId_ = std::max(nextAnomalyId_, a.id + 1);
}

void World::recordCivilizationSample() {
    CivilizationSample s;
    s.cycle = cycle;
    s.population = civ.population;
    s.stage = civ.stage;
    s.resources = civ.resources;
    s.concepts = civ.concept_count;
    s.laws = civ.law_count;
    s.religions = civ.religion_count;
    s.structures = civ.structure_count;
    s.stability = civ.stability;
    s.collapse_risk = civ.collapse_risk;
    s.unknown_score = civ.unknown_score;
    civilizationLog.push_back(s);
}

} // namespace aic
