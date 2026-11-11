#include "entities/Pickups.h"
#include "entities/Arena.h"
#include "core/Config.h"
#include "core/Util.h"
#include <cmath>

void Pickups::SpawnCore(const Arena& arena) {
    if (arena.platforms.empty()) return;
    platform_ = GetRandomValue(0, (int)arena.platforms.size() - 1);
    const Platform& p = arena.platforms[platform_];
    ox_ = Util::RandRange(-p.rect.width * 0.5f + 16.f, p.rect.width * 0.5f - 16.f);
    life_ = 16.f; t_ = 0.f; alive_ = true;
    pos_ = { p.rect.x + p.rect.width * 0.5f + ox_, p.rect.y - 22.f };
}

bool Pickups::Update(float dt, const Arena& arena, const Rectangle& collector) {
    if (!alive_) return false;
    t_ += dt; life_ -= dt;
    if (life_ <= 0.f) { alive_ = false; return false; }
    const Platform& p = arena.platforms[platform_];
    pos_ = { p.rect.x + p.rect.width * 0.5f + ox_, p.rect.y - 22.f + std::sin(t_ * 4.f) * 3.f };
    if (CheckCollisionCircleRec(pos_, 13.f, collector)) { alive_ = false; return true; }
    return false;
}

void Pickups::Draw(float time) const {
    if (!alive_) return;
    if (life_ < 4.f && std::fmod(time * 8.f, 1.f) < 0.4f) return;
    const int cx = (int)pos_.x, cy = (int)pos_.y;
    const float rot = time * 70.f;

    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangle(cx - 1, cy, 3, (int)(Cfg::GROUND_Y - pos_.y), Fade(Theme::NEON_YELLOW, 0.06f));
    DrawCircleGradient(cx, cy, 28.f, Fade(Theme::NEON_YELLOW, 0.5f), Fade(Theme::NEON_YELLOW, 0.f));
    DrawPolyLinesEx(pos_, 6, 17.f, -rot * 0.6f, 1.f, Fade(WHITE, 0.55f));
    for (int i = 0; i < 3; ++i) {
        const float a = time * 3.f + (float)i * 2.0944f;
        DrawCircleGradient((int)(pos_.x + std::cos(a) * 17.f), (int)(pos_.y + std::sin(a) * 6.f), 4.f, Fade(WHITE, 0.8f), Fade(WHITE, 0.f));
    }
    EndBlendMode();

    DrawPoly(pos_, 6, 11.f, rot, Color{255, 190, 40, 255});
    DrawPoly(pos_, 6, 8.f, rot, Color{255, 232, 120, 255});
    for (int i = 0; i < 3; ++i) {
        const float a = (rot + (float)i * 120.f) * 0.0174533f;
        DrawLineEx(pos_, { pos_.x + std::cos(a) * 11.f, pos_.y + std::sin(a) * 11.f }, 1.f, Fade(Color{255, 250, 210, 255}, 0.85f));
    }
    DrawPoly(pos_, 6, 3.5f, -rot, WHITE);
    BeginBlendMode(BLEND_ADDITIVE);
    const float tw = 0.5f + 0.5f * std::sin(time * 9.f);
    DrawRectangle(cx - 1, cy - 9 - (int)(3 * tw), 2, 4 + (int)(6 * tw), Fade(WHITE, 0.8f));
    DrawRectangle(cx - 9 - (int)(3 * tw), cy - 1, 4 + (int)(6 * tw), 2, Fade(WHITE, 0.8f));
    EndBlendMode();
}
