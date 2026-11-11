#pragma once
#include "raylib.h"

namespace Cfg {
    constexpr int   VIEW_W   = 640;
    constexpr int   VIEW_H   = 360;
    constexpr int   WINDOW_W = 1280;
    constexpr int   WINDOW_H = 720;

    constexpr float GROUND_Y = 300.0f;
    constexpr float GRAVITY  = 1400.0f;

    constexpr float BOSS_W               = 96.0f;
    constexpr float BOSS_H               = 96.0f;
    constexpr float BOSS_MAX_HP          = 1200.0f;
    constexpr float BOSS_SPEED           = 190.0f;
    constexpr float BOSS_ACCEL_GROUND    = 2600.0f;
    constexpr float BOSS_ACCEL_AIR       = 1500.0f;
    constexpr float BOSS_JUMP_V          = -540.0f;
    constexpr float BOSS_AIRJUMP_V       = -480.0f;
    constexpr float BOSS_COYOTE_TIME     = 0.10f;
    constexpr float BOSS_JUMP_BUFFER     = 0.12f;
    constexpr float BOSS_DASH_SPEED      = 620.0f;
    constexpr float BOSS_DASH_TIME       = 0.16f;
    constexpr float BOSS_DASH_COOLDOWN   = 0.85f;
    constexpr float BOSS_DIVE_SPEED      = 900.0f;
    constexpr float BOSS_MELEE_DAMAGE    = 45.0f;
    constexpr float BOSS_MELEE_WINDUP    = 0.28f;
    constexpr float BOSS_MELEE_COOLDOWN  = 0.90f;
    constexpr float BOSS_MELEE_REACH     = 95.0f;
    constexpr float BOSS_MELEE_HEIGHT    = 70.0f;
    constexpr float BOSS_SHOT_DAMAGE     = 35.0f;
    constexpr float BOSS_SHOT_SPEED      = 520.0f;
    constexpr float BOSS_SHOT_COOLDOWN   = 0.55f;

    constexpr float HERO_W               = 64.0f;
    constexpr float HERO_H               = 64.0f;
    constexpr float HERO_BASE_HP         = 220.0f;
    constexpr float HERO_HP_PER_GEN      = 45.0f;
    constexpr float HERO_BASE_SPEED      = 140.0f;
    constexpr float HERO_BASE_DAMAGE     = 7.0f;
    constexpr float HERO_BASE_FIRE       = 0.55f;
    constexpr float HERO_BASE_DODGE      = 0.25f;
    constexpr float HERO_BOLT_SPEED      = 320.0f;
    constexpr float HERO_JUMP_V          = -560.0f;
    constexpr float HERO_JET_V           = -470.0f;
    constexpr float DASH_SPEED           = 520.0f;
    constexpr float DASH_TIME            = 0.20f;
    constexpr float DASH_COOLDOWN        = 1.60f;
    constexpr float ORBITAL_DELAY        = 1.00f;
    constexpr float MINE_ARM_TIME        = 0.80f;

    constexpr float RATIO_THRESHOLD      = 0.55f;
    constexpr float PREDICT_THRESHOLD    = 0.60f;
    constexpr float MOBILITY_THRESHOLD   = 0.50f;
    constexpr float DEFLECT_REDUCTION    = 0.70f;
    constexpr int   MIN_ATTACK_SAMPLE    = 6;
    constexpr int   MAX_GENERATION       = 6;


    inline constexpr const char* BRAIN_FILE = "brain.data";

    constexpr float BOSS_MIN_SCALE       = 0.5f;
    inline float BossScaleForGeneration(int gen) {
        const int g = gen < 1 ? 1 : (gen > MAX_GENERATION ? MAX_GENERATION : gen);
        return 1.f - (1.f - BOSS_MIN_SCALE) * (float)(g - 1) / (float)(MAX_GENERATION - 1);
    }

    constexpr float EVENT_FIRST_DELAY    = 14.0f;
    constexpr float PHASE1_FRAC          = 0.66f;
    constexpr float PHASE2_FRAC          = 0.33f;
    constexpr float CORE_HEAL            = 140.0f;
    constexpr float OVERCHARGE_TIME      = 8.0f;

    constexpr float FINAL_HERO_HP        = 900.0f;
    constexpr float BULLET_DAMAGE_SMALL  = 10.0f;
    constexpr float BULLET_DAMAGE_BIG    = 16.0f;
    constexpr float BEAM_DAMAGE          = 24.0f;
    constexpr float PILLAR_DAMAGE        = 30.0f;
    constexpr float BOSS_HIT_INVULN      = 0.7f;
}

// ---------------------------------------------------------------------------
// El Serafin renace varias veces ("Ecos"). Empieza sabiendo casi nada y cada
// muerte lo vuelve mas fuerte. Todo el balance de la saga esta en esta tabla.
// ---------------------------------------------------------------------------
namespace Seraph {
    constexpr int CYCLES = 4;           // vidas totales (la ultima es la forma completa)

    // El Serafin hereda lo aprendido por los Heroes: desde su primera vida ya esquiva, ataca con intencion
    // e invoca copias. Sin fases: su dificultad es fija en cada vida y solo mejora al morir y volver.
    struct Stats {
        float hp;         // vida maxima en esta vida
        float attack;     // 0..1: cantidad, velocidad, cadencia y dano de las balas
        float dodge;      // probabilidad de esquivar un golpe del Boss (0..1)
        float moveMul;    // agilidad al volar / caminar
        int   level;      // intensidad base de los patrones (0..2)
        bool  shield;     // escudo de deflexion (solo si el Heroe lo habia aprendido)
        // --- mecanicas de invocacion para esquivar ---
        int   decoys;     // senuelos que invoca a la vez (0 = no sabe; 2 = clones de espejo)
        int   decoyHits;  // disparos que absorbe cada senuelo
        float decoyCd;    // recarga del senuelo (s)
        int   guardOrbs;  // orbes guardianes que invoca (0 = no sabe)
        float guardCd;    // recarga de los orbes (s)
    };

    inline Stats ForCycle(int cycle) {
        static constexpr Stats T[CYCLES] = {
            //  hp      atk    dodge  move   lvl shield  dec hits  decCd  orbs  orbCd
            { 300.f, 0.70f, 0.12f, 0.80f, 1, false,   1,  1,  8.0f,  0,  0.0f },   // vida 1: ya esquiva y invoca una copia
            { 380.f, 0.80f, 0.20f, 0.90f, 2, true,    1,  2,  6.5f,  2, 12.0f },   // vida 2: copias resistentes + orbes guardianes
            { 440.f, 0.90f, 0.28f, 0.95f, 2, true,    2,  2,  6.0f,  3, 11.0f },   // vida 3: clones de espejo
            { 520.f, 1.00f, 0.35f, 1.00f, 2, true,    2,  2,  6.0f,  4, 10.0f },   // vida 4: forma completa (con menos vida que antes)
        };
        const int i = cycle < 1 ? 0 : (cycle > CYCLES ? CYCLES - 1 : cycle - 1);
        return T[i];
    }
}

namespace Atk {
    constexpr unsigned BOLT    = 1u;
    constexpr unsigned BURST   = 2u;
    constexpr unsigned SPREAD  = 4u;
    constexpr unsigned MISSILE = 8u;
    constexpr unsigned ORBITAL = 16u;
    constexpr unsigned MINE    = 32u;
}

namespace Theme {
    inline constexpr Color NEON_CYAN    {24, 240, 255, 255};
    inline constexpr Color NEON_MAGENTA {255, 43, 214, 255};
    inline constexpr Color NEON_YELLOW  {255, 230, 60, 255};
    inline constexpr Color NEON_RED     {255, 48, 64, 255};
    inline constexpr Color NEON_GREEN   {57, 255, 120, 255};
    inline constexpr Color NEON_DARK    {10, 8, 24, 255};
}
