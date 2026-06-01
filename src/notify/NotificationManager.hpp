#pragma once

#include "app/AppState.hpp"
#include "notify/AlertTemplate.hpp"
#include "notify/VoicevoxClient.hpp"
#include "notify/WindowsNotifier.hpp"

#include <deque>
#include <string>
#include <unordered_map>

namespace aic {

class AudioManager;
struct Event;

class NotificationManager {
public:
    bool init(const NotificationSettings& settings);
    void shutdown();
    void applySettings(const NotificationSettings& settings);
    void processEvent(const Event& event, AudioManager& audio);
    void pushAlert(const AlertEvent& alert, AudioManager& audio);
    void update(float dt, AudioManager& audio);
    void drawInApp() const;
    void testWindows(AudioManager& audio);
    void testVoicevox(AudioManager& audio);
    void testInApp(AudioManager& audio);

private:
    struct InAppAlert {
        AlertEvent alert;
        float age = 0.0f;
        float lifetime = 6.0f;
    };

    struct TtsQueueItem {
        std::string path;
        float volume = 1.0f;
    };

    bool cooldownAllows(const AlertEvent& alert);
    bool ttsCooldownAllows(const AlertEvent& alert);
    void addInApp(const AlertEvent& alert);
    bool queueTts(const AlertEvent& alert);
    void playQueuedTts(AudioManager& audio);
    std::string makeTtsCachePath() const;
    void writeAlertLog(const AlertEvent& alert, const std::string& windowsResult,
                       const std::string& ttsResult);

    NotificationSettings settings_;
    WindowsNotifier windows_;
    VoicevoxClient voicevox_;
    std::deque<InAppAlert> inApp_;
    std::deque<TtsQueueItem> ttsQueue_;
    std::unordered_map<std::string, double> lastByType_;
    std::unordered_map<std::string, double> lastTtsByType_;
    bool windowsReady_ = false;
};

} // namespace aic
