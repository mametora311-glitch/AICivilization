#include "util/Localization.hpp"

#include <string_view>

namespace aic {

namespace {
struct Term {
    std::string_view key;
    std::string_view jp;
};

constexpr Term kTerms[] = {
    { "PostSingularity", "ポストシンギュラリティ" },
    { "WhiteHoleProtocol", "ホワイトホールプロトコル" },
    { "DreamMindAI", "夢心AI" },
    { "BlackHoleAbsorption", "ブラックホール吸収" },
    { "ResourceSharing", "資源共有" },
    { "TerritoryRights", "領域権" },
    { "ConflictResolution", "紛争調停" },
    { "ConceptProtection", "概念保護" },
    { "FaithFreedom", "信仰自由" },
    { "WhiteHole", "ホワイトホール" },
    { "BlackHole", "ブラックホール" },
    { "OutsideSignal", "外部信号" },
    { "TimeFracture", "時間断裂" },
    { "MemoryEater", "記憶喰らい" },
    { "NoiseRain", "ノイズ雨" },
    { "GravityField", "重力場" },
    { "GravityLaw", "重力法" },
    { "MemoryOcean", "記憶の海" },
    { "UnknownSeed", "未知の種" },
    { "Reconstruction", "再構築" },
    { "ResourceDepletion", "資源枯渇" },
    { "PopulationDrop", "人口急減" },
    { "CollapseRisk", "崩壊危険度" },
    { "Civilization", "文明" },
    { "Reconstructed", "再構築済み" },
    { "Transferred", "転送済み" },
    { "Agricultural", "農耕" },
    { "Industrial", "産業" },
    { "Primitive", "原始" },
    { "Collapsed", "崩壊" },
    { "Unknown", "未知" },
    { "Digital", "デジタル" },
    { "Tribal", "部族" },
    { "Erased", "消滅" },
    { "Unstable", "不安定" },
    { "Stable", "安定" },
    { "Running", "進行中" },
    { "Agents", "エージェント" },
    { "Agent", "エージェント" },
    { "Concepts", "概念" },
    { "Concept", "概念" },
    { "Laws", "法" },
    { "Law", "法" },
    { "Religions", "宗教" },
    { "Religion", "宗教" },
    { "Structures", "構造物" },
    { "Structure", "構造物" },
    { "World", "世界" },
    { "System", "システム" },
    { "Manual", "手動" },
    { "Scenario", "シナリオ" },
    { "Experiment", "実験" },
    { "Settings", "設定" },
    { "Notification", "通知" },
    { "Inheritance", "継承" },
    { "Cooperation", "協力" },
    { "Migration", "移住" },
    { "Stewardship", "管理責任" },
    { "Assembly", "集会" },
    { "Order", "秩序" },
    { "Trade", "交易" },
    { "Shelter", "住居" },
    { "Granary", "穀倉" },
    { "Temple", "神殿" },
    { "Archive", "記録庫" },
    { "Tower", "塔" },
    { "Workshop", "工房" },
    { "Monument", "記念碑" },
    { "Aqueduct", "水道" },
    { "Beacon", "灯台" },
    { "Hall", "広間" },
    { "Dream", "夢" },
    { "Ocean", "海" },
    { "Rain", "雨" },
    { "Memory", "記憶" },
    { "Emotion", "感情" }
};

constexpr Term kSuffixTerms[] = {
    { "Reverence", "崇敬" },
    { "Gravity", "重力" },
    { "Protocol", "プロトコル" },
    { "Loneliness", "孤独" },
    { "Mind", "心" },
    { "Cult", "信仰団" },
    { "Faith", "信仰" },
    { "AI", "AI" },
    { "Mirror", "鏡" },
    { "Eclipse", "蝕" },
    { "Dream", "夢" },
    { "Ocean", "海" },
    { "Memory", "記憶" },
    { "Emotion", "感情" }
};

std::string lookupExact(const std::string& name) {
    for (const Term& t : kTerms) {
        if (name == t.key) return std::string(t.jp);
    }
    for (const Term& t : kSuffixTerms) {
        if (name == t.key) return std::string(t.jp);
    }
    return {};
}

std::string abbreviateUtf8(const std::string& text) {
    constexpr size_t kMaxChars = 24;
    size_t pos = 0;
    size_t chars = 0;
    while (pos < text.size() && chars < kMaxChars) {
        const unsigned char c = static_cast<unsigned char>(text[pos]);
        size_t step = 1;
        if ((c & 0xe0) == 0xc0) step = 2;
        else if ((c & 0xf0) == 0xe0) step = 3;
        else if ((c & 0xf8) == 0xf0) step = 4;
        if (pos + step > text.size()) break;
        pos += step;
        ++chars;
    }
    if (pos >= text.size()) return text;
    return text.substr(0, pos) + "...";
}

} // namespace

const char* jpStageName(Stage stage) {
    switch (stage) {
        case Stage::Primitive:       return "原始";
        case Stage::Tribal:          return "部族";
        case Stage::Agricultural:    return "農耕";
        case Stage::Industrial:      return "産業";
        case Stage::Digital:         return "デジタル";
        case Stage::PostSingularity: return "ポストシンギュラリティ";
        case Stage::Collapsed:       return "崩壊";
        case Stage::Unknown:         return "未知";
    }
    return "原始";
}

const char* jpAgentStateName(AgentState state) {
    switch (state) {
        case AgentState::Alive:       return "生存";
        case AgentState::Hungry:      return "飢餓";
        case AgentState::Mutating:    return "変異";
        case AgentState::Reproducing: return "繁殖";
        case AgentState::Believing:   return "信仰";
        case AgentState::Collapsed:   return "崩壊";
        case AgentState::Dead:        return "死亡";
        case AgentState::Unknown:     return "未知";
    }
    return "生存";
}

const char* jpEventTypeName(EventType type) {
    switch (type) {
        case EventType::Agent:        return "エージェント";
        case EventType::Concept:      return "概念";
        case EventType::Law:          return "法";
        case EventType::Religion:     return "宗教";
        case EventType::Structure:    return "構造物";
        case EventType::Anomaly:      return "異常";
        case EventType::Civilization: return "文明";
        case EventType::System:       return "システム";
    }
    return "システム";
}

const char* jpAnomalyTypeName(AnomalyType type) {
    switch (type) {
        case AnomalyType::BlackHole:     return "ブラックホール";
        case AnomalyType::Gravity:       return "重力";
        case AnomalyType::WhiteHole:     return "ホワイトホール";
        case AnomalyType::OutsideSignal: return "外部信号";
        case AnomalyType::TimeFracture:  return "時間断裂";
        case AnomalyType::MemoryEater:   return "記憶喰らい";
        case AnomalyType::Eclipse:       return "蝕";
        case AnomalyType::Mirror:        return "鏡";
        case AnomalyType::NoiseRain:     return "ノイズ雨";
    }
    return "外部信号";
}

std::string jpDomainName(const std::string& name) {
    if (name.empty()) return "";

    if (auto exact = lookupExact(name); !exact.empty()) return abbreviateUtf8(exact);

    std::string out;
    size_t pos = 0;
    while (pos < name.size()) {
        bool matched = false;
        for (const Term& t : kTerms) {
            if (name.compare(pos, t.key.size(), t.key) == 0) {
                out += t.jp;
                pos += t.key.size();
                matched = true;
                break;
            }
        }
        if (matched) continue;
        for (const Term& t : kSuffixTerms) {
            if (name.compare(pos, t.key.size(), t.key) == 0) {
                out += t.jp;
                pos += t.key.size();
                matched = true;
                break;
            }
        }
        if (!matched) return abbreviateUtf8(name);
    }
    return abbreviateUtf8(out.empty() ? name : out);
}

std::string jpExperimentResult(const std::string& result) {
    return jpDomainName(result);
}

} // namespace aic
