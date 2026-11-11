#include "entities/Summons.h"
#include "entities/Hero.h"
#include "entities/Boss.h"
#include "gfx/Sprites.h"
#include "core/Util.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr float TAU        = 6.2831853f;
    constexpr float DECOY_R    = 26.f;
    constexpr float DECOY_LIFE = 4.5f;
    constexpr float ORB_R      = 10.f;
    constexpr float ORB_ORBIT  = 48.f;
    constexpr float ORB_LIFE   = 7.f;
    const Color SERAPH_GOLD{255, 225, 120, 255};
}

void Summons::Clear() {
    decoys_.clear(); orbs_.clear(); events_.clear();
    st_ = Seraph::Stats{};
    decoyCd_ = guardCd_ = guardAng_ = guardAge_ = 0.f;
    absorbed_ = 0;
}

void Summons::Configure(const Seraph::Stats& st) {
    Clear();
    st_ = st;
    decoyCd_ = 1.5f;    // no invoca nada nada mas empezar
    guardCd_ = 3.0f;
}

Vector2 Summons::OrbPos(size_t i) const {
    const float a = guardAng_ + orbs_[i].off;
    return { c_.x + std::cos(a) * ORB_ORBIT, std::min(c_.y + std::sin(a) * ORB_ORBIT, Cfg::GROUND_Y - 6.f) };
}

void Summons::SpawnDecoys(Vector2 c, Vector2 aim) {
    const float base = std::atan2(aim.y - c.y, aim.x - c.x);
    const int n = std::max(1, st_.decoys);
    for (int i = 0; i < n; ++i) {
        const float off = (n == 1) ? 0.f : (i == 0 ? -0.40f : 0.40f);   // clones de espejo: uno a cada lado de la trayectoria
        Vector2 p = { c.x + std::cos(base + off) * 58.f, c.y + std::sin(base + off) * 58.f };
        p.x = Clamp(p.x, 24.f, (float)Cfg::VIEW_W - 24.f);
        p.y = Clamp(p.y, 70.f, Cfg::GROUND_Y - 30.f);
        decoys_.push_back({ p, 0.f, DECOY_LIFE, std::max(1, st_.decoyHits), aim.x >= c.x });
        events_.push_back({ SummonEvent::DecoySpawn, p });
    }
    decoyCd_ = st_.decoyCd;
}

void Summons::SpawnGuards(Vector2 c, Vector2 aim) {
    orbs_.clear();
    const int n = std::max(1, st_.guardOrbs);
    guardAng_ = std::atan2(aim.y - c.y, aim.x - c.x);     // el primer orbe nace justo en la trayectoria
    guardAge_ = 0.f;
    for (int i = 0; i < n; ++i) orbs_.push_back({ TAU * (float)i / (float)n, true });
    c_ = c;
    for (size_t i = 0; i < orbs_.size(); ++i) events_.push_back({ SummonEvent::GuardSpawn, OrbPos(i) });
    guardCd_ = st_.guardCd;
}

void Summons::Update(float dt, const Hero& hero, const Boss& boss, const std::vector<Projectile>& projs) {
    if (st_.decoys <= 0 && st_.guardOrbs <= 0) return;

    decoyCd_ = std::max(0.f, decoyCd_ - dt);
    guardCd_ = std::max(0.f, guardCd_ - dt);

    for (auto& d : decoys_) d.age += dt;
    decoys_.erase(std::remove_if(decoys_.begin(), decoys_.end(), [](const Decoy& d) { return d.age >= d.life; }), decoys_.end());

    c_ = hero.Center();
    if (!orbs_.empty()) {
        guardAng_ += 3.4f * dt;
        guardAge_ += dt;
        const bool any = std::any_of(orbs_.begin(), orbs_.end(), [](const Orb& o) { return o.alive; });
        if (!any || guardAge_ >= ORB_LIFE) orbs_.clear();
    }
    if (!hero.Alive()) return;

    // Detectar un disparo del Boss que viene hacia el Serafin.
    bool threatened = false;
    Vector2 threat{ 0.f, 0.f };
    float best = 1e9f;
    for (const auto& p : projs) {
        if (!p.alive || p.owner != Owner::Boss) continue;
        const Vector2 rel = Vector2Subtract(c_, p.pos);
        const float dist = Vector2Length(rel);
        if (dist > 240.f || Vector2DotProduct(p.vel, rel) <= 0.f) continue;
        const Vector2 vn = Vector2Normalize(p.vel);
        if (std::fabs(vn.x * rel.y - vn.y * rel.x) > 52.f) continue;
        if (dist < best) { best = dist; threat = p.pos; threatened = true; }
    }
    const bool bossClose = Vector2Distance(boss.Center(), c_) < 150.f;
    if (!threatened && !bossClose) return;

    const Vector2 aim = threatened ? threat : boss.Center();
    if (st_.decoys > 0 && decoyCd_ <= 0.f && decoys_.size() < 2) SpawnDecoys(c_, aim);
    if (st_.guardOrbs > 0 && guardCd_ <= 0.f && orbs_.empty() && threatened) SpawnGuards(c_, aim);
}

bool Summons::Absorb(Vector2 p, float r) {
    for (auto& d : decoys_) {
        if (d.age >= d.life || !CheckCollisionCircles(p, r, d.pos, DECOY_R)) continue;
        ++absorbed_;
        if (--d.hp <= 0) { events_.push_back({ SummonEvent::DecoyBreak, d.pos }); d.age = d.life; }
        else             events_.push_back({ SummonEvent::DecoyHit, d.pos });
        return true;
    }
    for (size_t i = 0; i < orbs_.size(); ++i) {
        if (!orbs_[i].alive) continue;
        const Vector2 op = OrbPos(i);
        if (!CheckCollisionCircles(p, r, op, ORB_R)) continue;
        ++absorbed_;
        orbs_[i].alive = false;
        events_.push_back({ SummonEvent::GuardBreak, op });
        return true;
    }
    return false;
}

void Summons::DestroyIn(const Rectangle& zone) {
    for (auto& d : decoys_) {
        if (d.age < d.life && CheckCollisionCircleRec(d.pos, DECOY_R, zone)) {
            events_.push_back({ SummonEvent::DecoyBreak, d.pos });
            d.age = d.life;
        }
    }
    for (size_t i = 0; i < orbs_.size(); ++i) {
        if (!orbs_[i].alive) continue;
        const Vector2 op = OrbPos(i);
        if (CheckCollisionCircleRec(op, ORB_R, zone)) {
            orbs_[i].alive = false;
            events_.push_back({ SummonEvent::GuardBreak, op });
        }
    }
}

void Summons::Draw(const Hero& hero, const Sprites& sp, float time) const {
    for (const auto& d : decoys_) {
        const float fin  = Util::Clamp01(d.age / 0.3f);
        const float fout = Util::Clamp01((d.life - d.age) / 0.5f);
        const float k = fin * fout;
        if (k <= 0.f) continue;
        hero.DrawEcho(sp, time, { d.pos.x, d.pos.y + 32.f }, 0.6f * k, d.faceRight);
        BeginBlendMode(BLEND_ADDITIVE);
        const float r = DECOY_R + 2.f * std::sin(time * 6.f + d.pos.x);
        DrawPolyLinesEx(d.pos, 6, r, time * 40.f, 1.5f, Fade(SERAPH_GOLD, 0.65f * k));
        for (int h = 0; h < d.hp; ++h)                                    // puntos = disparos que aun absorbe
            DrawCircle((int)(d.pos.x - 6.f * (float)(d.hp - 1) + 12.f * (float)h), (int)(d.pos.y - 44.f), 2.5f, Fade(SERAPH_GOLD, 0.9f * k));
        EndBlendMode();
    }

    if (!orbs_.empty()) {
        const float f = Util::Clamp01((ORB_LIFE - guardAge_) / 0.8f);
        BeginBlendMode(BLEND_ADDITIVE);
        for (size_t i = 0; i < orbs_.size(); ++i) {
            if (!orbs_[i].alive) continue;
            const Vector2 p = OrbPos(i);
            DrawCircleGradient((int)p.x, (int)p.y, 18.f, Fade(SERAPH_GOLD, 0.45f * f), Fade(SERAPH_GOLD, 0.f));
            DrawCircle((int)p.x, (int)p.y, 4.f, Fade(WHITE, 0.9f * f));
            DrawCircleLines((int)p.x, (int)p.y, ORB_R, Fade(SERAPH_GOLD, 0.9f * f));
        }
        EndBlendMode();
    }
}
