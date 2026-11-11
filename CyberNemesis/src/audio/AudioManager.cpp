#include "audio/AudioManager.h"
#include <cmath>
#include <cstdlib>

namespace {
    constexpr int   SR  = 22050;
    constexpr float TAU = 6.28318530718f;

    float Noise()                 { return (float)GetRandomValue(-1000, 1000) / 1000.f; }
    float Env(float t, float k)   { return std::exp(-t * k); }
    float Square(float ph)        { return std::sin(ph) >= 0.f ? 1.f : -1.f; }
    float Saw(float ph)           { float x = ph / TAU; return 2.f * (x - std::floor(x)) - 1.f; }
}

Sound AudioManager::Synth(float seconds, const std::function<float(float, float)>& fn) {
    const unsigned frames = (unsigned)(seconds * SR);
    short* data = (short*)std::malloc(frames * sizeof(short));
    for (unsigned i = 0; i < frames; ++i) {
        float s = fn((float)i / (float)SR, seconds);
        if (s > 1.f) s = 1.f; else if (s < -1.f) s = -1.f;
        data[i] = (short)(s * 30000.f);
    }
    Wave w{};
    w.frameCount = frames;
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels   = 1;
    w.data       = data;
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

bool AudioManager::Init() {
    InitAudioDevice();
    ready_ = IsAudioDeviceReady();
    if (!ready_) return false;

    sfx_[BossShot] = Synth(0.28f, [ph = 0.f](float t, float) mutable {
        float f = 900.f * std::exp(-t * 7.f) + 110.f;
        ph += TAU * f / SR;
        return (0.5f * Square(ph) + 0.4f * std::sin(ph * 0.5f)) * Env(t, 6.f);
    });
    sfx_[HeroShot] = Synth(0.14f, [ph = 0.f](float t, float) mutable {
        float f = 1500.f * std::exp(-t * 18.f) + 500.f;
        ph += TAU * f / SR;
        return (0.35f * std::sin(ph) + 0.15f * Square(ph)) * Env(t, 22.f);
    });
    sfx_[Slam] = Synth(0.7f, [ph = 0.f](float t, float) mutable {
        float f = 70.f * std::exp(-t * 3.f) + 28.f;
        ph += TAU * f / SR;
        return 0.9f * std::sin(ph) * Env(t, 4.f) + 0.6f * Noise() * Env(t, 14.f);
    });
    sfx_[Hit] = Synth(0.2f, [](float t, float) {
        return 0.6f * Noise() * Env(t, 22.f) + 0.4f * std::sin(TAU * 180.f * t) * Env(t, 18.f);
    });
    sfx_[ShieldHit] = Synth(0.35f, [](float t, float) {
        return 0.4f * std::sin(TAU * 880.f * t) * Env(t, 8.f) * (0.6f + 0.4f * std::sin(TAU * 30.f * t))
             + 0.25f * std::sin(TAU * 1320.f * t) * Env(t, 10.f);
    });
    sfx_[Dash] = Synth(0.25f, [ph = 0.f](float t, float d) mutable {
        float k = t / d;
        ph += TAU * (300.f + 1400.f * k) / SR;
        return (0.3f * std::sin(ph) + 0.25f * Noise()) * (1.f - k);
    });
    sfx_[Jump] = Synth(0.15f, [ph = 0.f](float t, float d) mutable {
        float k = t / d;
        ph += TAU * (250.f + 700.f * k) / SR;
        return 0.35f * std::sin(ph) * (1.f - k);
    });
    sfx_[HeroDeath] = Synth(1.1f, [ph = 0.f](float t, float) mutable {
        float f = 700.f * std::exp(-t * 3.5f) + 40.f;
        ph += TAU * f / SR;
        return 0.4f * Saw(ph) * Env(t, 2.5f) + 0.5f * Noise() * Env(t, 5.f);
    });
    sfx_[HeroSpawn] = Synth(0.6f, [ph = 0.f](float t, float) mutable {
        static const float notes[5] = { 440.f, 554.f, 659.f, 880.f, 1108.f };
        int idx = (int)(t / 0.12f); if (idx > 4) idx = 4;
        ph += TAU * notes[idx] / SR;
        return 0.3f * Square(ph) * Env(std::fmod(t, 0.12f), 12.f);
    });
    sfx_[BossDeath] = Synth(1.6f, [ph = 0.f](float t, float) mutable {
        ph += TAU * 55.f / SR;
        float rumble = 0.5f + 0.5f * std::sin(TAU * 18.f * t);
        return 0.6f * std::sin(ph) * Env(t, 1.5f) + 0.6f * Noise() * Env(t, 2.2f) * rumble;
    });
    sfx_[Victory] = Synth(1.4f, [ph = 0.f](float t, float) mutable {
        static const float notes[7] = { 523.f, 659.f, 784.f, 1046.f, 784.f, 1046.f, 1319.f };
        int idx = (int)(t / 0.18f); if (idx > 6) idx = 6;
        ph += TAU * notes[idx] / SR;
        return 0.3f * (0.6f * Square(ph) + 0.4f * std::sin(ph)) * Env(std::fmod(t, 0.18f), 5.f);
    });
    sfx_[Analyze] = Synth(0.6f, [](float t, float d) {
        float gate = std::fmod(t, 0.08f) < 0.04f ? 1.f : 0.f;
        return 0.25f * Square(TAU * 1200.f * t) * gate * (1.f - t / d);
    });
    sfx_[Blip] = Synth(0.07f, [](float t, float d) {
        return 0.3f * Square(TAU * 1000.f * t) * (1.f - t / d);
    });

    sfx_[Thunder] = Synth(1.8f, [lp = 0.f](float t, float) mutable {
        lp += (Noise() - lp) * 0.04f;
        return lp * 5.f * Env(t, 1.4f) * (0.6f + 0.4f * std::sin(TAU * 9.f * t));
    });
    sfx_[Missile] = Synth(0.5f, [ph = 0.f](float t, float d) mutable {
        float k = t / d;
        ph += TAU * (180.f + 500.f * k) / SR;
        return (0.3f * Saw(ph) + 0.35f * Noise() * (1.f - k)) * (1.f - k);
    });
    sfx_[Beam] = Synth(0.8f, [ph = 0.f](float t, float) mutable {
        ph += TAU * (110.f + 1400.f * std::exp(-t * 7.f)) / SR;
        return (0.45f * std::sin(ph) + 0.3f * Square(ph * 0.5f) + 0.25f * Noise()) * Env(t, 3.5f);
    });
    sfx_[Explosion] = Synth(0.6f, [ph = 0.f](float t, float) mutable {
        ph += TAU * (90.f * std::exp(-t * 6.f) + 30.f) / SR;
        return 0.8f * std::sin(ph) * Env(t, 5.f) + 0.7f * Noise() * Env(t, 9.f);
    });

    sfx_[Roar] = Synth(2.0f, [ph = 0.f, lp = 0.f](float t, float) mutable {
        ph += TAU * (95.f * std::exp(-t * 0.9f) + 38.f) / SR;
        lp += (Noise() - lp) * 0.08f;
        return 0.5f * Saw(ph) * Env(t, 1.1f) + 0.7f * lp * 2.f * Env(t, 0.9f) * (0.6f + 0.4f * std::sin(TAU * 22.f * t));
    });
    sfx_[Charge] = Synth(0.95f, [ph = 0.f](float t, float d) mutable {
        const float k = t / d;
        ph += TAU * (180.f + 900.f * k * k) / SR;
        return (0.28f * std::sin(ph) + 0.12f * Square(ph * 0.5f)) * (0.3f + 0.7f * k);
    });
    sfx_[Pew] = Synth(0.07f, [ph = 0.f](float t, float d) mutable {
        const float k = t / d;
        ph += TAU * (900.f - 500.f * k) / SR;
        return 0.22f * std::sin(ph) * (1.f - k);
    });

    music_ = Synth(4.0f, [ph = 0.f, kph = 0.f](float t, float) mutable {
        static const float bass[16] = { 55.f, 55.f, 55.f, 82.4f, 55.f, 55.f, 73.4f, 65.4f,
                                        49.f, 49.f, 49.f, 73.4f, 49.f, 49.f, 65.4f, 55.f };
        const float step = 0.25f;
        int   idx   = (int)(t / step) % 16;
        float local = std::fmod(t, step);
        ph += TAU * bass[idx] / SR;
        float b = (0.35f * Saw(ph) + 0.4f * std::sin(ph)) * Env(local, 4.f);

        float kt = std::fmod(t, 0.5f);
        kph += TAU * (120.f * std::exp(-kt * 30.f) + 40.f) / SR;
        float kick = 0.8f * std::sin(kph) * Env(kt, 12.f);

        float hat = (idx % 2 == 1) ? 0.16f * Noise() * Env(local, 60.f) : 0.f;
        return 0.55f * (b + kick + hat);
    });
    SetSoundVolume(music_, 0.35f);
    return true;
}

void AudioManager::Shutdown() {
    if (!ready_) return;
    for (auto& s : sfx_) UnloadSound(s);
    UnloadSound(music_);
    CloseAudioDevice();
    ready_ = false;
}

void AudioManager::Play(Sfx s, float volume, float pitch) {
    if (!ready_) return;
    SetSoundVolume(sfx_[s], volume);
    SetSoundPitch(sfx_[s], pitch);
    PlaySound(sfx_[s]);
}

void AudioManager::UpdateMusic(float intensity) {
    if (!ready_ || !musicOn_) return;
    SetSoundPitch(music_, 1.f + 0.25f * intensity);
    if (!IsSoundPlaying(music_)) PlaySound(music_);
}

void AudioManager::ToggleMusic() {
    musicOn_ = !musicOn_;
    if (ready_ && !musicOn_) StopSound(music_);
}
