#include "entities/Bullets.h"
#include "gfx/Sprites.h"
#include "core/Config.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
    Color HueColor(int h) {
        switch (((h % 5) + 5) % 5) {
            case 0:  return Theme::NEON_MAGENTA;
            case 1:  return Theme::NEON_CYAN;
            case 2:  return Theme::NEON_YELLOW;
            case 3:  return Color{255, 130, 175, 255};
            default: return Theme::NEON_GREEN;
        }
    }
    Vector2 Dir(float a) { return { std::cos(a), std::sin(a) }; }
}

void BulletField::Clear() { b_.clear(); bl_.clear(); chargedPending_ = firedPending_ = 0; }

void BulletField::Update(float dt) {
    for (auto& b : b_) {
        b.age += dt;
        if (b.delay > 0.f) { b.delay -= dt; continue; }
        b.life -= dt;
        b.vel.x += b.ax * dt; b.vel.y += b.ay * dt;
        b.base.x += b.vel.x * dt; b.base.y += b.vel.y * dt;
        if (b.bounces > 0) {
            const float r = b.radius;
            if ((b.base.x < r && b.vel.x < 0.f) || (b.base.x > Cfg::VIEW_W - r && b.vel.x > 0.f)) { b.vel.x = -b.vel.x; --b.bounces; }
            if (b.base.y > Cfg::GROUND_Y - r && b.vel.y > 0.f) { b.vel.y = -b.vel.y; --b.bounces; }
            if (b.base.y < r + 30.f && b.vel.y < 0.f) { b.vel.y = -b.vel.y; --b.bounces; }
        }
        if (b.sineAmp != 0.f) {
            const float s = std::sin(b.sineFreq * b.age + b.sinePhase) * b.sineAmp;
            b.pos = { b.base.x + b.sineAxis.x * s, b.base.y + b.sineAxis.y * s };
        } else {
            b.pos = b.base;
        }
        if (b.life <= 0.f || b.base.x < -50.f || b.base.x > Cfg::VIEW_W + 50.f ||
            b.base.y < -70.f || b.base.y > Cfg::VIEW_H + 40.f)
            b.alive = false;
    }
    b_.erase(std::remove_if(b_.begin(), b_.end(), [](const Bullet& b) { return !b.alive; }), b_.end());

    for (auto& bl : bl_) {
        bl.t += dt;
        if (!bl.lockedAim) {
            if (bl.t < bl.charge * 0.55f) bl.angle = std::atan2(aimAt_.y - bl.pos.y, aimAt_.x - bl.pos.x);
            else bl.lockedAim = true;
        }
        if (!bl.fired && bl.t >= bl.charge) { bl.fired = true; ++firedPending_; }
        if (bl.fired) bl.angle += bl.spin * dt;
        if (bl.t > bl.charge + bl.fire) bl.alive = false;
    }
    bl_.erase(std::remove_if(bl_.begin(), bl_.end(), [](const Blaster& b) { return !b.alive; }), bl_.end());
}

BulletHit BulletField::Collide(const Rectangle& r, bool vulnerable) {
    BulletHit h;
    if (!vulnerable) return h;
    for (auto& b : b_) {
        if (!b.alive || b.delay > 0.f) continue;
        if (CheckCollisionCircleRec(b.pos, b.radius * 0.8f, r)) {
            h.hit = true; h.damage = b.damage; h.pos = b.pos;
            b.alive = false;
            return h;
        }
    }
    for (const auto& bl : bl_) {
        if (!bl.fired || bl.t > bl.charge + bl.fire) continue;
        const Vector2 d = Dir(bl.angle);
        for (float s = 18.f; s < bl.length; s += 10.f) {
            const Vector2 p = { bl.pos.x + d.x * s, bl.pos.y + d.y * s };
            if (p.x < -40.f || p.x > Cfg::VIEW_W + 40.f || p.y < -40.f || p.y > Cfg::VIEW_H + 40.f) break;
            if (CheckCollisionCircleRec(p, bl.width * 0.42f, r)) {
                h.hit = true; h.damage = bl.damage; h.pos = p;
                return h;
            }
        }
    }
    return h;
}

int BulletField::ClearIn(const Rectangle& zone) {
    int n = 0;
    for (auto& b : b_)
        if (b.alive && b.delay <= 0.f && CheckCollisionPointRec(b.pos, zone)) { b.alive = false; ++n; }
    b_.erase(std::remove_if(b_.begin(), b_.end(), [](const Bullet& b) { return !b.alive; }), b_.end());
    return n;
}

void BulletField::Draw(const Sprites& sp, float time) const {
    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& bl : bl_) {
        const Vector2 d = Dir(bl.angle);
        const Vector2 end = { bl.pos.x + d.x * bl.length, bl.pos.y + d.y * bl.length };
        if (!bl.fired) {
            const float k = Clamp(bl.t / bl.charge, 0.f, 1.f);
            const bool blink = std::fmod(time * 18.f, 1.f) < 0.55f;
            DrawLineEx(bl.pos, end, 1.f + 3.f * k, Fade(Theme::NEON_RED, (0.18f + 0.5f * k) * (blink ? 1.f : 0.6f)));
            sp.DrawGlow(bl.pos, 16.f + 20.f * k, Fade(Theme::NEON_CYAN, 0.5f * k));
        } else {
            const float f = Clamp((bl.t - bl.charge) / bl.fire, 0.f, 1.f);
            const float w = bl.width * (1.f - 0.55f * f * f);
            DrawLineEx(bl.pos, end, w * 1.7f, Fade(Theme::NEON_CYAN, 0.22f));
            DrawLineEx(bl.pos, end, w, Fade(Theme::NEON_MAGENTA, 0.45f));
            DrawLineEx(bl.pos, end, w * 0.55f, Fade(WHITE, 0.95f));
            sp.DrawGlow(bl.pos, 34.f, Fade(WHITE, 0.8f * (1.f - f)));
        }
    }
    for (const auto& b : b_) {
        if (b.delay > 0.f) continue;
        const float g = (b.kind == BulletKind::Big ? 3.6f : 3.0f) * b.radius;
        sp.DrawGlow(b.pos, g, Fade(HueColor(b.hue), 0.55f));
    }
    EndBlendMode();

    for (const auto& b : b_) {
        const Color c = HueColor(b.hue);
        if (b.delay > 0.f) {
            const float a = 0.30f + 0.25f * std::sin(time * 14.f + b.base.y);
            DrawCircleLines((int)b.pos.x, (int)b.pos.y, b.radius + 2.f, Fade(c, a));
            continue;
        }
        const float ang = std::atan2(b.vel.y, b.vel.x) * RAD2DEG;
        switch (b.kind) {
            case BulletKind::Shard:
                DrawPoly(b.pos, 4, b.radius * 1.5f, ang + 45.f, c);
                DrawPoly(b.pos, 4, b.radius * 0.7f, ang + 45.f, WHITE);
                break;
            case BulletKind::Star:
                DrawPoly(b.pos, 5, b.radius * 1.6f, time * 120.f, c);
                DrawPoly(b.pos, 5, b.radius * 0.8f, time * 120.f, WHITE);
                break;
            case BulletKind::Big:
                DrawCircleV(b.pos, b.radius, c);
                DrawCircleLines((int)b.pos.x, (int)b.pos.y, b.radius + 3.f, Fade(WHITE, 0.8f));
                DrawCircleV(b.pos, b.radius * 0.6f, WHITE);
                break;
            default:
                DrawCircleV(b.pos, b.radius, c);
                DrawCircleV(b.pos, b.radius * 0.55f, WHITE);
                break;
        }
    }

    for (const auto& bl : bl_) {
        const Vector2 d = Dir(bl.angle), n = { -d.y, d.x };
        const float deg = bl.angle * RAD2DEG;
        const float open = bl.fired ? 4.f : 2.f * Clamp(bl.t / bl.charge, 0.f, 1.f);
        const bool hot = bl.t > bl.charge * 0.55f;
        DrawCircleV(bl.pos, 11.f, Color{226, 228, 240, 255});
        DrawRectanglePro({ bl.pos.x + d.x * 9.f, bl.pos.y + d.y * 9.f, 11.f, 8.f + open }, { 5.f, 4.f + open * 0.5f }, deg, Color{196, 198, 214, 255});
        for (int s2 = -1; s2 <= 1; s2 += 2) {
            const Vector2 e = { bl.pos.x + d.x * 3.f + n.x * 4.5f * (float)s2, bl.pos.y + d.y * 3.f + n.y * 4.5f * (float)s2 };
            DrawCircleV(e, 3.f, Color{20, 12, 30, 255});
            DrawCircleV(e, 1.6f, hot ? WHITE : Theme::NEON_RED);
        }
    }
}
