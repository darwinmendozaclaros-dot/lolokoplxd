#include "gfx/Particles.h"
#include "core/Util.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

void ParticleSystem::Add(Vector2 p, Vector2 v, float life, float size, float drag, Color c, bool g) {
    if (particles_.size() >= MAX_PARTICLES) return;
    particles_.push_back({ p, v, life, life, size, drag, c, g });
}

void ParticleSystem::Burst(Vector2 p, int n, Color c, float speed, float size, float life, bool gravity) {
    for (int i = 0; i < n; ++i) {
        float a = Util::RandRange(0.f, 6.2831853f);
        float s = speed * Util::RandRange(0.25f, 1.f);
        Add(p, { std::cos(a) * s, std::sin(a) * s }, life * Util::RandRange(0.6f, 1.f),
            size * Util::RandRange(0.6f, 1.2f), 2.f, c, gravity);
    }
}

void ParticleSystem::Directional(Vector2 p, Vector2 dir, float spread, int n, Color c,
                                 float speed, float size, float life, bool gravity) {
    dir = Vector2Normalize(dir);
    for (int i = 0; i < n; ++i) {
        Vector2 d = Vector2Rotate(dir, Util::RandRange(-spread, spread));
        float s = speed * Util::RandRange(0.3f, 1.f);
        Add(p, Vector2Scale(d, s), life * Util::RandRange(0.6f, 1.f),
            size * Util::RandRange(0.6f, 1.2f), 2.5f, c, gravity);
    }
}

void ParticleSystem::Ring(Vector2 p, int n, Color c, float speed, float size, float life) {
    for (int i = 0; i < n; ++i) {
        float a = 6.2831853f * (float)i / (float)n;
        Add(p, { std::cos(a) * speed, std::sin(a) * speed }, life, size, 1.5f, c, false);
    }
}

void ParticleSystem::Update(float dt) {
    for (size_t i = 0; i < particles_.size();) {
        Particle& q = particles_[i];
        q.life -= dt;
        if (q.life <= 0.f) {
            q = particles_.back();
            particles_.pop_back();
            continue;
        }
        q.vel = Vector2Scale(q.vel, 1.f / (1.f + q.drag * dt));
        if (q.gravity) q.vel.y += 600.f * dt;
        q.pos = Vector2Add(q.pos, Vector2Scale(q.vel, dt));
        ++i;
    }
}

void ParticleSystem::Draw() const {
    for (const auto& q : particles_) {
        float k = q.life / q.maxLife;
        float s = std::max(1.f, q.size * k);
        DrawRectangleV({ std::floor(q.pos.x - s * 0.5f), std::floor(q.pos.y - s * 0.5f) },
                       { std::floor(s), std::floor(s) }, Fade(q.color, k));
    }
}
