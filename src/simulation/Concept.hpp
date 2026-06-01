#pragma once

#include "raylib.h"

#include <string>
#include <vector>

namespace aic {

// A concept emerges from agents or anomalies and can combine with others
// (spec section 7.2).
struct Concept {
    int id = 0;
    std::string name;
    float strength = 1.0f;
    std::vector<int> parents;        // parent concept ids (empty = primitive)
    long origin_cycle = 0;
    Vector2 position{ 0.0f, 0.0f };
    std::vector<int> linked_agents;  // agent ids associated with this concept
};

// Candidate primitive concept names (spec section 7.2).
const std::vector<std::string>& primitiveConceptNames();

} // namespace aic
