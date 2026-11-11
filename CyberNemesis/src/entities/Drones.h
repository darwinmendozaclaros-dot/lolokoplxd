#pragma once
#include "raylib.h"
#include "entities/Projectile.h"
#include <vector>

struct Drone {
    Vector2 pos{0.f, 0.f};
    float   hp = 22.f, shootT = 1.f, phase = 0.f, life = 14.f, side = 1.f, hitFlash = 0.f;
    int     slot = 0;
    bool    alive = true;
};

class DroneSwarm {
public:
    void Clear() { drones_.clear(); deaths_.clear(); }
    void Spawn(int n, Vector2 from, float damage);
    void Update(float dt, Vector2 bossCenter, std::vector<ShotCmd>& shots);
    bool HitCircle(Vector2 p, float r, float dmg);
    void DestroyIn(const Rectangle& zone);
    std::vector<Vector2> TakeDeaths() { std::vector<Vector2> r; r.swap(deaths_); return r; }
    int  Count() const { int n = 0; for (const auto& d : drones_) if (d.alive) ++n; return n; }
    void Draw(float time) const;

private:
    std::vector<Drone>   drones_;
    std::vector<Vector2> deaths_;
    float damage_ = 6.f;
};
