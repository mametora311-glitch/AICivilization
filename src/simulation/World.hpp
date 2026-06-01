#pragma once

#include "raylib.h"

#include "anomaly/Anomaly.hpp"
#include "simulation/Agent.hpp"
#include "simulation/Civilization.hpp"
#include "simulation/Concept.hpp"

#include <string>
#include <vector>

namespace aic {

// Event category (matches the "type" column of event_log.csv, spec section 12).
enum class EventType {
    Agent,
    Concept,
    Law,
    Religion,
    Structure,
    Anomaly,
    Civilization,
    System
};

const char* eventTypeName(EventType t);

// A notable simulation event. Columns mirror event_log.csv:
// cycle,type,name,source,target,value,message
struct Event {
    long cycle = 0;
    EventType type = EventType::System;
    std::string name;
    std::string source;
    std::string target;
    float value = 0.0f;
    std::string message;

    // Internal routing key (not part of the CSV columns): maps the event to a
    // sound effect and notification template. Examples:
    // "agent_born", "agent_dead", "concept_created", "blackhole_start".
    std::string tag;
};

// A scenario-scheduled anomaly injection consumed by AnomalySystem.
struct ScheduledAnomaly {
    long cycle = 0;
    std::string type;
    float strength = 50.0f;
    bool fired = false;
};

// A physical structure built by the civilization (rendered in the world view).
struct Structure {
    std::string name;
    Vector2 position{ 0.0f, 0.0f };
    float size = 12.0f;
    long origin_cycle = 0;
};

struct CivilizationSample {
    long cycle = 0;
    int population = 0;
    Stage stage = Stage::Primitive;
    double resources = 0.0;
    int concepts = 0;
    int laws = 0;
    int religions = 0;
    int structures = 0;
    float stability = 0.0f;
    float collapse_risk = 0.0f;
    float unknown_score = 0.0f;
};

// The closed environment that holds the whole simulation state (spec 5.1).
class World {
public:
    long cycle = 0;
    unsigned int worldSeed = 0;

    double resourceTotal = 1000.0;
    float environmentPressure = 0.2f;
    float conceptDensity = 0.0f;
    float anomalyPressure = 0.0f;
    float stability = 90.0f;
    float collapseRisk = 5.0f;

    Vector2 bounds{ 2000.0f, 1400.0f };
    Vector2 center{ 1000.0f, 700.0f };

    std::vector<Agent> agents;
    std::vector<Concept> concepts;
    std::vector<std::string> laws;
    std::vector<std::string> religions;
    std::vector<Structure> structures;
    std::vector<ActiveAnomaly> activeAnomalies;
    Civilization civ;

    std::vector<ScheduledAnomaly> schedule;
    bool experimentBhGvHh = false;
    std::string experimentResult = "Stable";

    std::vector<Event> events;   // full history (tail shown in the event log)
    std::vector<CivilizationSample> civilizationLog;

    bool hasConcept(const std::string& name) const;

    void clear();
    int  newAgentId()   { return nextAgentId_++; }
    int  newConceptId() { return nextConceptId_++; }
    int  newAnomalyId() { return nextAnomalyId_++; }
    void rebuildNextIds();
    void recordCivilizationSample();
    int  aliveAgents() const;

    // Appends an event, mirrors it to app.log and lets observers pick it up.
    // Notable events only -- routine per-cycle activity is silent.
    void pushEvent(const Event& e);

private:
    int nextAgentId_ = 1;
    int nextConceptId_ = 1;
    int nextAnomalyId_ = 1;
};

} // namespace aic
