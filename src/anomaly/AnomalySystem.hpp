#pragma once

#include "anomaly/Anomaly.hpp"

#include <string>

namespace aic {

class Random;
class World;

class AnomalySystem {
public:
    bool inject(World& world, Random& rng, AnomalyType type, float strength,
                const std::string& source = "System");
    bool inject(World& world, Random& rng, const std::string& type, float strength,
                const std::string& source = "System");
    void update(World& world, Random& rng);
    void startBhGvHhExperiment(World& world, Random& rng);

private:
    void processSchedule(World& world, Random& rng);
    void applyBlackHole(World& world, Random& rng, ActiveAnomaly& anomaly);
    void applyGravity(World& world, Random& rng, ActiveAnomaly& anomaly);
    void applyWhiteHole(World& world, Random& rng, ActiveAnomaly& anomaly);
    void applyUnknownStress(World& world, Random& rng, ActiveAnomaly& anomaly);
    void evaluateExperiment(World& world);
    void ensureConcept(World& world, Random& rng, const std::string& name,
                       float strength, Vector2 position, int parentA = 0,
                       int parentB = 0);
};

} // namespace aic
