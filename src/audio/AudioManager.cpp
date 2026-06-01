#include "audio/AudioManager.hpp"

#include "util/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>

namespace aic {

namespace {

constexpr int kSampleRate = 44100;

const char* kBgmNames[static_cast<size_t>(BgmState::Count)] = {
    "title", "primitive", "growth", "growth", "anomaly",
    "blackhole", "gravity", "whitehole", "collapse", "post_singularity"
};

const char* kSeNames[static_cast<size_t>(SeId::Count)] = {
    "concept_created", "law_created", "religion_created", "agent_born",
    "agent_dead", "blackhole_start", "gravity_start", "whitehole_start",
    "collapse", "warning"
};

// Distinct base frequency per BGM state so fallback tones are differentiable.
const float kBgmFreq[static_cast<size_t>(BgmState::Count)] = {
    220.0f, 110.0f, 165.0f, 330.0f, 138.0f,
    70.0f, 90.0f, 440.0f, 100.0f, 523.0f
};

const float kSeFreq[static_cast<size_t>(SeId::Count)] = {
    660.0f, 550.0f, 740.0f, 880.0f, 200.0f,
    80.0f, 120.0f, 990.0f, 90.0f, 440.0f
};

// Synthesizes a mono 16-bit PCM wave. When seBlip is true a short percussive
// blip with exponential decay is produced; otherwise a softly modulated
// ambient pad suitable for looping BGM.
Wave synthWave(float seconds, float baseFreq, float amp, bool seBlip) {
    const int frames = std::max(1, static_cast<int>(seconds * kSampleRate));
    auto* data = static_cast<int16_t*>(std::malloc(sizeof(int16_t) * frames));

    for (int i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        float s;
        if (seBlip) {
            const float env = std::exp(-t * 18.0f);
            s = amp * env * std::sin(2.0f * PI * baseFreq * t);
        } else {
            const float tremolo = 0.85f + 0.15f * std::sin(2.0f * PI * 0.5f * t);
            const float body = std::sin(2.0f * PI * baseFreq * t) +
                               0.30f * std::sin(2.0f * PI * 2.0f * baseFreq * t);
            // Short fades at both ends to avoid loop click.
            float edge = 1.0f;
            const float fade = 0.05f;
            if (t < fade) edge = t / fade;
            else if (t > seconds - fade) edge = std::max(0.0f, (seconds - t) / fade);
            s = amp * tremolo * 0.5f * body * edge;
        }
        s = std::clamp(s, -1.0f, 1.0f);
        data[i] = static_cast<int16_t>(s * 32767.0f);
    }

    Wave w{};
    w.frameCount = static_cast<unsigned int>(frames);
    w.sampleRate = static_cast<unsigned int>(kSampleRate);
    w.sampleSize = 16;
    w.channels = 1;
    w.data = data;
    return w;
}

} // namespace

bool AudioManager::init(const AudioSettings& settings) {
    bgmVolume_ = settings.bgmVolume;
    seVolume_ = settings.seVolume;
    muted_ = settings.mute;
    duckEnabled_ = settings.duckDuringTts;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        Logger::warn("Audio device unavailable; continuing without sound");
        ready_ = false;
        return false;
    }
    ready_ = true;
    Logger::info("Audio device initialized");

    loadAllBgm();
    loadAllSe();
    return true;
}

void AudioManager::loadAllBgm() {
    std::error_code ec;
    std::filesystem::create_directories("cache/fallback", ec);

    for (size_t i = 0; i < bgm_.size(); ++i) {
        const std::string file = std::string("assets/audio/bgm_") +
                                 kBgmNames[i] + ".ogg";
        Music m{};
        bool loaded = false;
        if (FileExists(file.c_str())) {
            m = LoadMusicStream(file.c_str());
            loaded = (m.frameCount > 0);
            if (!loaded) {
                Logger::warn(std::string("BGM load failed: ") + file);
            }
        }
        if (!loaded) {
            const std::string fb = std::string("cache/fallback/bgm_") +
                                   kBgmNames[i] + ".wav";
            Wave w = synthWave(4.0f, kBgmFreq[i], 0.6f, /*seBlip=*/false);
            ExportWave(w, fb.c_str());
            UnloadWave(w);
            m = LoadMusicStream(fb.c_str());
            Logger::warn(std::string("BGM missing, using fallback tone: ") + file);
        }
        m.looping = true;
        bgm_[i] = m;
        bgmValid_[i] = (m.frameCount > 0);
    }
}

void AudioManager::loadAllSe() {
    for (size_t i = 0; i < se_.size(); ++i) {
        const std::string file = std::string("assets/sfx/") +
                                 kSeNames[i] + ".wav";
        Sound s{};
        if (FileExists(file.c_str())) {
            s = LoadSound(file.c_str());
        } else {
            Wave w = synthWave(0.25f, kSeFreq[i], 0.7f, /*seBlip=*/true);
            s = LoadSoundFromWave(w);
            UnloadWave(w);
            Logger::warn(std::string("SE missing, using fallback blip: ") + file);
        }
        se_[i] = s;
        seValid_[i] = (s.frameCount > 0);
    }
}

void AudioManager::shutdown() {
    if (!ready_) {
        return;
    }
    if (ttsValid_) {
        StopSound(ttsSound_);
        UnloadSound(ttsSound_);
        ttsValid_ = false;
        ttsPlaying_ = false;
    }
    for (size_t i = 0; i < bgm_.size(); ++i) {
        if (bgmValid_[i]) UnloadMusicStream(bgm_[i]);
    }
    for (size_t i = 0; i < se_.size(); ++i) {
        if (seValid_[i]) UnloadSound(se_[i]);
    }
    CloseAudioDevice();
    ready_ = false;
    Logger::info("Audio device closed");
}

float AudioManager::effectiveBgmVolume() const {
    if (muted_) return 0.0f;
    return std::clamp(bgmVolume_, 0.0f, 1.0f) * duckCurrent_;
}

void AudioManager::update(float dt) {
    if (!ready_) {
        return;
    }

    // Smoothly approach the ducking target.
    if (duckCurrent_ != duckTarget_) {
        const float step = dt * 6.0f;
        if (duckCurrent_ < duckTarget_)
            duckCurrent_ = std::min(duckTarget_, duckCurrent_ + step);
        else
            duckCurrent_ = std::max(duckTarget_, duckCurrent_ - step);
    }

    const size_t cur = static_cast<size_t>(current_);
    const float base = effectiveBgmVolume();

    if (current_ != BgmState::Count && bgmValid_[cur]) {
        UpdateMusicStream(bgm_[cur]);
    }

    if (crossfading_) {
        fadeTimer_ += dt;
        const float f = std::clamp(fadeTimer_ / fadeDuration_, 0.0f, 1.0f);
        if (current_ != BgmState::Count && bgmValid_[cur]) {
            SetMusicVolume(bgm_[cur], base * f);
        }
        if (previous_ != BgmState::Count) {
            const size_t prev = static_cast<size_t>(previous_);
            if (bgmValid_[prev]) {
                UpdateMusicStream(bgm_[prev]);
                SetMusicVolume(bgm_[prev], base * (1.0f - f));
            }
        }
        if (f >= 1.0f) {
            if (previous_ != BgmState::Count) {
                const size_t prev = static_cast<size_t>(previous_);
                if (bgmValid_[prev]) StopMusicStream(bgm_[prev]);
            }
            previous_ = BgmState::Count;
            crossfading_ = false;
        }
    } else if (current_ != BgmState::Count && bgmValid_[cur]) {
        SetMusicVolume(bgm_[cur], base);
    }

    if (ttsPlaying_ && ttsValid_ && !IsSoundPlaying(ttsSound_)) {
        UnloadSound(ttsSound_);
        ttsValid_ = false;
        ttsPlaying_ = false;
        setDucking(false);
    }
}

void AudioManager::playBgm(BgmState state) {
    if (state == BgmState::Count) {
        return;
    }
    if (state == current_) {
        return;
    }

    Logger::info(std::string("BGM -> ") +
                 kBgmNames[static_cast<size_t>(state)]);

    if (!ready_) {
        current_ = state;   // remember intent even when silent
        return;
    }

    // Finish any in-flight fade immediately to avoid stacking streams.
    if (crossfading_ && previous_ != BgmState::Count) {
        const size_t prev = static_cast<size_t>(previous_);
        if (bgmValid_[prev]) StopMusicStream(bgm_[prev]);
    }

    previous_ = current_;
    current_ = state;
    crossfading_ = (previous_ != BgmState::Count);
    fadeTimer_ = 0.0f;

    const size_t cur = static_cast<size_t>(current_);
    if (bgmValid_[cur]) {
        PlayMusicStream(bgm_[cur]);
        SetMusicVolume(bgm_[cur], crossfading_ ? 0.0f : effectiveBgmVolume());
    }
}

void AudioManager::playSe(SeId id) {
    if (!ready_ || id == SeId::Count) {
        return;
    }
    const size_t i = static_cast<size_t>(id);
    if (!seValid_[i]) {
        return;
    }
    SetSoundVolume(se_[i], muted_ ? 0.0f : std::clamp(seVolume_, 0.0f, 1.0f));
    PlaySound(se_[i]);
}

bool AudioManager::playVoiceFile(const std::string& wavPath, float volume) {
    if (!ready_) return false;
    if (!FileExists(wavPath.c_str())) return false;
    if (ttsValid_) {
        StopSound(ttsSound_);
        UnloadSound(ttsSound_);
        ttsValid_ = false;
        ttsPlaying_ = false;
    }
    Sound s = LoadSound(wavPath.c_str());
    if (s.frameCount == 0) {
        Logger::warn("TTS WAV load failed: " + wavPath);
        return false;
    }
    ttsSound_ = s;
    ttsValid_ = true;
    ttsPlaying_ = true;
    SetSoundVolume(ttsSound_, muted_ ? 0.0f : std::clamp(volume, 0.0f, 1.0f));
    setDucking(true);
    PlaySound(ttsSound_);
    return true;
}

void AudioManager::setBgmVolume(float v) {
    bgmVolume_ = std::clamp(v, 0.0f, 1.0f);
}

void AudioManager::setSeVolume(float v) {
    seVolume_ = std::clamp(v, 0.0f, 1.0f);
}

void AudioManager::setMuted(bool m) {
    if (muted_ == m) {
        return;
    }
    muted_ = m;
    Logger::info(std::string("Audio mute: ") + (m ? "ON" : "OFF"));
}

void AudioManager::setDucking(bool on) {
    duckTarget_ = (on && duckEnabled_) ? 0.35f : 1.0f;
}

} // namespace aic
