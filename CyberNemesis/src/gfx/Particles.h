#pragma once
#include "raylib.h"
#include <vector>

struct Particle {
    Vector2 pos, vel;
    float   life, maxLife, size, drag;
    Color   color;
    bool    gravity;
};

class ParticleSystem {
public:
    void Clear() { particles_.clear(); }
    void Update(float dt);
    void Draw() const;

    void Burst(Vector2 p, int n, Color c, float speed, float size, float life, bool gravity = false);
    void Directional(Vector2 p, Vector2 dir, float spread, int n, Color c,
                     float speed, float size, float life, bool gravity = false);
    void Ring(Vector2 p, int n, Color c, float speed, float size, float life);

private:
    void Add(Vector2 p, Vector2 v, float life, float size, float drag, Color c, bool g);
    std::vector<Particle> particles_;
    static constexpr size_t MAX_PARTICLES = 2500;
};
