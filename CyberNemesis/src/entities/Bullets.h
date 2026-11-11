#pragma once
#include "raylib.h"
#include <vector>

struct Sprites;

enum class BulletKind { Orb, Shard, Star, Big };

struct Bullet {
    Vector2 base{0.f, 0.f}, pos{0.f, 0.f}, vel{0.f, 0.f};
    float   ax = 0.f, ay = 0.f;
    float   radius = 5.f, life = 9.f, delay = 0.f, age = 0.f, damage = 10.f;
    float   sineAmp = 0.f, sineFreq = 0.f, sinePhase = 0.f;
    Vector2 sineAxis{0.f, 1.f};
    int     bounces = 0, hue = 0;
    BulletKind kind = BulletKind::Orb;
    bool    alive = true;
};

struct Blaster {
    Vector2 pos{0.f, 0.f};
    float   angle = 0.f, t = 0.f, charge = 0.95f, fire = 0.5f;
    float   length = 1100.f, width = 28.f, damage = 24.f, spin = 0.f;
    bool    alive = true, lockedAim = false, fired = false;
};

struct BulletHit { bool hit = false; float damage = 0.f; Vector2 pos{0.f, 0.f}; };

class BulletField {
public:
    void Clear();
    void Update(float dt);
    void Add(Bullet b) { b.pos = b.base; b_.push_back(b); }
    void AddBlaster(const Blaster& bl) { bl_.push_back(bl); ++chargedPending_; }

    BulletHit Collide(const Rectangle& target, bool vulnerable);
    int  ClearIn(const Rectangle& zone);
    void SetAimTarget(Vector2 aimAt) { aimAt_ = aimAt; }

    int  Count() const { return (int)b_.size(); }
    int  BlasterCount() const { return (int)bl_.size(); }
    int  ConsumeCharged() { int r = chargedPending_; chargedPending_ = 0; return r; }
    int  ConsumeFired()   { int r = firedPending_;   firedPending_ = 0;   return r; }

    void Draw(const Sprites& sp, float time) const;

private:
    std::vector<Bullet>  b_;
    std::vector<Blaster> bl_;
    Vector2 aimAt_{320.f, 250.f};
    int chargedPending_ = 0, firedPending_ = 0;
};
