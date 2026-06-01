#pragma once

#include "raylib.h"

#include <string>

namespace aic {

class Random;

// Agent lifecycle states (spec section 7.1).
enum class AgentState {
    Alive,
    Hungry,
    Mutating,
    Reproducing,
    Believing,
    Collapsed,
    Dead,
    Unknown
};

const char* agentStateName(AgentState s);
Color agentStateColor(AgentState s);

// Smallest unit of the civilization. Attributes are on a 0..100 scale.
struct Agent {
    int id = 0;
    std::string name;

    float life = 100.0f;
    float intelligence = 50.0f;
    float emotion = 50.0f;
    float adaptability = 50.0f;
    float creativity = 50.0f;
    float sociality = 50.0f;
    float faith_bias = 30.0f;

    float memory = 50.0f;
    float resource = 50.0f;

    Vector2 position{ 0.0f, 0.0f };
    Vector2 velocity{ 0.0f, 0.0f };
    AgentState state = AgentState::Alive;

    bool alive() const {
        return state != AgentState::Dead && state != AgentState::Collapsed;
    }

    // Generates a pronounceable random name (e.g. "Melora", "Kisuna").
    static std::string makeName(Random& rng);
};

} // namespace aic
