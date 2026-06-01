#pragma once

#include "simulation/World.hpp"

#include <string>

namespace aic {

enum class AlertLevel {
    Info,
    Notice,
    Warning,
    Critical,
    Emergency
};

const char* alertLevelName(AlertLevel level);
int alertLevelIndex(AlertLevel level);

struct AlertEvent {
    long cycle = 0;
    AlertLevel level = AlertLevel::Info;
    std::string type;
    std::string title;
    std::string message;
    std::string ttsText;
    bool sendWindowsNotification = true;
    bool speakByVoicevox = true;
    bool showInApp = true;
    int cooldownSeconds = 15;
};

class AlertTemplate {
public:
    static AlertEvent blackHole(long cycle);
    static AlertEvent gravity(long cycle);
    static AlertEvent whiteHole(long cycle);
    static AlertEvent collapse(long cycle);
    static AlertEvent postSingularity(long cycle);
    static AlertEvent generic(const Event& event, AlertLevel level,
                              const std::string& titlePrefix);
};

} // namespace aic
