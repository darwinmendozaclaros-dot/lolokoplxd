#include "gfx/Background.h"
#include "core/Config.h"
#include "core/Util.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {
    const float LAYER_SPEED[3] = { 2.f, 5.f, 10.f };

    unsigned Hash(unsigned a, unsigned b, unsigned c) {
        unsigned h = a * 2654435761u ^ b * 40503u ^ c * 2246822519u;
        h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
        return h;
    }
    Color C(int r, int g, int b) { return Color{ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 }; }
}

Background::Palette Background::ThemePalette(int i) {
    Palette p{};
    switch (std::max(0, std::min(5, i))) {
        case 0:
            p = { C(8,4,30),  C(88,14,108), C(255,225,80),  C(255,43,214), C(255,43,214),
                  { C(24,240,255), C(255,43,214), C(255,230,60) } }; break;
        case 1:
            p = { C(2,8,30),  C(10,62,118), C(130,255,230), C(40,120,255), C(24,240,255),
                  { C(24,240,255), C(70,150,255), C(220,255,255) } }; break;
        case 2:
            p = { C(20,6,10), C(150,52,20), C(255,205,60),  C(255,90,30),  C(255,140,40),
                  { C(255,150,40), C(255,230,60), C(255,70,50) } }; break;
        case 3:
            p = { C(2,14,8),  C(18,92,52),  C(200,255,100), C(57,255,120), C(57,255,120),
                  { C(57,255,120), C(190,255,60), C(24,240,255) } }; break;
        case 4:
            p = { C(26,0,8),  C(142,10,42), C(255,130,60),  C(255,30,70),  C(255,48,64),
                  { C(255,48,64), C(255,140,50), C(255,43,214) } }; break;
        default:
            p = { C(0,0,0),   C(52,0,92),   C(255,255,255), C(160,60,255), C(220,200,255),
                  { C(255,255,255), C(160,60,255), C(24,240,255) } }; break;
    }
    return p;
}

void Background::LerpPalette(Palette& c, const Palette& t, float k) {
    auto L = [k](Color& a, const Color& b) { a = Util::LerpColor(a, b, k); };
    L(c.skyTop, t.skyTop); L(c.skyBot, t.skyBot); L(c.sunTop, t.sunTop);
    L(c.sunBot, t.sunBot); L(c.grid, t.grid);
    for (int i = 0; i < 3; ++i) L(c.acc[i], t.acc[i]);
}

void Background::SetTheme(int generation) { target_ = ThemePalette(generation - 1); }

void Background::Init(unsigned seed) {
    SetRandomSeed(seed);
    cur_ = target_ = ThemePalette(0);

    for (int l = 0; l < 3; ++l) {
        layers_[l].clear();
        float x = -20.f;
        const float hMin = 60.f + l * 20.f, hMax = 140.f + l * 35.f;
        while (x < Cfg::VIEW_W + 140.f) {
            float w = (float)GetRandomValue(36, 78);
            Building b;
            b.x = x; b.w = w;
            b.h = (float)GetRandomValue((int)hMin, (int)hMax);
            b.seed = (unsigned)GetRandomValue(1, 100000);
            b.accent = GetRandomValue(0, 2);
            b.sign = (l == 2) && GetRandomValue(0, 2) == 0;
            b.billboard = (l == 1) && GetRandomValue(0, 3) == 0;
            b.antenna = (l >= 1) && GetRandomValue(0, 3) == 0;
            layers_[l].push_back(b);
            x += w + (float)GetRandomValue(0, 6);
        }
        wrap_[l] = x;
    }

    stars_.clear();
    for (int i = 0; i < 50; ++i)
        stars_.push_back({ (float)GetRandomValue(0, Cfg::VIEW_W), (float)GetRandomValue(0, 150),
                           (float)GetRandomValue(0, 628) / 100.f });

    drones_.clear();
    for (int i = 0; i < 3; ++i)
        drones_.push_back({ (float)GetRandomValue(0, Cfg::VIEW_W), (float)GetRandomValue(110, 200),
                            (float)GetRandomValue(14, 32) * (GetRandomValue(0, 1) ? 1.f : -1.f),
                            (float)GetRandomValue(0, 628) / 100.f });

    drops_.clear();
    for (int i = 0; i < 170; ++i)
        drops_.push_back({ (float)GetRandomValue(-60, Cfg::VIEW_W + 60), (float)GetRandomValue(0, Cfg::VIEW_H),
                           (float)GetRandomValue(380, 560) });
    columns_.clear();
    for (int i = 0; i < 40; ++i)
        columns_.push_back({ (float)(i * 16 + 8), (float)GetRandomValue(-300, 300),
                             (float)GetRandomValue(60, 160), GetRandomValue(5, 10) });
}

void Background::StartEvent(Event e) {
    ev_ = e; lastEv_ = e; evT_ = 0.f;
    switch (e) {
        case Event::Storm:    evDur_ = Util::RandRange(13.f, 18.f); lightTimer_ = Util::RandRange(1.5f, 3.f); break;
        case Event::DataRain: evDur_ = Util::RandRange(11.f, 15.f); break;
        case Event::Meteors:  evDur_ = Util::RandRange(9.f, 13.f);  meteorTimer_ = 0.f; break;
        case Event::Airship:  evDur_ = 15.f; break;
        case Event::Blackout: evDur_ = Util::RandRange(6.f, 8.f);   break;
        default: evDur_ = 0.f; break;
    }
}

void Background::EndEvent() {
    ev_ = Event::None;
    evTimer_ = Util::RandRange(12.f, 22.f);
    meteors_.clear();
}

const char* Background::EventLabel(float& alpha) const {
    alpha = 0.f;
    if (ev_ == Event::None) return "";
    if (evT_ < 3.f) alpha = std::min(1.f, evT_ / 0.4f);
    else            alpha = std::max(0.f, 1.f - (evT_ - 3.f) / 0.6f);
    switch (ev_) {
        case Event::Storm:    return "EVENTO: TORMENTA ELECTRICA";
        case Event::DataRain: return "EVENTO: LLUVIA DE DATOS";
        case Event::Meteors:  return "EVENTO: LLUVIA DE METEOROS";
        case Event::Airship:  return "EVENTO: PATRULLA AEREA";
        case Event::Blackout: return "EVENTO: APAGON DE LA CIUDAD";
        default:              return "";
    }
}

void Background::Update(float dt) {
    for (int l = 0; l < 3; ++l) scroll_[l] += LAYER_SPEED[l] * dt;
    LerpPalette(cur_, target_, 1.f - std::exp(-dt * 1.4f));

    for (auto& d : drones_) {
        d.x += d.speed * dt;
        if (d.speed > 0.f && d.x > Cfg::VIEW_W + 30.f) d.x = -30.f;
        if (d.speed < 0.f && d.x < -30.f) d.x = Cfg::VIEW_W + 30.f;
    }

    if (!eventsEnabled_) {
        if (ev_ != Event::None) EndEvent();
        evTimer_ = 15.f;
    } else if (ev_ == Event::None) {
        evTimer_ -= dt;
        if (evTimer_ <= 0.f) {
            Event e;
            do { e = (Event)GetRandomValue(1, 5); } while (e == lastEv_);
            StartEvent(e);
        }
    } else {
        evT_ += dt;
        if (evT_ >= evDur_) EndEvent();
    }
    const float target = (ev_ == Event::None) ? 0.f : Clamp(std::min(evT_, evDur_ - evT_) / 1.2f, 0.f, 1.f);
    eventFade_ = Lerp(eventFade_, target, 1.f - std::exp(-dt * 6.f));

    for (auto& d : drops_) {
        d.y += d.speed * dt; d.x -= d.speed * 0.22f * dt;
        if (d.y > Cfg::VIEW_H) { d.y = -10.f; d.x = (float)GetRandomValue(-20, Cfg::VIEW_W + 140); }
    }
    flash_ = std::max(0.f, flash_ - dt * 3.2f);
    boltT_ = std::max(0.f, boltT_ - dt);
    if (ev_ == Event::Storm) {
        lightTimer_ -= dt;
        if (lightTimer_ <= 0.f) {
            lightTimer_ = Util::RandRange(2.5f, 5.5f);
            flash_ = 1.f; boltT_ = 0.25f; lightningPending_ = true;
            bolt_.clear();
            float x = Util::RandRange(60.f, 580.f), y = 0.f;
            bolt_.push_back({ x, y });
            while (y < Cfg::GROUND_Y - 20.f) {
                y += Util::RandRange(14.f, 30.f);
                x += Util::RandRange(-22.f, 22.f);
                bolt_.push_back({ x, y });
            }
        }
    }

    for (auto& c : columns_) {
        c.y += c.speed * dt;
        if (c.y - c.len * 7 > Cfg::VIEW_H) { c.y = (float)GetRandomValue(-120, 0); c.speed = (float)GetRandomValue(60, 160); }
    }

    if (ev_ == Event::Meteors) {
        meteorTimer_ -= dt;
        if (meteorTimer_ <= 0.f) {
            meteorTimer_ = Util::RandRange(0.15f, 0.5f);
            meteors_.push_back({ { Util::RandRange(150.f, 760.f), -10.f }, { Util::RandRange(-380.f, -260.f), Util::RandRange(160.f, 240.f) }, 1.6f });
        }
    }
    for (auto& m : meteors_) { m.pos = Vector2Add(m.pos, Vector2Scale(m.vel, dt)); m.life -= dt; }
    meteors_.erase(std::remove_if(meteors_.begin(), meteors_.end(), [](const Meteor& m) { return m.life <= 0.f; }), meteors_.end());

    flickerT_ -= dt;
    if (flickerT_ <= 0.f) { flickerT_ = Util::RandRange(0.04f, 0.12f); flicker_ = Util::Rand01() < 0.55f ? 1.f : 0.25f; }
    const float wantLight = (ev_ == Event::Blackout) ? 1.f - 0.92f * eventFade_ : 1.f;
    const bool transition = (ev_ == Event::Blackout) && eventFade_ < 0.98f;
    lightFactor_ = wantLight * (transition ? flicker_ : 1.f);
}

void Background::DrawSun() const {
    const Vector2 c = { 470.f, 150.f };
    const float R = 62.f;
    for (int y = (int)-R; y <= (int)R; ++y) {
        if (y > 0 && ((y / 5) % 2 == 1) && y > 8) continue;
        float hw = std::sqrt(R * R - (float)(y * y));
        Color col = Util::LerpColor(cur_.sunTop, cur_.sunBot, ((float)y + R) / (2.f * R));
        DrawRectangle((int)(c.x - hw), (int)(c.y + y), (int)(hw * 2.f), 1, Fade(col, 0.9f));
    }
}

void Background::DrawLayer(int l, float time) const {
    static const Color BODY[3] = { {24, 14, 54, 255}, {17, 10, 40, 255}, {11, 7, 28, 255} };
    const Color body = Util::LerpColor(BODY[l], cur_.skyBot, l == 0 ? 0.30f : (l == 1 ? 0.14f : 0.05f));
    const float off = std::fmod(scroll_[l], wrap_[l]);

    for (int rep = 0; rep < 2; ++rep) {
        for (const auto& b : layers_[l]) {
            const float bx = b.x - off + rep * wrap_[l];
            if (bx > Cfg::VIEW_W || bx + b.w < 0.f) continue;
            const float top = Cfg::GROUND_Y - b.h;
            const Color acc = cur_.acc[b.accent];

            DrawRectangle((int)bx, (int)top, (int)b.w, (int)b.h, body);
            DrawRectangle((int)bx, (int)top, (int)b.w, 1, Fade(acc, (l == 0 ? 0.25f : 0.55f) * (0.35f + 0.65f * lightFactor_)));

            if (b.antenna) {
                const float ax = bx + b.w * 0.5f;
                DrawRectangle((int)ax, (int)top - 14, 1, 14, Fade(WHITE, 0.25f));
                if (std::fmod(time * 1.3f + (float)(b.seed % 10), 1.f) < 0.18f) {
                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawCircleGradient((int)ax, (int)top - 14, 6.f, Fade(Theme::NEON_RED, 0.9f), Fade(Theme::NEON_RED, 0.f));
                    EndBlendMode();
                    DrawRectangle((int)ax - 1, (int)top - 15, 3, 2, Theme::NEON_RED);
                }
            }
            if (l == 0) continue;

            int ix = 0;
            for (float wx = bx + 4.f; wx < bx + b.w - 6.f; wx += 8.f, ++ix) {
                int iy = 0;
                for (float wy = top + 8.f; wy < Cfg::GROUND_Y - 12.f; wy += 10.f, ++iy) {
                    unsigned h = Hash(b.seed, (unsigned)ix, (unsigned)iy);
                    if (h % (l == 1 ? 7u : 4u) != 0) continue;
                    bool flick = (h % 23u == 0) && std::fmod(time * 3.f + (float)(h % 100), 2.f) < 0.4f;
                    if (flick) continue;
                    const Color wc = cur_.acc[h % 3u];
                    const float a = (l == 1 ? 0.45f : 0.8f) * lightFactor_;
                    if (a > 0.02f) DrawRectangle((int)wx, (int)wy, 3, 4, Fade(wc, a));
                }
            }

            if (b.billboard && b.w > 44.f) {
                const float pw = std::min(b.w - 8.f, 56.f);
                const float px = bx + 4.f, py = top + 12.f;
                DrawRectangle((int)px, (int)py, (int)pw, 30, Fade(BLACK, 0.55f));
                DrawRectangleLines((int)px, (int)py, (int)pw, 30, Fade(acc, 0.7f * lightFactor_));
                const int bars = (int)(pw / 6.f);
                for (int k = 0; k < bars; ++k) {
                    float hh = 4.f + 20.f * (0.5f + 0.5f * std::sin(time * 3.f + k * 1.7f + (float)(b.seed % 7)));
                    DrawRectangle((int)(px + 3 + k * 6), (int)(py + 27 - hh), 4, (int)hh, Fade(cur_.acc[k % 3], 0.8f * lightFactor_));
                }
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangle((int)px - 2, (int)py - 2, (int)pw + 4, 34, Fade(acc, 0.08f * lightFactor_));
                EndBlendMode();
            }
            if (b.sign) {
                float sx = bx + b.w - 8.f;
                float pulse = (0.65f + 0.35f * std::sin(time * 4.f + (float)b.seed)) * lightFactor_;
                DrawRectangle((int)sx, (int)top + 14, 3, 46, Fade(acc, pulse));
                DrawRectangle((int)sx - 2, (int)top + 14, 7, 46, Fade(acc, 0.12f * pulse));
            }
        }
    }
}

void Background::DrawAirship(float time) const {
    if (ev_ != Event::Airship) return;
    const float k = evT_ / evDur_;
    const float x = -110.f + k * (Cfg::VIEW_W + 220.f);
    const float y = 66.f + std::sin(time * 0.8f) * 4.f;

    const float sweep = std::sin(time * 1.4f) * 120.f;
    const Vector2 apex = { x + 14.f, y + 10.f };
    const Vector2 l = { x + sweep - 38.f, Cfg::GROUND_Y }, r = { x + sweep + 38.f, Cfg::GROUND_Y };
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTriangle(apex, l, r, Fade(cur_.acc[0], 0.10f * eventFade_ + 0.04f));
    DrawEllipse((int)(x + sweep), (int)Cfg::GROUND_Y, 38.f, 5.f, Fade(cur_.acc[0], 0.28f));
    EndBlendMode();

    DrawEllipse((int)x, (int)y, 52.f, 13.f, Color{12, 9, 28, 255});
    DrawEllipse((int)x, (int)y - 2, 50.f, 3.f, Fade(cur_.acc[1], 0.4f));
    DrawTriangle({ x - 44.f, y }, { x - 62.f, y + 14.f }, { x - 40.f, y + 8.f }, Color{12, 9, 28, 255});
    DrawTriangle({ x - 44.f, y }, { x - 40.f, y - 8.f }, { x - 62.f, y - 14.f }, Color{12, 9, 28, 255});
    for (int i = 0; i < 6; ++i)
        DrawRectangle((int)(x - 22 + i * 9), (int)y - 1, 3, 3, Fade(cur_.acc[2], 0.8f));
    const bool blink = std::fmod(time * 2.f, 1.f) < 0.5f;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient((int)(x + 48.f), (int)y, 7.f, Fade(blink ? Theme::NEON_RED : Theme::NEON_GREEN, 0.9f), Fade(WHITE, 0.f));
    DrawCircleGradient((int)(x - 58.f), (int)y, 6.f, Fade(blink ? Theme::NEON_GREEN : Theme::NEON_RED, 0.9f), Fade(WHITE, 0.f));
    EndBlendMode();
}

void Background::DrawLightning() const {
    if (boltT_ <= 0.f || bolt_.size() < 2) return;
    BeginBlendMode(BLEND_ADDITIVE);
    for (size_t i = 1; i < bolt_.size(); ++i) {
        DrawLineEx(bolt_[i - 1], bolt_[i], 5.f, Fade(Theme::NEON_CYAN, 0.25f * boltT_ * 4.f));
        DrawLineEx(bolt_[i - 1], bolt_[i], 1.5f, Fade(WHITE, boltT_ * 4.f));
    }
    EndBlendMode();
}

void Background::DrawDecor(float time) const {
    static const int LAMPS[4] = { 70, 250, 390, 570 };
    for (int i = 0; i < 4; ++i) {
        const int x = LAMPS[i];
        const Color c = cur_.acc[i % 3];
        const bool flick = (i == 2) && std::fmod(time * 7.f, 5.f) < 0.25f;
        const float on = flick ? 0.2f : 1.f;
        DrawRectangle(x - 1, (int)Cfg::GROUND_Y - 46, 2, 46, Color{26, 20, 52, 255});
        DrawRectangle(x - 6, (int)Cfg::GROUND_Y - 48, 12, 3, Color{26, 20, 52, 255});
        DrawRectangle(x - 5, (int)Cfg::GROUND_Y - 46, 10, 2, Fade(c, 0.9f * on * (0.3f + 0.7f * lightFactor_)));
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(x, (int)Cfg::GROUND_Y - 44, 30.f, Fade(c, 0.28f * on * lightFactor_), Fade(c, 0.f));
        DrawEllipse(x, (int)Cfg::GROUND_Y + 1, 30.f, 3.f, Fade(c, 0.14f * on * lightFactor_));
        EndBlendMode();
    }
    for (int side = 0; side < 2; ++side) {
        const int bx = side == 0 ? 6 : Cfg::VIEW_W - 36;
        const Color c = cur_.acc[side == 0 ? 1 : 0];
        const int gy = (int)Cfg::GROUND_Y;
        DrawRectangle(bx, gy - 14, 14, 14, Color{34, 26, 62, 255});
        DrawRectangleLines(bx, gy - 14, 14, 14, Fade(c, 0.7f));
        DrawRectangle(bx + 16, gy - 10, 12, 10, Color{28, 22, 54, 255});
        DrawRectangleLines(bx + 16, gy - 10, 12, 10, Fade(c, 0.5f));
        DrawRectangle(bx + 4, gy - 26, 14, 12, Color{38, 28, 68, 255});
        DrawRectangleLines(bx + 4, gy - 26, 14, 12, Fade(c, 0.7f));
        DrawRectangle(bx + 7, gy - 22, 8, 2, Fade(c, 0.9f * (0.3f + 0.7f * lightFactor_)));
    }
}

void Background::DrawGround(float time) const {
    const int gy = (int)Cfg::GROUND_Y;
    DrawRectangle(0, gy, Cfg::VIEW_W, Cfg::VIEW_H - gy, Color{8, 6, 20, 255});

    const Color g = cur_.grid;
    for (int i = -14; i <= 14; ++i) {
        float xt = 320.f + i * 38.f;
        float xb = 320.f + i * 38.f * 1.9f;
        DrawLine((int)xt, gy, (int)xb, Cfg::VIEW_H, Fade(g, 0.45f));
    }
    for (int k = 0; k < 6; ++k) {
        float t = std::fmod((float)k + time * 0.6f, 6.f) / 6.f;
        float y = Cfg::GROUND_Y + (Cfg::VIEW_H - Cfg::GROUND_Y) * t * t;
        DrawLine(0, (int)y, Cfg::VIEW_W, (int)y, Fade(g, 0.15f + 0.4f * t));
    }
    DrawRectangle(0, gy, Cfg::VIEW_W, 1, cur_.acc[0]);
    DrawRectangle(0, gy + 1, Cfg::VIEW_W, 2, Fade(cur_.acc[0], 0.25f));
}

void Background::Draw(float time) const {
    DrawRectangleGradientV(0, 0, Cfg::VIEW_W, (int)Cfg::GROUND_Y, cur_.skyTop, cur_.skyBot);

    const float storm = (ev_ == Event::Storm) ? eventFade_ : 0.f;
    for (const auto& s : stars_) {
        float a = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(time * 2.f + s.phase));
        DrawRectangle((int)s.x, (int)s.y, 1, 1, Fade(WHITE, a * 0.8f * (1.f - storm)));
    }
    DrawSun();

    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& m : meteors_) {
        const Vector2 tail = Vector2Subtract(m.pos, Vector2Scale(m.vel, 0.16f));
        DrawLineEx(tail, m.pos, 3.f, Fade(cur_.acc[0], 0.35f));
        DrawLineEx(tail, m.pos, 1.2f, Fade(WHITE, 0.9f));
        DrawCircleGradient((int)m.pos.x, (int)m.pos.y, 6.f, Fade(WHITE, 0.8f), Fade(WHITE, 0.f));
    }
    EndBlendMode();

    DrawLightning();
    DrawLayer(0, time);
    DrawAirship(time);
    DrawLayer(1, time);

    if (ev_ == Event::DataRain || eventFade_ > 0.02f) {
        const float ef = (ev_ == Event::DataRain) ? eventFade_ : 0.f;
        if (ef > 0.01f) {
            BeginBlendMode(BLEND_ADDITIVE);
            for (const auto& c : columns_) {
                for (int i = 0; i < c.len; ++i) {
                    float y = c.y - i * 7.f;
                    if (y < -6.f || y > Cfg::GROUND_Y) continue;
                    float a = (1.f - (float)i / (float)c.len) * 0.6f * ef;
                    Color col = (i == 0) ? WHITE : cur_.acc[0];
                    DrawRectangle((int)c.x, (int)y, 3, 5, Fade(col, a));
                }
            }
            EndBlendMode();
        }
    }

    DrawLayer(2, time);

    for (const auto& d : drones_) {
        const float y = d.y + std::sin(time * 1.5f + d.phase) * 4.f;
        DrawRectangle((int)d.x - 5, (int)y, 10, 3, Color{30, 24, 56, 255});
        DrawRectangle((int)d.x - 8, (int)y - 2, 16, 1, Fade(WHITE, 0.3f));
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient((int)d.x, (int)y + 3, 6.f, Fade(cur_.acc[1], 0.55f * (0.4f + 0.6f * lightFactor_)), Fade(cur_.acc[1], 0.f));
        EndBlendMode();
    }

    const float dim = 0.34f * storm + 0.38f * (1.f - lightFactor_);
    if (dim > 0.01f) DrawRectangle(0, 0, Cfg::VIEW_W, (int)Cfg::GROUND_Y, Fade(BLACK, dim));

    DrawGround(time);
    DrawDecor(time);
}

void Background::DrawFront(float time) const {
    (void)time;
    const float storm = (ev_ == Event::Storm) ? eventFade_ : 0.f;
    if (storm > 0.02f) {
        for (const auto& d : drops_) {
            DrawLineEx({ d.x, d.y }, { d.x + 4.f, d.y + 11.f }, 1.f, Fade(Color{170, 200, 255, 255}, 0.42f * storm));
        }
        for (size_t i = 0; i < drops_.size(); i += 6) {
            const float sx = std::fmod(drops_[i].x * 7.3f + time * 90.f, (float)Cfg::VIEW_W);
            const float k = std::fmod(time * 5.f + (float)i, 1.f);
            DrawCircleLines((int)sx, (int)Cfg::GROUND_Y + 4 + (int)(k * 30.f), 2.f + k * 4.f, Fade(WHITE, (1.f - k) * 0.25f * storm));
        }
    }
    if (flash_ > 0.01f) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(Color{190, 215, 255, 255}, 0.16f * flash_));
        EndBlendMode();
    }
}
