#include "util/CsvWriter.hpp"

#include "util/Logger.hpp"

#include <filesystem>
#include <fstream>

namespace aic {

namespace {

bool ensureParent(const std::string& path) {
    try {
        const std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        return true;
    } catch (...) {
        return false;
    }
}

void writeCells(std::ofstream& out, const std::vector<std::string>& cells) {
    for (size_t i = 0; i < cells.size(); ++i) {
        if (i) out << ',';
        out << CsvWriter::escape(cells[i]);
    }
    out << '\n';
}

} // namespace

bool CsvWriter::writeHeader(const std::string& path, const std::vector<std::string>& columns) {
    ensureParent(path);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        Logger::warn("CSV header write failed: " + path);
        return false;
    }
    writeCells(out, columns);
    return true;
}

bool CsvWriter::appendRow(const std::string& path, const std::vector<std::string>& values) {
    ensureParent(path);
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out.is_open()) {
        Logger::warn("CSV row append failed: " + path);
        return false;
    }
    writeCells(out, values);
    return true;
}

std::string CsvWriter::escape(const std::string& value) {
    bool quote = false;
    for (char c : value) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') {
            quote = true;
            break;
        }
    }
    if (!quote) return value;
    std::string out = "\"";
    for (char c : value) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

} // namespace aic
