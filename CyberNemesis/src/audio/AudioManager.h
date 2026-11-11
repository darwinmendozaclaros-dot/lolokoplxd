#pragma once
#include "raylib.h"
#include <functional>

class AudioManager {
public:
    enum Sfx { BossShot, HeroShot, Slam, Hit, ShieldHit, Dash, Jump,
               HeroDeath, HeroSpawn, BossDeath, Victory, Analyze, Blip,
               Thunder, Missile, Beam, Explosion, Roar, Charge, Pew, SFX_COUNT };

    bool Init();
    void Shutdown();
    void Play(Sfx s, float volume = 1.f, float pitch = 1.f);
    void UpdateMusic(float intensity);
    void ToggleMusic();
    bool MusicOn() const { return musicOn_; }

private:
    static Sound Synth(float seconds, const std::function<float(float, float)>& fn);

    bool  ready_   = false;
    bool  musicOn_ = true;
    Sound sfx_[SFX_COUNT]{};
    Sound music_{};
};
