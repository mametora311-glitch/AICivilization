#pragma once

#include "notify/AlertTemplate.hpp"

#include <optional>

namespace aic {

class AlertRouter {
public:
    static std::optional<AlertEvent> route(const Event& event);
    static AlertEvent testAlert(const std::string& type);
};

} // namespace aic
