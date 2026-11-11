#include "entities/Drones.h"
#include "core/Config.h"
#include "core/Util.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

void DroneSwarm::Spawn(int n, Vector2 from, float damage) {
    damage_ = damage;
    for (int i = 0; i < n; ++i) {
        Drone d;
        d.pos = { from.x + (float)(i - n / 2) * 24.f, -20.f - i * 14.f };
        d.slot = i;
        d.side = (i % 2 == 0) ? 1.f : -1.f;
        d.phase = Util::RandRange(0.f, 6.28f);
        d.shootT = Util::RandRange(0.8f, 1.6f);
        drones_.push_back(d);
    }
}

void DroneSwarm::Update(float dt, Vector2 bossCenter, std::vector<ShotCmd>& shots) {
    for (auto& d : drones_) {
        d.life -= dt;
        d.hitFlash = std::max(0.f, d.hitFlash - dt * 5.f);
        if (d.life <= 0.f) { d.alive = false; continue; }

        const float tx = Clamp(bossCenter.x + d.side * (130.f + 38.f * (float)d.slot) + std::sin(d.phase + d.life * 0.9f) * 40.f,
                               30.f, (float)Cfg::VIEW_W - 30.f);
        const float ty = 105.f + 32.f * (float)(d.slot % 3) + std::sin(d.phase + d.life * 2.3f) * 10.f;
        const float k = std::min(1.f, dt * 1.6f);
        d.pos.x += (tx - d.pos.x) * k;
        d.pos.y += (ty - d.pos.y) * k;

        d.shootT -= dt;
        if (d.shootT <= 0.f && d.pos.y > 20.f) {
            d.shootT = Util::RandRange(1.3f, 2.1f);
            ShotCmd s;
            s.origin = d.pos;
            s.dir = Vector2Normalize(Vector2Subtract(bossCenter, d.pos));
            s.speed = 280.f; s.damage = damage_; s.radius = 3.5f; s.kind = ProjKind::Bolt;
            shots.push_back(s);
        }
    }
    drones_.erase(std::remove_if(drones_.begin(), drones_.end(), [](const Drone& d) { return !d.alive; }), drones_.end());
}

bool DroneSwarm::HitCircle(Vector2 p, float r, float dmg) {
    for (auto& d : drones_) {
        if (!d.alive) continue;
        if (Vector2Distance(p, d.pos) < r + 11.f) {
            d.hp -= dmg; d.hitFlash = 1.f;
            if (d.hp <= 0.f) { d.alive = false; deaths_.push_back(d.pos); }
            return true;
        }
    }
    return false;
}

void DroneSwarm::DestroyIn(const Rectangle& zone) {
    for (auto& d : drones_)
        if (d.alive && CheckCollisionPointRec(d.pos, zone)) { d.alive = false; deaths_.push_back(d.pos); }
}

void DroneSwarm::Draw(float time) const {
    for (const auto& d : drones_) {
        if (!d.alive) continue;
        const int x = (int)d.pos.x, y = (int)d.pos.y;
        const bool hit = d.hitFlash > 0.f;
        const Color hull  = hit ? Color{255, 140, 140, 255} : Color{44, 48, 78, 255};
        const Color hullL = hit ? Color{255, 200, 200, 255} : Color{92, 100, 150, 255};

        for (int s2 = -1; s2 <= 1; s2 += 2) {
            DrawRectangle(x + s2 * 7 - (s2 < 0 ? 4 : 0), y - 3, 4, 2, Color{26, 28, 46, 255});
            const float spin = std::fmod(time * 55.f + d.phase + (float)s2, 2.f) < 1.f ? 1.f : -1.f;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawEllipse(x + s2 * 11, y - 5, 7.f, 1.6f, Fade(WHITE, 0.22f));
            DrawRectangle(x + s2 * 11 - (int)(6 * spin), y - 5, 12, 1, Fade(WHITE, 0.55f));
            EndBlendMode();
        }
        DrawRectangle(x - 8, y - 3, 16, 8, hull);
        DrawRectangle(x - 8, y - 3, 16, 1, hullL);
        DrawRectangle(x - 6, y + 5, 12, 2, Color{26, 28, 46, 255});
        DrawRectangle(x - 8, y + 1, 16, 1, Color{20, 22, 38, 255});
        DrawRectangle(x - 1, y + 7, 3, 3, Color{70, 76, 110, 255});
        const bool blink = d.life < 3.f && std::fmod(time * 8.f, 1.f) < 0.5f;
        DrawRectangle(x - 3, y - 1, 7, 3, Color{12, 8, 20, 255});
        DrawRectangle(x - 2 + (int)(std::sin(time * 3.f + d.phase) * 1.5f), y - 1, 3, 3,
                      blink ? WHITE : Theme::NEON_RED);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(x, y, 10.f, Fade(blink ? WHITE : Theme::NEON_RED, 0.5f), Fade(Theme::NEON_RED, 0.f));
        DrawCircleGradient(x, y + 9, 6.f, Fade(Theme::NEON_CYAN, 0.25f + 0.15f * std::sin(time * 20.f)), Fade(Theme::NEON_CYAN, 0.f));
        EndBlendMode();
    }
}
