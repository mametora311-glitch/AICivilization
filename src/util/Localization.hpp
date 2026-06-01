#pragma once

#include "anomaly/Anomaly.hpp"
#include "simulation/Agent.hpp"
#include "simulation/Civilization.hpp"
#include "simulation/World.hpp"

#include <string>

namespace aic {

const char* jpStageName(Stage stage);
const char* jpAgentStateName(AgentState state);
const char* jpEventTypeName(EventType type);
const char* jpAnomalyTypeName(AnomalyType type);

std::string jpDomainName(const std::string& name);
std::string jpExperimentResult(const std::string& result);

} // namespace aic
