#include "simulation/Civilization.hpp"

namespace aic {

const char* stageName(Stage s) {
    switch (s) {
        case Stage::Primitive:       return "Primitive";
        case Stage::Tribal:          return "Tribal";
        case Stage::Agricultural:    return "Agricultural";
        case Stage::Industrial:      return "Industrial";
        case Stage::Digital:         return "Digital";
        case Stage::PostSingularity: return "PostSingularity";
        case Stage::Collapsed:       return "Collapsed";
        case Stage::Unknown:         return "Unknown";
    }
    return "Primitive";
}

} // namespace aic
