#include "entities/Projectile.h"
#include "core/Config.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

void Projectile::Update(float dt, Vector2 target) {
    trail.push_back(pos);
    if (trail.size() > 10) trail.erase(trail.begin());

    if (homing && life > 1.2f) {
        const Vector2 d = Vector2Subtract(target, pos);
        const float want = std::atan2(d.y, d.x);
        float cur = std::atan2(vel.y, vel.x);
        float diff = want - cur;
        while (diff >  PI) diff -= 2.f * PI;
        while (diff < -PI) diff += 2.f * PI;
        const float maxTurn = turnRate * dt;
        diff = std::max(-maxTurn, std::min(maxTurn, diff));
        cur += diff;
        const float sp = std::min(maxSpeed, Vector2Length(vel) + 260.f * dt);
        vel = { std::cos(cur) * sp, std::sin(cur) * sp };
    }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    life  -= dt;

    if (life <= 0.f || pos.x < -60.f || pos.x > Cfg::VIEW_W + 60.f ||
        pos.y < -80.f || pos.y > Cfg::GROUND_Y)
        alive = false;
}

void Projectile::Draw(float time) const {
    const Color c = (owner == Owner::Boss) ? Theme::NEON_MAGENTA
                  : (kind == ProjKind::Missile ? Theme::NEON_YELLOW
                  : (kind == ProjKind::Pellet ? Color{120, 255, 200, 255} : Theme::NEON_CYAN));
    const size_t n = trail.size();
    const float ang = std::atan2(vel.y, vel.x);

    for (size_t i = 1; i < n; ++i) {
        const float k = (float)i / (float)n;
        DrawLineEx(trail[i - 1], trail[i], std::max(1.f, radius * 1.5f * k), Fade(c, 0.45f * k));
    }

    if (kind == ProjKind::Missile) {
        const float deg = ang * RAD2DEG;
        const float fl = 0.6f + 0.4f * std::sin(time * 60.f);
        const Vector2 tail = { pos.x - std::cos(ang) * 8.f, pos.y - std::sin(ang) * 8.f };
        DrawCircleGradient((int)tail.x, (int)tail.y, 11.f * fl + 4.f, Fade(Theme::NEON_RED, 0.8f), Fade(Theme::NEON_YELLOW, 0.f));
        DrawCircleGradient((int)pos.x, (int)pos.y, 13.f, Fade(c, 0.35f), Fade(c, 0.f));
        DrawRectanglePro({ pos.x, pos.y, 12.f, 4.f }, { 8.f, 2.f }, deg, Fade(WHITE, 0.95f));
        DrawRectanglePro({ pos.x, pos.y, 4.f, 4.f }, { -4.f, 2.f }, deg, Fade(Theme::NEON_RED, 0.95f));
        DrawRectanglePro({ pos.x, pos.y, 5.f, 8.f }, { 9.f, 4.f }, deg, Fade(Color{255, 190, 60, 255}, 0.85f));
        return;
    }
    if (kind == ProjKind::Pellet) {
        DrawCircleGradient((int)pos.x, (int)pos.y, radius * 3.f, Fade(c, 0.4f), Fade(c, 0.f));
        DrawPoly(pos, 4, radius * 1.6f, ang * RAD2DEG + 45.f, Fade(c, 0.95f));
        DrawPoly(pos, 4, radius * 0.7f, ang * RAD2DEG + 45.f, WHITE);
        return;
    }
    if (kind == ProjKind::Bolt) {
        const Vector2 head = pos;
        const Vector2 back = { pos.x - std::cos(ang) * (radius * 4.f), pos.y - std::sin(ang) * (radius * 4.f) };
        DrawLineEx(back, head, radius * 1.8f, Fade(c, 0.45f));
        DrawLineEx(back, head, radius * 0.8f, Fade(WHITE, 0.95f));
        DrawCircleGradient((int)head.x, (int)head.y, radius * 3.f, Fade(c, 0.5f), Fade(c, 0.f));
        return;
    }
    DrawCircleV(pos, radius * 2.0f, Fade(c, 0.22f));
    DrawCircleV(pos, radius * 1.3f, Fade(c, 0.8f));
    for (int i = 0; i < 4; ++i) {
        const float a = time * 9.f + (float)i * 1.5708f;
        DrawPoly({ pos.x + std::cos(a) * radius * 1.7f, pos.y + std::sin(a) * radius * 1.7f }, 3, 2.2f, a * RAD2DEG, Fade(WHITE, 0.85f));
    }
    DrawCircleV(pos, radius * 0.65f, WHITE);
}
