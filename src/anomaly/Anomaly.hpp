#pragma once

#include "raylib.h"

#include <string>

namespace aic {

enum class AnomalyType {
    BlackHole,
    Gravity,
    WhiteHole,
    OutsideSignal,
    TimeFracture,
    MemoryEater,
    Eclipse,
    Mirror,
    NoiseRain
};

struct ActiveAnomaly {
    int id = 0;
    AnomalyType type = AnomalyType::OutsideSignal;
    float strength = 50.0f;
    int remainingCycles = 60;
    Vector2 position{ 0.0f, 0.0f };
    float radius = 160.0f;
    float phase = 0.0f;
    bool pulseFired = false;
};

const char* anomalyTypeName(AnomalyType type);
bool parseAnomalyType(const std::string& text, AnomalyType& out);
bool isUnknownStress(AnomalyType type);
Color anomalyColor(AnomalyType type);

} // namespace aic
