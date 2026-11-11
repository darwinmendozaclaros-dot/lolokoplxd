#pragma once
#include "raylib.h"

namespace Util {
    inline float Rand01() { return (float)GetRandomValue(0, 10000) / 10000.0f; }
    inline float RandRange(float a, float b) { return a + (b - a) * Rand01(); }
    inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

    inline Color LerpColor(Color a, Color b, float t) {
        t = Clamp01(t);
        return Color{ (unsigned char)(a.r + (b.r - a.r) * t),
                      (unsigned char)(a.g + (b.g - a.g) * t),
                      (unsigned char)(a.b + (b.b - a.b) * t),
                      (unsigned char)(a.a + (b.a - a.a) * t) };
    }
}
