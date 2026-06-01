#include "analysis/CivilizationAnalyzer.hpp"

#include "analysis/SurvivalReasoner.hpp"

#include "util/Localization.hpp"

#include <sstream>

namespace aic {

std::string CivilizationAnalyzer::finalReport(const Simulation& simulation) {
    const World& w = simulation.world();
    std::ostringstream out;
    out << "AICivilization 最終レポート\n\n";
    out << "シナリオ:\n" << simulation.config().name << "\n\n";
    out << "シード:\n" << w.worldSeed << "\n\n";
    out << "最終サイクル:\n" << w.cycle << "\n\n";
    out << "最終段階:\n" << jpStageName(w.civ.stage) << "\n\n";
    out << "人口:\n" << w.civ.population << "\n\n";
    out << "概念:\n" << w.civ.concept_count << "\n\n";
    out << "法:\n" << w.civ.law_count << "\n\n";
    out << "宗教:\n" << w.civ.religion_count << "\n\n";
    out << "結果:\n" << SurvivalReasoner::result(w) << "\n\n";
    out << "主要イベント:\n";
    int written = 0;
    const int start = (int)w.events.size() > 20 ? (int)w.events.size() - 20 : 0;
    for (int i = start; i < (int)w.events.size(); ++i) {
        const Event& e = w.events[i];
        out << "- サイクル " << e.cycle << ": " << e.message << "\n";
        ++written;
    }
    if (written == 0) out << "- なし\n";
    out << "\n崩壊/回復サマリー:\n";
    out << SurvivalReasoner::collapseRecoverySummary(w) << "\n";
    return out.str();
}

} // namespace aic
