#pragma once

#include "raylib.h"

#include "app/AppState.hpp"

#include <array>
#include <string>

namespace aic {

// BGM tracks, switched by civilization/anomaly state (spec section 6).
enum class BgmState {
    Title,
    Primitive,
    Growth,
    Digital,
    Anomaly,
    BlackHole,
    Gravity,
    WhiteHole,
    Collapse,
    PostSingularity,
    Count
};

// Sound effects fired on simulation events (spec section 4.3 / 9).
enum class SeId {
    ConceptCreated,
    LawCreated,
    ReligionCreated,
    AgentBorn,
    AgentDead,
    BlackHoleStart,
    GravityStart,
    WhiteHoleStart,
    Collapse,
    Warning,
    Count
};

// Owns the raylib audio device, all BGM streams and SE samples.
//
// Robustness contract (spec): the application must never crash because audio
// files or an audio device are missing. If the device cannot be opened the
// manager degrades to a silent no-op. If a BGM/SE file is missing a procedural
// fallback tone is synthesized so the corresponding cue is still audible.
class AudioManager {
public:
    bool init(const AudioSettings& settings);
    void shutdown();

    // Must be called once per frame: pumps music streams and advances the
    // crossfade / ducking ramps.
    void update(float dt);

    void playBgm(BgmState state);   // crossfades from the current track
    void playSe(SeId id);
    bool playVoiceFile(const std::string& wavPath, float volume);

    void setBgmVolume(float v);
    void setSeVolume(float v);
    void setMuted(bool m);
    bool isMuted() const { return muted_; }
    float bgmVolume() const { return bgmVolume_; }
    float seVolume() const { return seVolume_; }

    // TTS ducking: when enabled, BGM is smoothly lowered to 35% and restored.
    void setDucking(bool on);
    void setDuckEnabled(bool on) { duckEnabled_ = on; if (!on) duckTarget_ = 1.0f; }
    bool duckEnabled() const { return duckEnabled_; }

    bool ready() const { return ready_; }
    BgmState currentBgm() const { return current_; }
    bool ttsPlaying() const { return ttsPlaying_; }

private:
    void loadAllBgm();
    void loadAllSe();
    float effectiveBgmVolume() const;

    bool ready_ = false;
    bool muted_ = false;
    bool duckEnabled_ = true;
    float bgmVolume_ = 0.6f;
    float seVolume_ = 0.8f;

    std::array<Music, static_cast<size_t>(BgmState::Count)> bgm_{};
    std::array<bool,  static_cast<size_t>(BgmState::Count)> bgmValid_{};
    std::array<Sound, static_cast<size_t>(SeId::Count)> se_{};
    std::array<bool,  static_cast<size_t>(SeId::Count)> seValid_{};

    BgmState current_ = BgmState::Count;   // Count = none playing
    BgmState previous_ = BgmState::Count;
    bool crossfading_ = false;
    float fadeTimer_ = 0.0f;
    float fadeDuration_ = 1.0f;

    float duckCurrent_ = 1.0f;
    float duckTarget_ = 1.0f;

    Sound ttsSound_{};
    bool ttsValid_ = false;
    bool ttsPlaying_ = false;
};

} // namespace aic
