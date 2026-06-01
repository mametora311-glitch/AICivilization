#include "notify/NotificationManager.hpp"

#include "audio/AudioManager.hpp"
#include "gui/GuiText.hpp"
#include "notify/AlertRouter.hpp"
#include "raylib.h"
#include "util/CsvWriter.hpp"
#include "util/Logger.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

#define DrawText aic::drawText

namespace aic {

namespace {

constexpr char kAlertLog[] = "logs/alert_log.csv";

std::string boolText(bool v) { return v ? "true" : "false"; }

std::string nowStamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()).count() % 1000;
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S") << '_' << std::setw(3)
       << std::setfill('0') << ms;
    return ss.str();
}

Color levelColor(AlertLevel level) {
    switch (level) {
        case AlertLevel::Info:      return Color{ 90, 160, 230, 240 };
        case AlertLevel::Notice:    return Color{ 110, 210, 150, 240 };
        case AlertLevel::Warning:   return Color{ 230, 180, 80, 245 };
        case AlertLevel::Critical:  return Color{ 230, 95, 80, 250 };
        case AlertLevel::Emergency: return Color{ 210, 45, 95, 255 };
    }
    return RAYWHITE;
}

std::string truncateUtf8(const std::string& text, size_t maxChars) {
    size_t pos = 0;
    size_t chars = 0;
    while (pos < text.size() && chars < maxChars) {
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

std::vector<std::string> wrapText(const std::string& text, int width, int fontSize,
                                  int maxLines) {
    std::vector<std::string> lines;
    std::string line;
    std::string token;
    auto flushToken = [&]() {
        if (token.empty()) return;
        std::string test = line.empty() ? token : line + token;
        if (!line.empty() && measureText(test.c_str(), fontSize) > width) {
            lines.push_back(line);
            line.clear();
            if ((int)lines.size() >= maxLines) return;
        }
        line += token;
        token.clear();
    };

    for (size_t i = 0; i < text.size();) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        size_t step = 1;
        if ((c & 0xe0) == 0xc0) step = 2;
        else if ((c & 0xf0) == 0xe0) step = 3;
        else if ((c & 0xf8) == 0xf0) step = 4;
        if (i + step > text.size()) break;
        token = text.substr(i, step);
        flushToken();
        if ((int)lines.size() >= maxLines) break;
        i += step;
    }
    if ((int)lines.size() < maxLines && !line.empty()) lines.push_back(line);
    if ((int)lines.size() > maxLines) lines.resize((size_t)maxLines);
    if (!lines.empty() && measureText(lines.back().c_str(), fontSize) > width) {
        lines.back() = truncateUtf8(lines.back(), 30);
    }
    return lines;
}

} // namespace

bool NotificationManager::init(const NotificationSettings& settings) {
    settings_ = settings;
    CsvWriter::writeHeader(kAlertLog, {
        "cycle", "level", "type", "title", "message", "windows", "tts", "result"
    });
    windowsReady_ = windows_.init();
    return true;
}

void NotificationManager::shutdown() {
    windows_.shutdown();
    windowsReady_ = false;
}

void NotificationManager::applySettings(const NotificationSettings& settings) {
    settings_ = settings;
}

bool NotificationManager::cooldownAllows(const AlertEvent& alert) {
    if (alert.cooldownSeconds < 0) return true;
    const double now = GetTime();
    const int cooldown = alert.cooldownSeconds > 0 ? alert.cooldownSeconds
                                                   : settings_.cooldownSeconds;
    auto it = lastByType_.find(alert.type);
    if (it != lastByType_.end() && now - it->second < cooldown) {
        return false;
    }
    lastByType_[alert.type] = now;
    return true;
}

bool NotificationManager::ttsCooldownAllows(const AlertEvent& alert) {
    if (alert.cooldownSeconds < 0 ||
        alert.level == AlertLevel::Critical ||
        alert.level == AlertLevel::Emergency) {
        return true;
    }
    const double now = GetTime();
    const int cooldown = alert.cooldownSeconds > 0 ? alert.cooldownSeconds
                                                   : settings_.cooldownSeconds;
    auto it = lastTtsByType_.find(alert.type);
    if (it != lastTtsByType_.end() && now - it->second < cooldown) {
        return false;
    }
    lastTtsByType_[alert.type] = now;
    return true;
}

void NotificationManager::addInApp(const AlertEvent& alert) {
    inApp_.push_back({ alert, 0.0f, 6.0f + alertLevelIndex(alert.level) });
    while (inApp_.size() > 10) {
        auto it = std::min_element(inApp_.begin(), inApp_.end(),
            [](const InAppAlert& a, const InAppAlert& b) {
                return alertLevelIndex(a.alert.level) < alertLevelIndex(b.alert.level);
            });
        if (it != inApp_.end()) inApp_.erase(it);
        else break;
    }
}

std::string NotificationManager::makeTtsCachePath() const {
    return "cache/tts/tts_" + nowStamp() + ".wav";
}

void NotificationManager::writeAlertLog(const AlertEvent& alert,
                                        const std::string& windowsResult,
                                        const std::string& ttsResult) {
    const bool ttsOk = ttsResult == "played" || ttsResult == "queued" ||
                       ttsResult == "played_or_queued";
    CsvWriter::appendRow(kAlertLog, {
        std::to_string(alert.cycle),
        alertLevelName(alert.level),
        alert.type,
        alert.title,
        alert.message,
        boolText(windowsResult == "sent"),
        boolText(ttsOk),
        "windows=" + windowsResult + ";tts=" + ttsResult
    });
}

bool NotificationManager::queueTts(const AlertEvent& alert) {
    const std::string path = makeTtsCachePath();
    std::string result;
    if (voicevox_.synthesizeToFile(settings_.voicevoxEndpoint, settings_.speakerId,
                                   alert.ttsText, path, result)) {
        ttsQueue_.push_back({ path, settings_.ttsVolume });
        while (ttsQueue_.size() > 8) ttsQueue_.pop_front();
        return true;
    }
    Logger::warn(result.empty() ? "VOICEVOX synthesis failed" : result);
    return false;
}

void NotificationManager::playQueuedTts(AudioManager& audio) {
    if (ttsQueue_.empty() || audio.ttsPlaying()) return;
    TtsQueueItem item = ttsQueue_.front();
    ttsQueue_.pop_front();
    if (!audio.playVoiceFile(item.path, item.volume)) {
        Logger::warn("TTS queued WAV playback failed: " + item.path);
    }
}

void NotificationManager::processEvent(const Event& event, AudioManager& audio) {
    auto alert = AlertRouter::route(event);
    if (alert) pushAlert(*alert, audio);
}

void NotificationManager::pushAlert(const AlertEvent& original, AudioManager& audio) {
    AlertEvent alert = original;
    alert.showInApp = alert.showInApp && settings_.inApp;
    alert.sendWindowsNotification = alert.sendWindowsNotification && settings_.windows;
    alert.speakByVoicevox = alert.speakByVoicevox && settings_.tts &&
                             alertLevelIndex(alert.level) >= settings_.minLevel;
    if (alert.cooldownSeconds <= 0) alert.cooldownSeconds = settings_.cooldownSeconds;

    const bool presentationAllowed = cooldownAllows(alert);
    const bool ttsAllowed = alert.speakByVoicevox && ttsCooldownAllows(alert);

    std::string windowsResult = "disabled";
    std::string ttsResult = "disabled";
    if (!presentationAllowed) {
        windowsResult = alert.sendWindowsNotification ? "cooldown" : "disabled";
    } else if (alert.showInApp) {
        addInApp(alert);
    }

    if (presentationAllowed && alert.sendWindowsNotification) {
        if (windowsReady_ && windows_.notify(alert.title, alert.message)) {
            windowsResult = "sent";
        } else {
            windowsResult = "failed";
        }
    }

    if (alert.speakByVoicevox) {
        if (ttsAllowed) {
            if (queueTts(alert)) {
                playQueuedTts(audio);
                ttsResult = audio.ttsPlaying() ? "played_or_queued" : "queued";
            } else {
                ttsResult = "synthesis_failed";
            }
        } else {
            ttsResult = "cooldown";
        }
    }

    writeAlertLog(alert, windowsResult, ttsResult);
}

void NotificationManager::update(float dt, AudioManager& audio) {
    for (auto& a : inApp_) a.age += dt;
    inApp_.erase(std::remove_if(inApp_.begin(), inApp_.end(),
        [](const InAppAlert& a) { return a.age >= a.lifetime; }), inApp_.end());
    playQueuedTts(audio);
}

void NotificationManager::drawInApp() const {
    const float W = (float)GetScreenWidth();
    float y = 16.0f;
    int index = 0;
    for (const auto& item : inApp_) {
        const AlertEvent& a = item.alert;
        const float alpha = std::clamp((item.lifetime - item.age) / 0.8f, 0.0f, 1.0f);
        Color lc = levelColor(a.level);
        lc.a = (unsigned char)(lc.a * alpha);
        Color bg{ 12, 15, 24, (unsigned char)(235 * alpha) };
        Rectangle r{ W - 540.0f, y + index * 94.0f, 520.0f, 84.0f };
        DrawRectangleRec(r, bg);
        DrawRectangleLinesEx(r, 2.0f, lc);
        BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
        DrawText(truncateUtf8(a.title, 34).c_str(), (int)r.x + 14, (int)r.y + 10, 18, lc);
        const auto lines = wrapText(a.message, (int)r.width - 28, 15, 2);
        int ly = (int)r.y + 39;
        for (const auto& line : lines) {
            DrawText(line.c_str(), (int)r.x + 14, ly, 15,
                     Color{ 238, 242, 252, (unsigned char)(240 * alpha) });
            ly += 19;
        }
        EndScissorMode();
        ++index;
    }
}

void NotificationManager::testWindows(AudioManager& audio) {
    AlertEvent a = AlertRouter::testAlert("Windows");
    a.speakByVoicevox = false;
    a.showInApp = false;
    a.sendWindowsNotification = true;
    a.cooldownSeconds = -1;
    pushAlert(a, audio);
}

void NotificationManager::testVoicevox(AudioManager& audio) {
    AlertEvent a = AlertRouter::testAlert("VoiceVox");
    a.showInApp = false;
    a.sendWindowsNotification = false;
    a.speakByVoicevox = true;
    a.cooldownSeconds = -1;
    pushAlert(a, audio);
}

void NotificationManager::testInApp(AudioManager& audio) {
    AlertEvent a = AlertRouter::testAlert("InApp");
    a.showInApp = true;
    a.sendWindowsNotification = false;
    a.speakByVoicevox = false;
    a.cooldownSeconds = -1;
    pushAlert(a, audio);
}

} // namespace aic
