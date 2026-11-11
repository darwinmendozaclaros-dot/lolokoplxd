#pragma once
#include "raylib.h"
#include "core/Config.h"
#include <vector>

class Hero;
class Boss;
class Hazards;
class BulletField;

class FinalDirector {
public:
    enum class Pattern { None, Ring, Spiral, Rain, Fan, Sine, Blasters, Walls, Waves, Bounce, Pillars };

    // cycle = Eco actual (1..Seraph::CYCLES); st = estadisticas de ese Eco.
    void Start(const Seraph::Stats& st);
    void Update(float dt, Hero& hero, const Boss& boss, BulletField& bf, Hazards& hz);

    const char* PatternName() const;
    bool IsFlyingMode() const { return flying_; }
    bool InRest() const { return phase_ == Phase::Rest; }
    int  ConsumeBursts() { int r = bursts_; bursts_ = 0; return r; }

private:
    enum class Phase { Intro, Turn, Rest };
    struct Active {
        Pattern p = Pattern::None;
        float t = 0.f, dur = 8.f, tick = 0.f, ang = 0.f, aux = 0.f, scale = 1.f;
        int   n = 0;
    };

    void BeginTurn(const Boss& boss);
    void Run(Active& a, float dt, Hero& hero, const Boss& boss, BulletField& bf, Hazards& hz);
    void SteerTurn(Hero& hero, const Boss& boss) const;
    void SteerRest(Hero& hero, const Boss& boss) const;
    Pattern PickPattern() const;
    static const char* NameOf(Pattern p);

    Phase  phase_ = Phase::Intro;
    float  phaseT_ = 0.f, clock_ = 0.f, restDur_ = 2.2f;
    bool   flying_ = true;
    int    tier_ = 0, turns_ = 0, bursts_ = 0;   // tier_ = intensidad fija del Eco (no cambia durante la pelea)
    float  atk_ = 1.f, rate_ = 1.f;      // conocimiento de ataque (0.5..1) y cadencia relativa
    bool   knowsWalls_ = true, knowsBlasters_ = true, knowsRain_ = true, knowsCombos_ = true;
    Pattern last_ = Pattern::None, main_ = Pattern::None;
    std::vector<Active> active_;
};
