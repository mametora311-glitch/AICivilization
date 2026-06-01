#include "util/JsonUtil.hpp"

#include "util/Logger.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace aic {
namespace jsonutil {

std::optional<Json> readFile(const std::string& path) {
    try {
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) {
            Logger::warn("JSON read failed (cannot open): " + path);
            return std::nullopt;
        }
        std::stringstream ss;
        ss << in.rdbuf();
        Json j = Json::parse(ss.str(), nullptr, /*allow_exceptions=*/true,
                             /*ignore_comments=*/true);
        return j;
    } catch (const std::exception& e) {
        Logger::warn("JSON parse error in " + path + ": " + e.what());
        return std::nullopt;
    } catch (...) {
        Logger::warn("JSON parse error in " + path + " (unknown)");
        return std::nullopt;
    }
}

bool writeFile(const std::string& path, const Json& j, int indent) {
    try {
        const std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            Logger::warn("JSON write failed (cannot open): " + path);
            return false;
        }
        out << j.dump(indent);
        return true;
    } catch (const std::exception& e) {
        Logger::warn("JSON write error for " + path + ": " + e.what());
        return false;
    } catch (...) {
        Logger::warn("JSON write error for " + path + " (unknown)");
        return false;
    }
}

int getInt(const Json& j, const char* key, int fallback) {
    if (j.contains(key) && j[key].is_number()) return j[key].get<int>();
    return fallback;
}

unsigned getUInt(const Json& j, const char* key, unsigned fallback) {
    if (j.contains(key) && j[key].is_number()) {
        const long long v = j[key].get<long long>();
        return v < 0 ? fallback : static_cast<unsigned>(v);
    }
    return fallback;
}

float getFloat(const Json& j, const char* key, float fallback) {
    if (j.contains(key) && j[key].is_number()) return j[key].get<float>();
    return fallback;
}

bool getBool(const Json& j, const char* key, bool fallback) {
    if (j.contains(key) && j[key].is_boolean()) return j[key].get<bool>();
    return fallback;
}

std::string getString(const Json& j, const char* key, const std::string& fallback) {
    if (j.contains(key) && j[key].is_string()) return j[key].get<std::string>();
    return fallback;
}

} // namespace jsonutil
} // namespace aic
