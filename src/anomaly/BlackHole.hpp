#pragma once

#include "anomaly/Anomaly.hpp"

namespace aic {

struct BlackHole {
    static constexpr int durationCycles = 70;
    static constexpr float baseRadius = 210.0f;

    static ActiveAnomaly create(int id, float strength, Vector2 position) {
        ActiveAnomaly a;
        a.id = id;
        a.type = AnomalyType::BlackHole;
        a.strength = strength;
        a.remainingCycles = durationCycles;
        a.position = position;
        a.radius = baseRadius + strength * 1.2f;
        return a;
    }
};

} // namespace aic
