#include "ai/PatternTracker.h"
#include "entities/Boss.h"
#include <algorithm>

void PatternTracker::Reset() {
    data_ = Telemetry{};
    sampleTimer_ = 0.f;
    prevState_ = 99;
    steady_ = samples_ = 0;
}

void PatternTracker::Sample(const Boss& boss, float dt) {
    data_.duration += dt;

    int zone = (int)(boss.pos.x / ((float)Cfg::VIEW_W / 3.f));
    zone = std::max(0, std::min(2, zone));
    data_.zoneTime[zone] += dt;

    if (!boss.OnGround()) data_.airTime += dt;
    if (boss.StandingPlatform() >= 0) data_.platformTime += dt;

    sampleTimer_ += dt;
    if (sampleTimer_ >= 0.2f) {
        sampleTimer_ -= 0.2f;
        int state = (boss.vel.x > 1.f) ? 1 : (boss.vel.x < -1.f ? -1 : 0);
        if (state == 1) ++data_.moveRight;
        else if (state == -1) ++data_.moveLeft;
        else ++data_.stationary;

        if (prevState_ != 99) {
            ++samples_;
            if (state == prevState_) ++steady_;
        }
        prevState_ = state;
    }
}

Telemetry PatternTracker::Snapshot() const {
    Telemetry t = data_;
    t.steadyRatio = samples_ > 0 ? (float)steady_ / (float)samples_ : 0.f;

    float maxZone = std::max(t.zoneTime[0], std::max(t.zoneTime[1], t.zoneTime[2]));
    float zoneShare = t.duration > 0.f ? maxZone / t.duration : 0.33f;
    float zoneDom = std::max(0.f, (zoneShare - 0.333f) / 0.667f);

    t.predictability = std::min(1.f, 0.65f * t.steadyRatio + 0.35f * zoneDom);
    return t;
}
