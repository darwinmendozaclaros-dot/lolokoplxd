#pragma once
#include "raylib.h"
#include <vector>

struct Platform {
    Rectangle rect{0.f, 0.f, 0.f, 8.f};
    Vector2   origin{0.f, 0.f};
    Vector2   amp{0.f, 0.f};
    float     speed = 0.f, phase = 0.f;
    Vector2   delta{0.f, 0.f};
    int       kind = 0;
};

class Arena {
public:
    Arena() { Reset(); }
    void Reset();
    void Update(float dt);
    void Draw(float time) const;

    std::vector<Platform> platforms;

private:
    void Add(float cx, float topY, float w, float ampX, float ampY, float speed, float phase, int kind);
    void Place(Platform& p, bool computeDelta);
    float t_ = 0.f;
};
