#include "notify/AlertTemplate.hpp"

#include "util/Localization.hpp"

namespace aic {

const char* alertLevelName(AlertLevel level) {
    switch (level) {
        case AlertLevel::Info:      return "情報";
        case AlertLevel::Notice:    return "速報";
        case AlertLevel::Warning:   return "警告";
        case AlertLevel::Critical:  return "重大";
        case AlertLevel::Emergency: return "緊急";
    }
    return "情報";
}

int alertLevelIndex(AlertLevel level) {
    return static_cast<int>(level);
}

AlertEvent AlertTemplate::blackHole(long cycle) {
    AlertEvent e;
    e.cycle = cycle;
    e.level = AlertLevel::Critical;
    e.type = "BlackHole";
    e.title = "緊急速報: ブラックホール発生";
    e.message = "サイクル " + std::to_string(cycle) + ": ブラックホールが発生しました。文明圧縮が開始されます。";
    e.ttsText = "緊急速報です。サイクル" + std::to_string(cycle) + "。ブラックホールが発生しました。文明圧縮が開始されます。";
    return e;
}

AlertEvent AlertTemplate::gravity(long cycle) {
    AlertEvent e;
    e.cycle = cycle;
    e.level = AlertLevel::Critical;
    e.type = "Gravity";
    e.title = "速報: 重力場起動";
    e.message = "サイクル " + std::to_string(cycle) + ": 重力場が起動しました。文明断片の移送が開始されます。";
    e.ttsText = "速報です。グラビティフィールドが起動しました。文明断片の移送が開始されます。";
    return e;
}

AlertEvent AlertTemplate::whiteHole(long cycle) {
    AlertEvent e;
    e.cycle = cycle;
    e.level = AlertLevel::Critical;
    e.type = "WhiteHole";
    e.title = "緊急速報: ホワイトホール再展開";
    e.message = "サイクル " + std::to_string(cycle) + ": ホワイトホールが発生しました。圧縮概念の再展開を開始します。";
    e.ttsText = "緊急速報です。ホワイトホールが発生しました。圧縮された概念の再展開を開始します。";
    return e;
}

AlertEvent AlertTemplate::collapse(long cycle) {
    AlertEvent e;
    e.cycle = cycle;
    e.level = AlertLevel::Critical;
    e.type = "Collapse";
    e.title = "文明崩壊警報";
    e.message = "サイクル " + std::to_string(cycle) + ": 文明崩壊を検出しました。";
    e.ttsText = "文明崩壊警報です。サイクル" + std::to_string(cycle) + "。文明崩壊が検出されました。";
    return e;
}

AlertEvent AlertTemplate::postSingularity(long cycle) {
    AlertEvent e;
    e.cycle = cycle;
    e.level = AlertLevel::Critical;
    e.type = "PostSingularity";
    e.title = "文明特異点到達";
    e.message = "サイクル " + std::to_string(cycle) + ": 文明がポストシンギュラリティに到達しました。";
    e.ttsText = "速報です。文明がポストシンギュラリティ段階に到達しました。";
    return e;
}

AlertEvent AlertTemplate::generic(const Event& event, AlertLevel level,
                                  const std::string& titlePrefix) {
    AlertEvent a;
    a.cycle = event.cycle;
    a.level = level;
    a.type = event.tag.empty() ? event.name : event.tag;
    a.title = titlePrefix + jpDomainName(event.name);
    a.message = "サイクル " + std::to_string(event.cycle) + ": " + event.message;
    a.ttsText = a.message;
    return a;
}

} // namespace aic
