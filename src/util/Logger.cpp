#include "util/Logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

namespace aic {

namespace {

std::mutex g_mutex;
std::ofstream g_file;
bool g_fileOpen = false;

const char* levelTag(LogLevel level) {
    switch (level) {
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

// Returns "YYYY-MM-DD HH:MM:SS" for the current local time.
std::string timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

} // namespace

bool Logger::init(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_file.is_open()) {
        g_file.flush();
        g_file.close();
    }
    g_fileOpen = false;

    try {
        const std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            // ec is intentionally ignored: if the directory cannot be created
            // the subsequent open will fail and we fall back to stdout-only.
        }
        g_file.open(filePath, std::ios::out | std::ios::app);
        g_fileOpen = g_file.is_open();
    } catch (...) {
        g_fileOpen = false;
    }

    if (g_fileOpen) {
        g_file << "==== AICivilization session start " << timestamp()
               << " ====\n";
        g_file.flush();
    }

    return g_fileOpen;
}

void Logger::log(LogLevel level, std::string_view message) {
    std::lock_guard<std::mutex> lock(g_mutex);

    std::string line;
    line.reserve(message.size() + 40);
    line.append("[").append(timestamp()).append("][");
    line.append(levelTag(level)).append("] ");
    line.append(message);

    std::cout << line << '\n';
    std::cout.flush();

    if (g_fileOpen) {
        g_file << line << '\n';
        g_file.flush();
    }
}

void Logger::info(std::string_view message)  { log(LogLevel::Info, message); }
void Logger::warn(std::string_view message)  { log(LogLevel::Warn, message); }
void Logger::error(std::string_view message) { log(LogLevel::Error, message); }

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file.is_open()) {
        g_file << "==== AICivilization session end " << timestamp()
               << " ====\n";
        g_file.flush();
        g_file.close();
    }
    g_fileOpen = false;
}

} // namespace aic
