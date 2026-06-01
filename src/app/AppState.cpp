#include "app/AppState.hpp"

#include "util/Logger.hpp"

#include <string>

namespace aic {

const char* screenName(Screen s) {
    switch (s) {
        case Screen::Title:          return "タイトル";
        case Screen::ScenarioSelect: return "シナリオ選択";
        case Screen::Main:           return "観測";
        case Screen::Agents:         return "エージェント";
        case Screen::Concepts:       return "概念";
        case Screen::Civilization:   return "文明";
        case Screen::AnomalyLab:     return "異常ラボ";
        case Screen::Timeline:       return "タイムライン";
        case Screen::Logs:           return "ログ";
        case Screen::Settings:       return "設定";
    }
    return "不明";
}

void AppState::setScreen(Screen s) {
    if (s == screen) {
        return;
    }
    Logger::info(std::string("Screen: ") + screenName(screen) + " -> " +
                 screenName(s));
    screen = s;
}

} // namespace aic
