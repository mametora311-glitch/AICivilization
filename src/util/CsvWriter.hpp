#pragma once

#include <string>
#include <vector>

namespace aic {

class CsvWriter {
public:
    static bool writeHeader(const std::string& path, const std::vector<std::string>& columns);
    static bool appendRow(const std::string& path, const std::vector<std::string>& values);
    static std::string escape(const std::string& value);
};

} // namespace aic
