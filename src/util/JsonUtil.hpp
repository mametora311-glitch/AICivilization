#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace aic {

using Json = nlohmann::json;

namespace jsonutil {

// Reads and parses a JSON file. Returns std::nullopt (and logs a warning) on
// any I/O or parse error. Never throws.
std::optional<Json> readFile(const std::string& path);

// Serializes and writes a JSON document, creating parent directories as needed.
// Returns false (and logs a warning) on failure. Never throws.
bool writeFile(const std::string& path, const Json& j, int indent = 2);

// Convenience typed getters that tolerate a missing/mistyped key by returning
// the provided fallback.
int         getInt(const Json& j, const char* key, int fallback);
unsigned    getUInt(const Json& j, const char* key, unsigned fallback);
float       getFloat(const Json& j, const char* key, float fallback);
bool        getBool(const Json& j, const char* key, bool fallback);
std::string getString(const Json& j, const char* key, const std::string& fallback);

} // namespace jsonutil
} // namespace aic
