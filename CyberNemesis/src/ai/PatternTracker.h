#pragma once
#include "core/Config.h"

class Boss;

struct Telemetry {
    int   melee = 0, ranged = 0;
    int   moveLeft = 0, moveRight = 0, stationary = 0;
    int   jumps = 0, dashes = 0;
    float airTime = 0.f, platformTime = 0.f;
    float zoneTime[3] = {0.f, 0.f, 0.f};
    float steadyRatio = 0.f;
    float predictability = 0.f;
    float duration = 0.f;

    int   TotalAttacks() const { return melee + ranged; }
    float MeleeRatio()  const { return TotalAttacks() ? (float)melee  / TotalAttacks() : 0.f; }
    float RangedRatio() const { return TotalAttacks() ? (float)ranged / TotalAttacks() : 0.f; }
    float PlatformRatio() const { return duration > 0.f ? platformTime / duration : 0.f; }
    float EvasionRate() const { return duration > 0.f ? (float)(jumps + dashes) / duration : 0.f; }
    float Mobility() const {
        float a = EvasionRate() / 0.9f, b = PlatformRatio() / 0.7f;
        return a > b ? a : b;
    }
};

class PatternTracker {
public:
    void Reset();
    void RecordMelee()  { ++data_.melee; }
    void RecordRanged() { ++data_.ranged; }
    void RecordJump()   { ++data_.jumps; }
    void RecordDash()   { ++data_.dashes; }
    void Sample(const Boss& boss, float dt);
    Telemetry Snapshot() const;

private:
    Telemetry data_;
    float sampleTimer_ = 0.f;
    int   prevState_   = 99;
    int   steady_      = 0;
    int   samples_     = 0;
};
