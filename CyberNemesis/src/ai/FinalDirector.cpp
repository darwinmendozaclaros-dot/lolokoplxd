#include "ai/FinalDirector.h"
#include "entities/Hero.h"
#include "entities/Boss.h"
#include "entities/Hazards.h"
#include "entities/Bullets.h"
#include "core/Config.h"
#include "core/Util.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr float TAU = 6.2831853f;

    Bullet Make(Vector2 p, Vector2 v, int hue, BulletKind kind = BulletKind::Orb, float radius = 5.f) {
        Bullet b;
        b.base = p; b.pos = p; b.vel = v; b.hue = hue; b.kind = kind; b.radius = radius;
        b.damage = (kind == BulletKind::Big) ? Cfg::BULLET_DAMAGE_BIG : Cfg::BULLET_DAMAGE_SMALL;
        return b;
    }
    Vector2 Dir(float a, float s) { return { std::cos(a) * s, std::sin(a) * s }; }
}

const char* FinalDirector::NameOf(Pattern p) {
    switch (p) {
        case Pattern::Ring:     return "ANILLOS";
        case Pattern::Spiral:   return "ESPIRAL";
        case Pattern::Rain:     return "LLUVIA";
        case Pattern::Fan:      return "ABANICO";
        case Pattern::Sine:     return "OLAS";
        case Pattern::Blasters: return "CANONES";
        case Pattern::Walls:    return "MUROS";
        case Pattern::Waves:    return "ONDAS DE SUELO";
        case Pattern::Bounce:   return "ESFERAS REBOTANTES";
        case Pattern::Pillars:  return "PILARES";
        default:                return "...";
    }
}

const char* FinalDirector::PatternName() const {
    return phase_ == Phase::Turn ? NameOf(main_) : (phase_ == Phase::Intro ? "" : "...");
}

void FinalDirector::Start(const Seraph::Stats& st) {
    atk_ = st.attack;
    rate_ = 0.6f + 0.4f * st.attack;          // cadencia: 0.8 (Eco 1) .. 1.0 (Eco final)
    tier_ = st.level;                         // intensidad fija durante todo el Eco
    knowsCombos_ = st.attack >= 0.95f;        // solo el Serafin completo combina dos patrones
    knowsRain_ = st.attack >= 0.6f;           // Eco 2 en adelante
    knowsWalls_ = st.attack >= 0.6f;
    knowsBlasters_ = st.attack >= 0.8f;       // Eco 3 en adelante
    phase_ = Phase::Intro; phaseT_ = 0.f; clock_ = 0.f; restDur_ = 2.2f;
    flying_ = true; turns_ = 0; bursts_ = 0;
    last_ = main_ = Pattern::None;
    active_.clear();
}

FinalDirector::Pattern FinalDirector::PickPattern() const {
    std::vector<Pattern> pool;
    if (flying_) {
        pool = { Pattern::Ring, Pattern::Fan };
        if (knowsRain_) pool.push_back(Pattern::Rain);
        if (tier_ >= 1) { pool.push_back(Pattern::Spiral); pool.push_back(Pattern::Sine); }
        if (knowsBlasters_) pool.push_back(Pattern::Blasters);
    } else {
        pool = { Pattern::Waves };
        if (knowsWalls_) pool.push_back(Pattern::Walls);
        if (tier_ >= 1) { pool.push_back(Pattern::Bounce); pool.push_back(Pattern::Pillars); }
        if (knowsBlasters_) pool.push_back(Pattern::Blasters);
    }
    if (pool.size() > 1) pool.erase(std::remove(pool.begin(), pool.end(), last_), pool.end());
    return pool[(size_t)GetRandomValue(0, (int)pool.size() - 1)];
}

void FinalDirector::BeginTurn(const Boss& boss) {
    (void)boss;
    active_.clear();
    Active a;
    a.p = PickPattern();
    a.scale = rate_;
    a.dur = Util::RandRange(7.f, 9.5f);
    a.aux = Util::RandRange(0.f, TAU);
    active_.push_back(a);
    main_ = a.p; last_ = a.p;

    if (knowsCombos_ && Util::Rand01() < 0.5f) {
        Active b;
        b.dur = a.dur; b.scale = 0.6f * rate_; b.aux = Util::RandRange(0.f, TAU);
        if (flying_) {
            if (a.p == Pattern::Ring || a.p == Pattern::Spiral || a.p == Pattern::Fan) b.p = Util::Rand01() < 0.5f ? Pattern::Rain : Pattern::Sine;
            else if (a.p == Pattern::Rain || a.p == Pattern::Sine) b.p = Pattern::Ring;
        } else if (a.p != Pattern::Blasters) {
            b.p = Pattern::Pillars;
        }
        if (b.p != Pattern::None) active_.push_back(b);
    }
    phase_ = Phase::Turn; phaseT_ = 0.f;
}

void FinalDirector::SteerTurn(Hero& hero, const Boss& boss) const {
    if (flying_) {
        switch (main_) {
            case Pattern::Ring: case Pattern::Spiral:
                hero.FinalCommand({ 320.f, 128.f + 10.f * std::sin(clock_ * 1.3f) }, 140.f, true); break;
            case Pattern::Rain:
                hero.FinalCommand({ 320.f + 120.f * std::sin(clock_ * 0.4f), 118.f }, 120.f, true); break;
            default:
                hero.FinalCommand({ 320.f + 200.f * std::sin(clock_ * 0.55f), 128.f + 40.f * std::sin(clock_ * 0.9f + 1.f) }, 175.f, true); break;
        }
    } else {
        float x = 320.f;
        switch (main_) {
            case Pattern::Waves: case Pattern::Blasters: x = boss.pos.x < 320.f ? 540.f : 100.f; break;
            case Pattern::Pillars: x = Clamp(boss.pos.x < 320.f ? boss.pos.x + 240.f : boss.pos.x - 240.f, 60.f, 580.f); break;
            default: x = 320.f; break;
        }
        hero.FinalCommand({ x, Cfg::GROUND_Y }, 150.f, false);
    }
}

void FinalDirector::SteerRest(Hero& hero, const Boss& boss) const {
    if (flying_)
        hero.FinalCommand({ Clamp(boss.pos.x + (boss.pos.x < 320.f ? 170.f : -170.f), 70.f, 570.f), Cfg::GROUND_Y - 112.f }, 150.f, true);
    else
        hero.FinalCommand({ Clamp(boss.pos.x + (boss.pos.x < 320.f ? 200.f : -200.f), 70.f, 570.f), Cfg::GROUND_Y }, 120.f, false);
}

void FinalDirector::Update(float dt, Hero& hero, const Boss& boss, BulletField& bf, Hazards& hz) {
    clock_ += dt;
    phaseT_ += dt;
    bf.SetAimTarget(boss.Center());

    switch (phase_) {
        case Phase::Intro:
            hero.FinalCommand({ 320.f, 128.f }, 110.f, true);
            if (phaseT_ > 1.8f) BeginTurn(boss);
            break;

        case Phase::Turn: {
            for (auto& a : active_) Run(a, dt, hero, boss, bf, hz);
            active_.erase(std::remove_if(active_.begin(), active_.end(), [](const Active& a) { return a.t >= a.dur; }), active_.end());
            SteerTurn(hero, boss);
            if (active_.empty()) {
                ++turns_;
                const bool change = (turns_ % 2 == 0);
                if (change) flying_ = !flying_;
                phase_ = Phase::Rest; phaseT_ = 0.f; restDur_ = change ? 3.0f : 2.2f;
            }
            break;
        }

        case Phase::Rest:
            SteerRest(hero, boss);
            if ((phaseT_ >= restDur_ && hero.IsFlying() == flying_) || phaseT_ > restDur_ + 3.5f) BeginTurn(boss);
            break;
    }
}

void FinalDirector::Run(Active& a, float dt, Hero& hero, const Boss& boss, BulletField& bf, Hazards& hz) {
    a.t += dt;
    a.tick -= dt;
    const float ts = 1.f + 0.22f * (float)tier_;
    const Vector2 o = hero.Center();
    const Vector2 bc = boss.Center();
    const float G = Cfg::GROUND_Y;
    // Conocimiento de ataque: menos balas, mas lentas y de menor dano en los primeros Ecos.
    const float spd = 0.55f + 0.45f * atk_;
    auto add = [&](Bullet b) { b.damage *= atk_; bf.Add(b); };
    auto cnt = [&](int n, int lo) { return std::max(lo, (int)std::lround((float)n * atk_)); };

    switch (a.p) {
        case Pattern::Ring: {
            const float interval = 0.95f / ts / a.scale;
            if (a.tick <= 0.f) {
                a.tick += interval;
                const int n = cnt(14 + 3 * tier_, 6);
                const float sp = (118.f + 12.f * (float)tier_) * spd;
                for (int i = 0; i < n; ++i)
                    add(Make(o, Dir(a.ang + TAU * (float)i / (float)n, sp), a.n % 3));
                if (a.n % 3 == 2)
                    for (int i = 0; i < n; ++i)
                        add(Make(o, Dir(a.ang + TAU * ((float)i + 0.5f) / (float)n, sp * 0.7f), 1));
                a.ang += 0.23f; ++a.n; ++bursts_; hero.FinalCast();
            }
            break;
        }
        case Pattern::Spiral: {
            const int arms = cnt(3 + (tier_ >= 2 ? 1 : 0), 2);
            a.ang += (a.t < a.dur * 0.5f ? 2.1f : -2.1f) * dt;
            const float interval = 0.07f / a.scale;
            while (a.tick <= 0.f) {
                a.tick += interval;
                for (int k = 0; k < arms; ++k)
                    add(Make(o, Dir(a.ang + TAU * (float)k / (float)arms, (140.f + 10.f * (float)tier_) * spd), k % 4));
            }
            if (GetRandomValue(0, 6) == 0) ++bursts_;
            break;
        }
        case Pattern::Rain: {
            const float interval = 0.30f / (1.f + 0.15f * (float)tier_) / a.scale;
            if (a.tick <= 0.f) {
                a.tick += interval;
                const float center = 320.f + 210.f * std::sin(a.t * 0.85f + a.aux);
                const float gapW = std::max(112.f, 152.f - 8.f * (float)tier_) + 56.f * (1.f - atk_);
                for (float x = 14.f; x < 626.f; x += 22.f) {
                    if (std::fabs(x - center) < gapW * 0.5f) continue;
                    Bullet b = Make({ x + Util::RandRange(-3.f, 3.f), -8.f }, { 0.f, (140.f + 12.f * (float)tier_) * spd }, 1, BulletKind::Shard, 5.f);
                    b.ay = 45.f * spd;
                    add(b);
                }
                ++bursts_;
            }
            break;
        }
        case Pattern::Fan: {
            const float interval = 1.05f / (1.f + 0.12f * (float)tier_) / a.scale;
            if (a.tick <= 0.f) {
                a.tick += interval;
                const int n = cnt(5 + 2 * tier_, 3);
                const float base = std::atan2(bc.y - o.y, bc.x - o.x);
                for (int i = 0; i < n; ++i) {
                    const float ang = base + 0.95f * ((float)i / (float)(n - 1) - 0.5f);
                    add(Make(o, Dir(ang, (190.f + 10.f * (float)tier_) * spd), 0));
                }
                Bullet big = Make(o, Dir(base, 60.f), 2, BulletKind::Big, 9.f);
                big.ax = std::cos(base) * 230.f; big.ay = std::sin(base) * 230.f; big.delay = 0.25f;
                add(big);
                ++bursts_; hero.FinalCast();
            }
            break;
        }
        case Pattern::Sine: {
            const int rows = cnt(2 + tier_, 1);
            const float interval = 0.11f / a.scale;
            while (a.tick <= 0.f) {
                a.tick += interval;
                for (int r = 0; r < rows; ++r) {
                    const bool left = (r % 2 == 0);
                    const float y0 = 80.f + (G - 150.f) * ((float)r + 0.5f) / (float)rows;
                    Bullet b = Make({ left ? -8.f : 648.f, y0 }, { (left ? 130.f : -130.f) * spd, 0.f }, r % 4);
                    b.sineAmp = 45.f; b.sineFreq = 3.2f; b.sinePhase = (float)r;
                    add(b);
                }
            }
            if (GetRandomValue(0, 6) == 0) ++bursts_;
            break;
        }
        case Pattern::Blasters: {
            const float interval = 1.7f / (1.f + 0.15f * (float)tier_) / a.scale;
            if (a.tick <= 0.f && bf.BlasterCount() < 4) {
                a.tick += interval;
                const int count = 1 + (tier_ >= 1 ? 1 : 0);
                for (int k = 0; k < count; ++k) {
                    Blaster bl;
                    const float r = Util::Rand01();
                    if (r < 0.33f)      bl.pos = { 24.f, Util::RandRange(80.f, G - 30.f) };
                    else if (r < 0.66f) bl.pos = { 616.f, Util::RandRange(80.f, G - 30.f) };
                    else                bl.pos = { Util::RandRange(60.f, 580.f), 70.f };
                    bl.angle = std::atan2(bc.y - bl.pos.y, bc.x - bl.pos.x);
                    bl.damage = Cfg::BEAM_DAMAGE * atk_;
                    bl.spin = tier_ >= 2 ? (Util::Rand01() < 0.5f ? 0.55f : -0.55f) : 0.f;
                    bf.AddBlaster(bl);
                }
            }
            break;
        }
        case Pattern::Walls: {
            const float interval = 2.9f / (1.f + 0.1f * (float)tier_) / a.scale;
            if (a.tick <= 0.f) {
                a.tick += interval;
                const bool right = Util::Rand01() < 0.5f;
                const bool air = Util::Rand01() < 0.5f;
                const float gapTop = air ? G - 250.f : G - 135.f;
                const float gapBot = gapTop + 125.f + 45.f * (1.f - atk_);
                const float x0 = right ? 625.f : 15.f;
                const float vx = (right ? -1.f : 1.f) * (125.f + 10.f * (float)tier_) * spd;
                for (float y = 14.f; y < G - 2.f; y += 15.f) {
                    if (y > gapTop && y < gapBot) continue;
                    Bullet b = Make({ x0, y }, { vx, 0.f }, 4, BulletKind::Shard, 6.f);
                    b.delay = 0.55f;
                    add(b);
                }
                ++bursts_;
            }
            break;
        }
        case Pattern::Waves: {
            const float interval = 1.1f / (1.f + 0.12f * (float)tier_) / a.scale;
            if (a.tick <= 0.f) {
                a.tick += interval;
                const float dirx = boss.pos.x > o.x ? 1.f : -1.f;
                const bool high = (a.n % 2 == 1);
                add(Make({ o.x + dirx * 30.f, G - (high ? 40.f : 8.f) }, { dirx * (175.f + 10.f * (float)tier_) * spd, 0.f }, 2, BulletKind::Shard, 8.f));
                if (tier_ >= 1) {
                    Bullet b = Make({ o.x + dirx * 30.f, G - (high ? 8.f : 40.f) }, { dirx * (175.f + 10.f * (float)tier_) * spd, 0.f }, 0, BulletKind::Shard, 8.f);
                    b.delay = 0.35f;
                    add(b);
                }
                ++a.n; ++bursts_; hero.FinalCast();
            }
            break;
        }
        case Pattern::Pillars: {
            const float interval = 0.9f / (1.f + 0.1f * (float)tier_) / a.scale;
            if (a.tick <= 0.f && a.n < cnt(6 + 2 * tier_, 3)) {
                a.tick += interval;
                hz.SpawnOrbital(Clamp(bc.x + Util::RandRange(-70.f, 70.f), 30.f, 610.f), Cfg::PILLAR_DAMAGE * atk_);
                ++a.n; hero.FinalCast();
            }
            break;
        }
        case Pattern::Bounce: {
            if (a.n == 0) {
                const int count = cnt(3 + tier_, 2);
                for (int i = 0; i < count; ++i) {
                    const float ang = -1.2f + 2.4f * ((float)i + Util::Rand01() * 0.4f) / (float)count + (i % 2 ? 0.f : 0.5f);
                    Bullet b = Make(o, Dir(ang, 165.f * spd), i % 5, BulletKind::Big, 11.f);
                    b.bounces = 8; b.life = a.dur + 1.f;
                    add(b);
                }
                a.n = 1; a.tick = 3.f; ++bursts_;
            } else if (a.tick <= 0.f) {
                a.tick += 3.f;
                Bullet b = Make(o, Dir(Util::RandRange(-2.4f, -0.7f), 165.f * spd), 2, BulletKind::Big, 11.f);
                b.bounces = 8; b.life = std::max(1.f, a.dur - a.t + 1.f);
                add(b);
            }
            break;
        }
        default: break;
    }
}
