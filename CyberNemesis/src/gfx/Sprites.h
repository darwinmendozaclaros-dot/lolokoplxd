#pragma once
#include "raylib.h"

struct Sprites {
    static constexpr int CELL = 32;

    enum BossFrame { B_IDLE, B_WALK_A, B_WALK_B, B_JUMP, B_FALL, B_WINDUP, B_SLAM, B_SHOOT, B_DIVE, B_COUNT };
    enum HeroLegs  { L_STAND, L_RUN0, L_RUN1, L_RUN2, L_RUN3, L_JUMP, L_FALL, L_COUNT };
    enum HeroArms  { A_READY, A_AIM, A_COUNT };
    static int HeroFrame(int arms, int legs) { return arms * L_COUNT + legs; }

    Texture2D boss{}, bossGlow{}, hero{}, heroGlow{};
    Texture2D glow{};

    void Init();
    void Shutdown();

    void DrawGlow(Vector2 p, float radius, Color c) const;

    void DrawBoss(int frame, Vector2 feet, float size, bool faceRight, float lean,
                  float sx, float sy, Color tint, float glow) const;
    void DrawHero(int frame, Vector2 feet, float size, bool faceRight, float lean,
                  float sx, float sy, Color tint, float glow) const;

private:
    static void DrawSheet(const Texture2D& tex, const Texture2D& glowTex, int frame, Vector2 feet,
                          float size, bool faceRight, float lean, float sx, float sy, Color tint, float glow);
};
