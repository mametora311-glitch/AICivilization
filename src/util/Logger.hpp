#pragma once

#include <string>
#include <string_view>

namespace aic {

enum class LogLevel {
    Info,
    Warn,
    Error
};

// Append-only application logger.
//
// Writes timestamped lines to a log file (default logs/app.log, resolved
// relative to the process working directory) and mirrors every line to stdout.
// All operations are best-effort and never throw: if the log file cannot be
// opened the logger silently degrades to stdout-only so the application keeps
// running even when the logs/ directory is unavailable.
class Logger {
public:
    // Opens (creating parent directories as needed) and switches logging to the
    // given file. Writes a session banner on success. Returns false if the file
    // could not be opened, in which case logging continues to stdout only.
    static bool init(const std::string& filePath = "logs/app.log");

    static void log(LogLevel level, std::string_view message);
    static void info(std::string_view message);
    static void warn(std::string_view message);
    static void error(std::string_view message);

    // Flushes and closes the underlying file. Safe to call multiple times.
    static void shutdown();
};

} // namespace aic
