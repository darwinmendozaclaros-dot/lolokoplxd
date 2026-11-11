#pragma once
#include "raylib.h"
#include <vector>

enum class Owner { Boss, Hero };
enum class ProjKind { Plasma, Bolt, Pellet, Missile };

struct ShotCmd {
    Vector2  origin{0.f, 0.f}, dir{1.f, 0.f};
    float    speed = 320.f, damage = 1.f, radius = 4.f, life = 4.f;
    ProjKind kind = ProjKind::Bolt;
    bool     homing = false;
};

struct Projectile {
    Vector2  pos{0.f, 0.f};
    Vector2  vel{0.f, 0.f};
    float    radius = 6.f;
    float    damage = 1.f;
    float    life   = 4.f;
    Owner    owner  = Owner::Boss;
    ProjKind kind   = ProjKind::Plasma;
    bool     alive  = true;
    bool     homing = false;
    float    maxSpeed = 300.f;
    float    turnRate = 3.2f;
    int      id     = 0;
    std::vector<Vector2> trail;

    void Update(float dt, Vector2 homingTarget);
    void Draw(float time) const;
};
