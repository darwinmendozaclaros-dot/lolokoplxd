#include "entities/Hazards.h"
#include "core/Config.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

void Hazards::SpawnMine(Vector2 p, float damage) {
    Mine m; m.pos = p; m.damage = damage; m.arm = Cfg::MINE_ARM_TIME;
    if (mines_.size() >= 6) mines_.erase(mines_.begin());
    mines_.push_back(m);
}

void Hazards::SpawnOrbital(float x, float damage) {
    Orbital o; o.x = x; o.damage = damage; o.delay = Cfg::ORBITAL_DELAY;
    orbitals_.push_back(o);
}

void Hazards::SpawnStrike(float x, float damage) {
    Orbital o; o.x = x; o.damage = damage; o.delay = 1.15f; o.width = 40.f; o.neutral = true;
    orbitals_.push_back(o);
}

std::vector<float> Hazards::NeutralColumns() const {
    std::vector<float> xs;
    for (const auto& o : orbitals_)
        if (o.neutral && !o.fired) xs.push_back(o.x);
    return xs;
}

void Hazards::Update(float dt, const Rectangle& bossHurt, float bossFeetY, std::vector<HazardEvent>& out) {
    const float bossCx = bossHurt.x + bossHurt.width * 0.5f;

    for (auto& m : mines_) {
        m.arm -= dt;
        m.life -= dt;
        if (m.life <= 0.f) { m.alive = false; continue; }
        if (m.arm > 0.f) continue;
        const bool near = std::fabs(bossCx - m.pos.x) < bossHurt.width * 0.5f + 26.f;
        const bool low  = bossFeetY > m.pos.y - 36.f;
        if (near && low) {
            out.push_back({ HazardEvent::MineBlast, m.pos, 74.f, m.damage });
            m.alive = false;
        }
    }
    mines_.erase(std::remove_if(mines_.begin(), mines_.end(), [](const Mine& m) { return !m.alive; }), mines_.end());

    for (auto& o : orbitals_) {
        o.t += dt;
        if (!o.fired && o.t >= o.delay) {
            o.fired = true;
            o.fireT = 0.3f;
            out.push_back({ HazardEvent::OrbitalFire, { o.x, 0.f }, o.width * 0.5f, o.damage, o.neutral });
        }
        if (o.fired) o.fireT -= dt;
    }
    orbitals_.erase(std::remove_if(orbitals_.begin(), orbitals_.end(),
                    [](const Orbital& o) { return o.fired && o.fireT <= 0.f; }), orbitals_.end());
}

void Hazards::DestroyMinesIn(const Rectangle& zone, std::vector<HazardEvent>& out) {
    for (auto& m : mines_) {
        if (m.alive && CheckCollisionPointRec(m.pos, zone)) {
            out.push_back({ HazardEvent::MineFizzle, m.pos, 0.f, 0.f });
            m.alive = false;
        }
    }
    mines_.erase(std::remove_if(mines_.begin(), mines_.end(), [](const Mine& m) { return !m.alive; }), mines_.end());
}

void Hazards::Draw(float time) const {
    for (const auto& m : mines_) {
        const bool armed = m.arm <= 0.f;
        const int x = (int)m.pos.x, y = (int)m.pos.y;
        DrawRectangle(x - 7, y - 3, 14, 3, Color{34, 36, 56, 255});
        DrawRectangle(x - 8, y - 1, 2, 2, Color{70, 76, 110, 255});
        DrawRectangle(x + 6, y - 1, 2, 2, Color{70, 76, 110, 255});
        DrawCircle(x, y - 4, 6.f, Color{60, 64, 94, 255});
        DrawRectangle(x - 5, y - 8, 4, 1, Color{120, 128, 170, 255});
        for (int k = -1; k <= 1; ++k) DrawRectangle(x + k * 4 - 1, y - 12, 2, 3, Color{90, 96, 134, 255});
        const bool blink = armed ? std::fmod(time * 6.f, 1.f) < 0.5f : std::fmod(time * 14.f, 1.f) < 0.5f;
        const Color led = armed ? Theme::NEON_RED : Theme::NEON_YELLOW;
        DrawRectangle(x - 1, y - 6, 3, 3, blink ? led : Fade(led, 0.25f));
        BeginBlendMode(BLEND_ADDITIVE);
        if (blink) DrawCircleGradient(x, y - 5, 11.f, Fade(led, 0.6f), Fade(led, 0.f));
        if (armed) DrawCircleLines(x, y - 2, 74.f, Fade(Theme::NEON_RED, 0.08f));
        else DrawCircleLines(x, y - 2, 10.f + (1.f - m.arm / Cfg::MINE_ARM_TIME) * 14.f, Fade(Theme::NEON_YELLOW, 0.5f));
        EndBlendMode();
    }

    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& o : orbitals_) {
        if (!o.fired) {
            const float k = Clamp(o.t / o.delay, 0.f, 1.f);
            const float r = o.width * 0.5f;
            const Color W1 = o.neutral ? Color{255, 150, 40, 255} : Theme::NEON_RED;
            DrawRectangle((int)(o.x - r), 0, (int)(r * 2.f), (int)Cfg::GROUND_Y, Fade(W1, 0.04f + 0.10f * k));
            DrawRectangle((int)o.x, 0, 1, (int)Cfg::GROUND_Y, Fade(W1, 0.5f));
            DrawCircleLines((int)o.x, (int)Cfg::GROUND_Y, r * (2.4f - 1.4f * k), Fade(W1, 0.9f));
            DrawCircleLines((int)o.x, (int)Cfg::GROUND_Y, r, Fade(Theme::NEON_YELLOW, 0.5f));
            if (std::fmod(time * 20.f, 1.f) < 0.5f)
                DrawRectangle((int)(o.x - r), (int)Cfg::GROUND_Y - 2, (int)(r * 2.f), 3, Fade(W1, 0.8f));
        } else {
            const float k = Clamp(o.fireT / 0.3f, 0.f, 1.f);
            const float r = o.width * 0.5f * (0.4f + 0.6f * k);
            const Color F1 = o.neutral ? Color{255, 170, 60, 255} : Theme::NEON_CYAN;
            DrawRectangle((int)(o.x - r * 1.8f), 0, (int)(r * 3.6f), (int)Cfg::GROUND_Y, Fade(F1, 0.25f * k));
            DrawRectangle((int)(o.x - r), 0, (int)(r * 2.f), (int)Cfg::GROUND_Y, Fade(WHITE, 0.9f * k));
            DrawCircleGradient((int)o.x, (int)Cfg::GROUND_Y, 50.f * k, Fade(F1, 0.8f * k), Fade(F1, 0.f));
        }
    }
    EndBlendMode();
}
