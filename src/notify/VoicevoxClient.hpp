#pragma once

#include <string>

namespace aic {

class VoicevoxClient {
public:
    bool synthesizeToFile(const std::string& endpoint, int speaker,
                          const std::string& text, const std::string& wavPath,
                          std::string& resultMessage);
};

} // namespace aic
