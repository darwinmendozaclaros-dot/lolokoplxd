#include "gfx/ShardField.h"
#include "core/Config.h"
#include "core/Util.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
    constexpr float PI_F = 3.14159265f;
    constexpr float FOCAL = 300.f;
    constexpr float CX = 320.f, CY = 188.f;

    struct V3 { float x, y, z; };

    V3 RibbonPoint(int r, float u, float t) {
        switch (r) {
            case 0:  return { -440.f + 880.f * u, -112.f + 238.f * u + 30.f * std::sin(6.2832f * 1.1f * u + t * 0.5f),
                              -20.f + 190.f * std::sin(PI_F * u) };
            case 1:  return { -440.f + 880.f * u, 130.f - 190.f * u + 26.f * std::sin(6.2832f * 1.5f * u + t * 0.4f + 1.f),
                              -10.f + 170.f * std::sin(PI_F * u + 0.7f) };
            default: return { -500.f + 1000.f * u, -135.f + 40.f * std::sin(6.2832f * 0.8f * u + t * 0.3f),
                              300.f + 70.f * std::sin(6.2832f * u + t * 0.2f) };
        }
    }

    V3 Rotate(V3 v, float rx, float ry, float rz) {
        float c = std::cos(rz), s = std::sin(rz);
        V3 a = { v.x * c - v.y * s, v.x * s + v.y * c, v.z };
        c = std::cos(ry); s = std::sin(ry);
        V3 b = { a.x * c + a.z * s, a.y, -a.x * s + a.z * c };
        c = std::cos(rx); s = std::sin(rx);
        return { b.x, b.y * c - b.z * s, b.y * s + b.z * c };
    }

    void FillTri(Vector2 a, Vector2 b, Vector2 c, Color col) {
        const float cr = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        if (cr > 0.f) DrawTriangle(a, c, b, col);
        else          DrawTriangle(a, b, c, col);
    }
}

void ShardField::Init(unsigned seed) {
    std::mt19937 rng(seed);
    auto R = [&](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };
    std::normal_distribution<float> gauss(0.f, 1.f);

    shards_.clear();
    const int counts[3] = { 230, 230, 110 };
    const float spread[3] = { 30.f, 30.f, 75.f };
    for (int r = 0; r < 3; ++r) {
        for (int i = 0; i < counts[r]; ++i) {
            Shard s;
            s.ribbon = r;
            s.u0 = R(0.f, 1.f);
            s.speed = (r == 2 ? 0.012f : 0.03f) * R(0.7f, 1.3f);
            s.offA = gauss(rng) * spread[r];
            s.offB = gauss(rng) * spread[r];
            s.size = r == 2 ? R(14.f, 34.f) : R(6.f, 22.f);
            s.stretch = R(1.f, 2.3f);
            s.phase = R(0.f, 6.28f);
            s.rx = R(0.f, 6.28f); s.ry = R(0.f, 6.28f); s.rz = R(0.f, 6.28f);
            s.vx = R(-0.7f, 0.7f); s.vy = R(-0.7f, 0.7f); s.vz = R(-0.5f, 0.5f);
            for (int k = 0; k < 4; ++k) {
                s.ang[k] = (float)k * (PI_F * 0.5f) + R(-0.45f, 0.45f);
                s.rad[k] = R(0.45f, 1.2f);
            }
            shards_.push_back(s);
        }
    }
    order_.resize(shards_.size());
    for (size_t i = 0; i < order_.size(); ++i) order_[i] = (int)i;

    markers_.clear();
    for (int j = 0; j < 7; ++j)
        for (int i = 0; i < 9; ++i)
            markers_.push_back({ 18.f + (float)i * 78.f + ((j % 2) ? 39.f : 0.f), 14.f + (float)j * 56.f,
                                 R(0.f, 6.28f), R(0.12f, 0.4f) });
    clouds_.clear();
    for (int i = 0; i < 9; ++i)
        clouds_.push_back({ R(0.f, 640.f), R(60.f, 300.f), R(70.f, 150.f), R(-6.f, 6.f), R(0.f, 6.28f) });
}

void ShardField::Update(float dt, Vector2 par) {
    time_ += dt;
    const float k = std::min(1.f, dt * 3.f);
    parx_ += (par.x - parx_) * k;
    pary_ += (par.y - pary_) * k;

    for (auto& c : clouds_) {
        c.x += c.speed * dt;
        if (c.x < -170.f) c.x = 810.f;
        if (c.x > 810.f)  c.x = -170.f;
    }

    for (auto& s : shards_) {
        s.rx += s.vx * dt; s.ry += s.vy * dt; s.rz += s.vz * dt;
        const float u = std::fmod(s.u0 + time_ * s.speed, 1.f);
        V3 p = RibbonPoint(s.ribbon, u, time_);
        p.y += s.offA * std::cos(time_ * 0.35f + s.phase);
        p.z += s.offB * std::sin(time_ * 0.30f + s.phase * 1.3f);
        p.x += s.offA * 0.4f * std::sin(time_ * 0.2f + s.phase);

        V3 loc[4], rot[4];
        for (int v = 0; v < 4; ++v) {
            loc[v] = { std::cos(s.ang[v]) * s.rad[v] * s.size * s.stretch, std::sin(s.ang[v]) * s.rad[v] * s.size, 0.f };
            rot[v] = Rotate(loc[v], s.rx, s.ry, s.rz);
            const float zz = std::max(-FOCAL + 40.f, p.z + rot[v].z);
            const float sc = FOCAL / (FOCAL + zz);
            s.px[v] = CX + (p.x + rot[v].x) * sc + parx_ * 36.f * sc;
            s.py[v] = CY + (p.y + rot[v].y) * sc + pary_ * 22.f * sc;
        }
        const V3 e1 = { rot[1].x - rot[0].x, rot[1].y - rot[0].y, rot[1].z - rot[0].z };
        const V3 e2 = { rot[2].x - rot[0].x, rot[2].y - rot[0].y, rot[2].z - rot[0].z };
        V3 n = { e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x };
        const float nl = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z) + 1e-4f;
        const float nz = std::fabs(n.z / nl), nx = std::fabs(n.x / nl);
        s.bright = Clamp(0.25f + 0.6f * nz + 0.35f * nx, 0.f, 1.f);

        s.z = p.z;
        s.scale = FOCAL / (FOCAL + std::max(-FOCAL + 40.f, p.z));
        s.cx = CX + p.x * s.scale + parx_ * 36.f * s.scale;
        s.cy = CY + p.y * s.scale + pary_ * 22.f * s.scale;
        s.alpha = std::pow(std::max(0.f, std::sin(PI_F * u)), 0.6f);
        s.dof = Clamp((p.z - 100.f) / 150.f, 0.f, 1.f);
    }

    std::sort(order_.begin(), order_.end(), [&](int a, int b) { return shards_[a].z > shards_[b].z; });

    reticleT_ -= dt;
    if (reticleT_ <= 0.f) {
        reticleT_ = Util::RandRange(2.f, 3.5f);
        for (int tries = 0; tries < 30; ++tries) {
            const Shard& s = shards_[GetRandomValue(0, (int)shards_.size() - 1)];
            if (s.dof < 0.3f && s.alpha > 0.6f && s.cx > 40.f && s.cx < 600.f && s.cy > 40.f && s.cy < 320.f) {
                reticleTarget_ = { s.cx, s.cy };
                break;
            }
        }
    }
    const float rk = 1.f - std::exp(-dt * 5.f);
    reticle_.x += (reticleTarget_.x - reticle_.x) * rk;
    reticle_.y += (reticleTarget_.y - reticle_.y) * rk;
}

void ShardField::Draw() const {
    const int W = Cfg::VIEW_W, H = Cfg::VIEW_H;
    const Color fill = Util::LerpColor(tint_, Color{40, 20, 80, 255}, 0.35f);
    const Color edge = Util::LerpColor(tint_, WHITE, 0.30f);
    const Color soft = Util::LerpColor(tint_, Color{60, 30, 110, 255}, 0.4f);

    DrawRectangleGradientV(0, 0, W, H, Color{24, 15, 36, 255}, Color{9, 6, 17, 255});

    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& c : clouds_) {
        const float a = 0.10f + 0.04f * std::sin(time_ * 0.4f + c.phase);
        DrawCircleGradient((int)c.x, (int)c.y, c.r, Fade(soft, a), Fade(soft, 0.f));
    }
    for (int idx : order_) {
        const Shard& s = shards_[idx];
        if (s.dof < 0.12f) continue;
        const float r = s.size * s.scale * (1.2f + s.dof * 2.0f);
        DrawCircleGradient((int)s.cx, (int)s.cy, r, Fade(soft, 0.10f * s.dof * s.alpha + 0.01f), Fade(soft, 0.f));
    }
    EndBlendMode();

    for (const auto& m : markers_) {
        const float a = m.a * (0.6f + 0.4f * std::sin(time_ * 1.6f + m.phase));
        const int x = (int)m.x, y = (int)m.y;
        DrawRectangle(x - 3, y, 7, 1, Fade(edge, a));
        DrawRectangle(x, y - 3, 1, 7, Fade(edge, a));
    }

    for (int idx : order_) {
        const Shard& s = shards_[idx];
        const float sharp = 1.f - s.dof;
        if (sharp < 0.05f || s.alpha < 0.03f) continue;
        if (s.cx < -80.f || s.cx > W + 80.f || s.cy < -80.f || s.cy > H + 80.f) continue;
        const Vector2 p0 = { s.px[0], s.py[0] }, p1 = { s.px[1], s.py[1] }, p2 = { s.px[2], s.py[2] }, p3 = { s.px[3], s.py[3] };
        const float a = sharp * s.alpha;
        const Color f = Util::LerpColor(fill, edge, s.bright * 0.35f);
        FillTri(p0, p1, p2, Fade(f, (0.18f + 0.36f * s.bright) * a));
        FillTri(p0, p2, p3, Fade(f, (0.14f + 0.30f * s.bright) * a));
        const Color e = Fade(edge, (0.18f + 0.48f * s.bright) * a);
        DrawLineEx(p0, p1, 1.f, e); DrawLineEx(p1, p2, 1.f, e);
        DrawLineEx(p2, p3, 1.f, e); DrawLineEx(p3, p0, 1.f, e);
    }

    BeginBlendMode(BLEND_ADDITIVE);
    for (int idx : order_) {
        const Shard& s = shards_[idx];
        if (s.dof > 0.4f || s.bright < 0.86f || s.alpha < 0.3f) continue;
        const int k = idx % 4;
        DrawLineEx({ s.px[k], s.py[k] }, { s.px[(k + 1) % 4], s.py[(k + 1) % 4] }, 1.f,
                   Fade(WHITE, 0.38f * (1.f - s.dof) * s.alpha));
    }
    EndBlendMode();

    {
        const float sz = 11.f + 2.f * std::sin(time_ * 6.f);
        const int x = (int)reticle_.x, y = (int)reticle_.y, a = 5;
        const Color c = Fade(WHITE, 0.75f);
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sy = -1; sy <= 1; sy += 2) {
                const int cx = x + sx * (int)sz, cy = y + sy * (int)sz;
                DrawRectangle(sx > 0 ? cx - a : cx, cy, a + 1, 1, c);
                DrawRectangle(cx, sy > 0 ? cy - a : cy, 1, a + 1, c);
            }
        DrawRectangle(x, y, 1, 1, Fade(WHITE, 0.7f));
    }

    DrawRectangleGradientH(0, 0, 90, H, Fade(BLACK, 0.7f), Fade(BLACK, 0.f));
    DrawRectangleGradientH(W - 90, 0, 90, H, Fade(BLACK, 0.f), Fade(BLACK, 0.7f));
    DrawRectangleGradientV(0, 0, W, 60, Fade(BLACK, 0.65f), Fade(BLACK, 0.f));
    DrawRectangleGradientV(0, H - 60, W, 60, Fade(BLACK, 0.f), Fade(BLACK, 0.7f));
}
