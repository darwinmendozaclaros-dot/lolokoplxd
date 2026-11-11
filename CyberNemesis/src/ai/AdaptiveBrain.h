#pragma once
#include "ai/HeroTraits.h"
#include "ai/PatternTracker.h"
#include <string>
#include <vector>

struct BrainMemory {
    int   deaths   = 0;
    int   sessions = 0;
    float decisionMatrix[4][3] = {};
};

struct AdaptationReport {
    bool  enoughData = false;
    float rangedRatio = 0.f, meleeRatio = 0.f, predictability = 0.f, mobility = 0.f;
    int   fromGen = 1, toGen = 2;
    std::vector<std::string> lines;
};

class AdaptiveBrain {
public:
    bool Load();
    bool Save() const;
    void Reset();

    AdaptationReport Adapt(const Telemetry& t);

    const HeroTraits&  Traits() const { return traits_; }
    const BrainMemory& Memory() const { return memory_; }
    bool HasLearned() const { return traits_.generation > 1 || memory_.deaths > 0; }

private:
    HeroTraits  traits_;
    BrainMemory memory_;
};
