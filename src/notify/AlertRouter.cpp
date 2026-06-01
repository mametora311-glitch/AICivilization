#include "notify/AlertRouter.hpp"

namespace aic {

std::optional<AlertEvent> AlertRouter::route(const Event& event) {
    const std::string& tag = event.tag;
    if (tag == "blackhole_start") return AlertTemplate::blackHole(event.cycle);
    if (tag == "gravity_start") return AlertTemplate::gravity(event.cycle);
    if (tag == "whitehole_start") return AlertTemplate::whiteHole(event.cycle);
    if (tag == "collapse") return AlertTemplate::collapse(event.cycle);
    if (tag == "post_singularity") return AlertTemplate::postSingularity(event.cycle);

    if (tag == "emergency") return AlertTemplate::generic(event, AlertLevel::Emergency, "緊急速報: ");
    if (tag == "unknown_stress" || tag == "memory_eater" ||
        tag == "collapse_risk" || tag == "resource_depleted" ||
        tag == "population_drop" || tag == "warning") {
        return AlertTemplate::generic(event, AlertLevel::Warning, "警告: ");
    }
    if (tag == "law_created" || tag == "religion_created" ||
        tag == "stage_change" || tag == "experiment_result") {
        return AlertTemplate::generic(event, AlertLevel::Notice, "速報: ");
    }
    if (tag == "agent_born" || tag == "concept_created" ||
        tag == "structure_created") {
        AlertEvent a = AlertTemplate::generic(event, AlertLevel::Info, "情報: ");
        a.speakByVoicevox = false;
        return a;
    }
    return std::nullopt;
}

AlertEvent AlertRouter::testAlert(const std::string& type) {
    Event e;
    e.cycle = 0;
    e.type = EventType::System;
    if (type == "Windows") e.name = "Windows通知";
    else if (type == "VoiceVox") e.name = "VOICEVOX読み上げ";
    else e.name = "画面内通知";
    e.source = "設定";
    e.target = "通知";
    e.value = 1.0f;
    e.message = e.name + "のテストです。";
    e.tag = "warning";
    return AlertTemplate::generic(e, AlertLevel::Warning, "テスト: ");
}

} // namespace aic
