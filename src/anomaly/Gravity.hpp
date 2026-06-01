#pragma once

#include "anomaly/Anomaly.hpp"

namespace aic {

struct Gravity {
    static constexpr int durationCycles = 65;
    static constexpr float baseRadius = 260.0f;

    static ActiveAnomaly create(int id, float strength, Vector2 position) {
        ActiveAnomaly a;
        a.id = id;
        a.type = AnomalyType::Gravity;
        a.strength = strength;
        a.remainingCycles = durationCycles;
        a.position = position;
        a.radius = baseRadius + strength * 1.6f;
        return a;
    }
};

} // namespace aic
