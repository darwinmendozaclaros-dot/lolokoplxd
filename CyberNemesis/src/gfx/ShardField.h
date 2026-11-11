#pragma once
#include "raylib.h"
#include <vector>

class ShardField {
public:
    void Init(unsigned seed);
    void Update(float dt, Vector2 parallax);
    void Draw() const;
    void SetTint(Color c) { tint_ = c; }

private:
    struct Shard {
        int   ribbon = 0;
        float u0 = 0.f, speed = 0.f, offA = 0.f, offB = 0.f, size = 10.f, stretch = 1.f, phase = 0.f;
        float rx = 0.f, ry = 0.f, rz = 0.f, vx = 0.f, vy = 0.f, vz = 0.f;
        float ang[4] = {}, rad[4] = {};
        float px[4] = {}, py[4] = {};
        float cx = 0.f, cy = 0.f, z = 0.f, scale = 1.f, bright = 0.5f, alpha = 1.f, dof = 0.f;
    };
    struct Marker { float x, y, phase, a; };
    struct Cloud  { float x, y, r, speed, phase; };

    std::vector<Shard>  shards_;
    std::vector<int>    order_;
    std::vector<Marker> markers_;
    std::vector<Cloud>  clouds_;
    Vector2 reticle_{320.f, 180.f}, reticleTarget_{320.f, 180.f};
    float   reticleT_ = 0.f, time_ = 0.f, parx_ = 0.f, pary_ = 0.f;
    Color   tint_{150, 105, 225, 255};
};
