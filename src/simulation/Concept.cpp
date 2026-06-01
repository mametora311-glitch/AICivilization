#include "simulation/Concept.hpp"

namespace aic {

const std::vector<std::string>& primitiveConceptNames() {
    static const std::vector<std::string> names = {
        "AI", "Dream", "Ocean", "Rain", "Memory", "Emotion", "BlackHole",
        "WhiteHole", "Gravity", "Loneliness", "Protocol", "Mind", "Law",
        "Religion", "Unknown"
    };
    return names;
}

} // namespace aic
