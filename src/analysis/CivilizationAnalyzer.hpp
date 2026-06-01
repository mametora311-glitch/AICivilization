#pragma once

#include "simulation/Simulation.hpp"

#include <string>

namespace aic {

class CivilizationAnalyzer {
public:
    static std::string finalReport(const Simulation& simulation);
};

} // namespace aic
