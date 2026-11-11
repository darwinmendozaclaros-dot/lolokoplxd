#pragma once
#include "raylib.h"
#include <vector>

struct Mine {
    Vector2 pos{0.f, 0.f};
    float   arm = 0.8f, life = 10.f, damage = 40.f;
    bool    alive = true;
};

struct Orbital {
    float x = 0.f, t = 0.f, delay = 1.f, width = 46.f, damage = 60.f;
    bool  fired = false;
    bool  neutral = false;
    float fireT = 0.f;
};

struct HazardEvent {
    enum Type { MineBlast, MineFizzle, OrbitalFire } type;
    Vector2 pos;
    float   radius;
    float   damage;
    bool    neutral = false;
};

class Hazards {
public:
    void Clear() { mines_.clear(); orbitals_.clear(); }
    void SpawnMine(Vector2 p, float damage);
    void SpawnOrbital(float x, float damage);
    void SpawnStrike(float x, float damage);
    std::vector<float> NeutralColumns() const;
    void Update(float dt, const Rectangle& bossHurt, float bossFeetY, std::vector<HazardEvent>& out);
    void DestroyMinesIn(const Rectangle& zone, std::vector<HazardEvent>& out);
    void Draw(float time) const;

private:
    std::vector<Mine>    mines_;
    std::vector<Orbital> orbitals_;
};
