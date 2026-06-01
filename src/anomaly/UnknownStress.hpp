#pragma once

#include "anomaly/Anomaly.hpp"

namespace aic {

struct UnknownStress {
    static constexpr int durationCycles = 50;
    static constexpr float baseRadius = 180.0f;

    static ActiveAnomaly create(int id, AnomalyType type, float strength, Vector2 position) {
        ActiveAnomaly a;
        a.id = id;
        a.type = type;
        a.strength = strength;
        a.remainingCycles = durationCycles;
        a.position = position;
        a.radius = baseRadius + strength;
        return a;
    }
};

} // namespace aic
