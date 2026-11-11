#pragma once
#include "raylib.h"

class Arena;

class Pickups {
public:
    void Clear() { alive_ = false; }
    void SpawnCore(const Arena& arena);
    bool Update(float dt, const Arena& arena, const Rectangle& collector);
    bool Active() const { return alive_; }
    Vector2 Position() const { return pos_; }
    void Draw(float time) const;

private:
    bool    alive_ = false;
    int     platform_ = 0;
    float   ox_ = 0.f, life_ = 0.f, t_ = 0.f;
    Vector2 pos_{0.f, 0.f};
};
