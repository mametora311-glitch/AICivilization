#pragma once

#include "anomaly/Anomaly.hpp"

namespace aic {

struct WhiteHole {
    static constexpr int durationCycles = 80;
    static constexpr float baseRadius = 230.0f;

    static ActiveAnomaly create(int id, float strength, Vector2 position) {
        ActiveAnomaly a;
        a.id = id;
        a.type = AnomalyType::WhiteHole;
        a.strength = strength;
        a.remainingCycles = durationCycles;
        a.position = position;
        a.radius = baseRadius + strength * 1.8f;
        return a;
    }
};

} // namespace aic
