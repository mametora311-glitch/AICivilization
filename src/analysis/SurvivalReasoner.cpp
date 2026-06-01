#include "analysis/SurvivalReasoner.hpp"

#include "util/Localization.hpp"

namespace aic {

std::string SurvivalReasoner::result(const World& world) {
    if (world.experimentBhGvHh && !world.experimentResult.empty())
        return jpExperimentResult(world.experimentResult);
    if (world.civ.population <= 0) return "消滅";
    if (world.civ.stage == Stage::Collapsed) return "崩壊";
    if (world.civ.stage == Stage::Unknown) return "未知";
    if (world.civ.stage == Stage::PostSingularity) return "ポストシンギュラリティ";
    if (world.civ.collapse_risk >= 80.0f) return "不安定";
    return "安定";
}

std::string SurvivalReasoner::collapseRecoverySummary(const World& world) {
    if (world.civ.population <= 0) return "回復が起こる前に全エージェントが消失しました。";
    if (world.civ.stage == Stage::PostSingularity)
        return "文明は異常圧縮を生き延び、ポストシンギュラリティへ再展開しました。";
    if (world.civ.stage == Stage::Collapsed)
        return "文明は回復可能水準を下回る安定度に落ち、崩壊しました。";
    if (world.civ.unknown_score >= 60.0f)
        return "文明は高いUNKNOWN汚染を抱えたまま存続しました。";
    return "文明は終端的崩壊に至らず観測可能な状態を維持しました。";
}

} // namespace aic
