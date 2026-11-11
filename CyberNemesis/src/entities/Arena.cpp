#include "entities/Arena.h"
#include "core/Config.h"
#include <cmath>

void Arena::Add(float cx, float topY, float w, float ampX, float ampY, float speed, float phase, int kind) {
    Platform p;
    p.rect.width = w;
    p.origin = { cx, topY };
    p.amp = { ampX, ampY };
    p.speed = speed;
    p.phase = phase;
    p.kind = kind;
    Place(p, false);
    platforms.push_back(p);
}

void Arena::Place(Platform& p, bool computeDelta) {
    const Vector2 old = { p.rect.x, p.rect.y };
    const float s = std::sin(p.speed * t_ + p.phase);
    p.rect.x = p.origin.x - p.rect.width * 0.5f + p.amp.x * s;
    p.rect.y = p.origin.y + p.amp.y * s;
    p.delta = computeDelta ? Vector2{ p.rect.x - old.x, p.rect.y - old.y } : Vector2{ 0.f, 0.f };
}

void Arena::Reset() {
    t_ = 0.f;
    platforms.clear();
    const float G = Cfg::GROUND_Y;
    Add(170.f,  G - 72.f,   110.f,  95.f,  0.f, 0.90f, 0.0f, 0);
    Add(470.f,  G - 72.f,   110.f,  95.f,  0.f, 0.70f, 3.14f, 0);
    Add(320.f,  G - 92.f,    84.f,   0.f, 58.f, 1.10f, 0.0f, 1);
    Add(320.f,  G - 158.f,   92.f, 190.f,  0.f, 0.50f, 1.6f, 0);
    Add( 56.f,  G - 124.f,   80.f,   0.f,  0.f, 0.f,   0.0f, 2);
    Add(584.f,  G - 124.f,   80.f,   0.f,  0.f, 0.f,   0.0f, 2);
}

void Arena::Update(float dt) {
    t_ += dt;
    for (auto& p : platforms) Place(p, true);
}

void Arena::Draw(float time) const {
    static const Color ACC[3] = { Theme::NEON_MAGENTA, Theme::NEON_GREEN, Theme::NEON_YELLOW };

    for (const auto& p : platforms) {
        if (p.kind == 2) continue;
        const Color c = Fade(ACC[p.kind], 0.22f);
        const float cy = p.origin.y + 4.f;
        const float x0 = p.origin.x - p.amp.x, x1 = p.origin.x + p.amp.x;
        const float y0 = cy - p.amp.y, y1 = cy + p.amp.y;
        const int steps = 14;
        for (int i = 0; i <= steps; ++i) {
            float k = (float)i / (float)steps;
            DrawRectangle((int)(x0 + (x1 - x0) * k), (int)(y0 + (y1 - y0) * k), 2, 2, c);
        }
    }

    for (size_t i = 0; i < platforms.size(); ++i) {
        const Platform& p = platforms[i];
        const Color acc = ACC[p.kind];
        const int x = (int)p.rect.x, y = (int)p.rect.y, w = (int)p.rect.width;

        DrawRectangle(x, y + 2, w, 7, Color{12, 9, 30, 255});
        DrawRectangle(x, y, w, 3, Color{52, 40, 98, 255});
        DrawRectangle(x, y, w, 1, Color{104, 90, 170, 255});
        for (int sx = 16; sx < w - 4; sx += 16) {
            DrawRectangle(x + sx, y + 1, 1, 8, Color{6, 4, 16, 255});
            DrawRectangle(x + sx + 3, y + 4, 1, 1, Color{130, 120, 190, 255});
            DrawRectangle(x + sx - 4, y + 4, 1, 1, Color{130, 120, 190, 255});
        }
        if (p.kind != 2) {
            for (int k = 0; k < 8; ++k) {
                const Color hz = ((k / 2) % 2 == 0) ? Color{255, 210, 40, 255} : Color{20, 14, 30, 255};
                DrawRectangle(x + k, y + 3, 1, 5, hz);
                DrawRectangle(x + w - 1 - k, y + 3, 1, 5, hz);
            }
        }
        DrawRectangle(x, y - 1, w, 1, Fade(acc, 0.9f));
        const float run = std::fmod(time * 46.f * (i % 2 ? 1.f : -1.f) + (float)(i * 31), (float)w + 12.f);
        const float rx = run < 0.f ? run + (float)w + 12.f : run;

        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangle(x, y - 3, w, 4, Fade(acc, 0.16f));
        DrawRectangle(x + (int)std::min(rx, (float)w - 10.f), y - 2, 10, 2, Fade(WHITE, 0.75f));
        if (p.kind != 2) {
            for (int s2 = 0; s2 < 2; ++s2) {
                const float fx = (float)x + (s2 == 0 ? 12.f : (float)w - 12.f);
                const float fl = 0.5f + 0.5f * std::sin(time * 33.f + (float)(i * 3 + s2));
                const float len = 7.f + 8.f * fl;
                DrawTriangle({ fx - 3.f, (float)y + 9.f }, { fx, (float)y + 9.f + len }, { fx + 3.f, (float)y + 9.f }, Fade(acc, 0.8f));
                DrawCircleGradient((int)fx, y + 12, 9.f + 3.f * fl, Fade(acc, 0.45f), Fade(acc, 0.f));
            }
        }
        if (p.kind == 1) {
            const float dir = std::cos(p.speed * t_ + p.phase) >= 0.f ? 1.f : -1.f;
            for (int c = 0; c < 3; ++c) {
                const float cx = (float)x + (float)w * 0.5f + (c - 1) * 12.f;
                const float cy = (float)y + 4.f;
                const float off = dir * 2.f;
                DrawLineEx({ cx - 4.f, cy + off }, { cx, cy - 2.f * dir + off }, 1.f, Fade(acc, 0.8f));
                DrawLineEx({ cx + 4.f, cy + off }, { cx, cy - 2.f * dir + off }, 1.f, Fade(acc, 0.8f));
            }
        }
        EndBlendMode();
    }
}
