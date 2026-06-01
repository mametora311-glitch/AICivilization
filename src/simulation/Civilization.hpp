#pragma once

namespace aic {

// Civilization development stage (spec section 7.3).
enum class Stage {
    Primitive,
    Tribal,
    Agricultural,
    Industrial,
    Digital,
    PostSingularity,
    Collapsed,
    Unknown
};

const char* stageName(Stage s);

// Aggregate civilization state, recomputed each cycle from the agent/concept
// populations plus the system outputs (spec section 7.3).
struct Civilization {
    int population = 0;
    Stage stage = Stage::Primitive;
    double resources = 0.0;
    int concept_count = 0;
    int law_count = 0;
    int religion_count = 0;
    int structure_count = 0;
    float stability = 90.0f;
    float collapse_risk = 5.0f;
    float evolution_score = 0.0f;
    float unknown_score = 0.0f;
};

} // namespace aic
