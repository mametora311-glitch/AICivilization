#pragma once

#include "simulation/World.hpp"

#include <string>

namespace aic {

class SurvivalReasoner {
public:
    static std::string result(const World& world);
    static std::string collapseRecoverySummary(const World& world);
};

} // namespace aic
