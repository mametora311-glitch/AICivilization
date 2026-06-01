#include "simulation/Agent.hpp"

#include "util/Random.hpp"

#include <array>

namespace aic {

const char* agentStateName(AgentState s) {
    switch (s) {
        case AgentState::Alive:       return "Alive";
        case AgentState::Hungry:      return "Hungry";
        case AgentState::Mutating:    return "Mutating";
        case AgentState::Reproducing: return "Reproducing";
        case AgentState::Believing:   return "Believing";
        case AgentState::Collapsed:   return "Collapsed";
        case AgentState::Dead:        return "Dead";
        case AgentState::Unknown:     return "Unknown";
    }
    return "Alive";
}

Color agentStateColor(AgentState s) {
    switch (s) {
        case AgentState::Alive:       return Color{ 120, 200, 255, 255 };
        case AgentState::Hungry:      return Color{ 230, 200, 90, 255 };
        case AgentState::Mutating:    return Color{ 200, 120, 230, 255 };
        case AgentState::Reproducing: return Color{ 120, 230, 150, 255 };
        case AgentState::Believing:   return Color{ 235, 190, 120, 255 };
        case AgentState::Collapsed:   return Color{ 120, 70, 70, 255 };
        case AgentState::Dead:        return Color{ 90, 90, 100, 255 };
        case AgentState::Unknown:     return Color{ 220, 80, 160, 255 };
    }
    return WHITE;
}

std::string Agent::makeName(Random& rng) {
    static const std::array<const char*, 16> heads = {
        "Me", "Ra", "Su", "Ze", "Ka", "Lu", "Ne", "To",
        "Vi", "Om", "Ny", "Ae", "Ko", "Sy", "Ul", "Ix"
    };
    static const std::array<const char*, 16> tails = {
        "lo", "ki", "na", "ro", "mi", "sa", "ya", "el",
        "ra", "no", "vi", "th", "an", "us", "ea", "or"
    };
    std::string n = heads[rng.nextInt(0, (int)heads.size() - 1)];
    n += tails[rng.nextInt(0, (int)tails.size() - 1)];
    if (rng.chance(0.4f)) n += tails[rng.nextInt(0, (int)tails.size() - 1)];
    return n;
}

} // namespace aic
